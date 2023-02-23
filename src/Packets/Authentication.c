#include "Authentication.h"
#include "NetworkBuffer.h"
#include "Logger.h"
#include <curl/curl.h>
#include <stdlib.h>
#include <json.h>
#include <libc.h>

#define MAX_TOKEN_LENGTH 2048

typedef struct AuthenticationDetails {
    NetworkBuffer *ms_access_token;
    NetworkBuffer *xbl_token;
    NetworkBuffer *xsts_token;
    NetworkBuffer *mc_token;
    NetworkBuffer *player_hash;
} AuthenticationDetails;

AuthenticationDetails *authentication_details_new() {
    AuthenticationDetails *details = malloc(sizeof(AuthenticationDetails));
    details->ms_access_token = buffer_new();
    details->xbl_token = buffer_new();
    details->xsts_token = buffer_new();
    details->mc_token = buffer_new();
    details->player_hash = buffer_new();
    return details;
}

void authentication_details_free(AuthenticationDetails *details) {
    buffer_free(details->ms_access_token);
    buffer_free(details->xbl_token);
    buffer_free(details->xsts_token);
    buffer_free(details->mc_token);
    buffer_free(details->player_hash);
    free(details);
}

size_t save_response(char *ptr, size_t size, size_t nmemb, void *userdata) {
    NetworkBuffer *buffer = userdata;
    buffer_write_little_endian(buffer, ptr, size * nmemb);
    return size * nmemb;
}

void auth_token_get(AuthenticationDetails *details) {
    FILE *file = fopen(".token", "rb");
    if (file) {
        char *token = malloc(MAX_TOKEN_LENGTH);
        size_t length = 0;
        fread((char **) &length, sizeof(size_t), 1, file);
        fread(token, sizeof(char), length, file);
        buffer_write_little_endian(details->ms_access_token, token, length);
    }
    fclose(file);
}

void auth_token_save(AuthenticationDetails *details) {
    FILE *file = fopen(".token", "wb");
    fwrite(&details->ms_access_token->size, 1, sizeof(size_t), file);
    fwrite(details->ms_access_token->bytes, details->ms_access_token->size, sizeof(char), file);
    fclose(file);
}

void poll_xbl_access(AuthenticationDetails *details) {
    json_object *request = json_object_new_object();
    json_object *properties = json_object_new_object();

    json_object_object_add(properties, "AuthMethod", json_object_new_string("RPS"));
    json_object_object_add(properties, "SiteName", json_object_new_string("user.auth.xboxlive.com"));

    json_object_object_add(
            properties,
            "RpsTicket",
            json_object_new_string_len(
                    details->ms_access_token->bytes,
                    (int) details->ms_access_token->size
                    )
                );

    json_object_object_add(request, "Properties", properties);
    json_object_object_add(request, "RelyingParty", json_object_new_string("http://auth.xboxlive.com"));
    json_object_object_add(request, "TokenType", json_object_new_string("JWT"));

    CURL *curl;

    curl = curl_easy_init();
    if (curl){
        curl_easy_setopt(curl, CURLOPT_URL, "https://user.auth.xboxlive.com/user/authenticate");

        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);

        size_t json_len;
        const char *request_string = json_object_to_json_string_length(request, 0, &json_len);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_string);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, json_len);



        struct curl_slist *headers = curl_slist_append(NULL, "Content-Type: application/json");
        headers = curl_slist_append(headers, "Accept: application/json");
        headers = curl_slist_append(headers, "x-xbl-contract-version: 2");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        NetworkBuffer *response = buffer_new();
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &save_response);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);

        cmc_log(INFO, "Sending: %s", request_string);

        CURLcode res = curl_easy_perform(curl);

        json_object *json_response = json_tokener_parse((char *) response->bytes);
        cmc_log(INFO, "%s", json_object_get_string(json_response));
        json_object *json_token = json_object_object_get(json_response, "Token");
        buffer_write_little_endian(details->xbl_token, json_object_get_string(json_token), json_object_get_string_len(json_token));
    }
}

void poll_xbl_user_authentication(char *device_code, AuthenticationDetails *details) {
    CURL *curl;
    curl = curl_easy_init();
    if (curl){
        char data[1024];
        strcpy(data, "grant_type=urn:ietf:params:oauth:grant-type:device_code&client_id=00000000441cc96b&device_code=");
        strcat(data, device_code);


        curl_easy_setopt(curl, CURLOPT_URL, "https://login.live.com/oauth20_token.srf");


        //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(data));

        NetworkBuffer *response = buffer_new();
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &save_response);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);

        CURLcode res = curl_easy_perform(curl);

        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

        if (response_code == 400) {
            json_object *json_response = json_tokener_parse((char *) response->bytes);
            json_object *error = json_object_object_get(json_response, "error");
            if (strcmp(json_object_get_string(error), "authorization_declined") == 0) {
                cmc_log(ERR, "User declined XBox live authorization.");
                json_object_put(json_response);
                exit(EXIT_FAILURE);
            } else if (strcmp(json_object_get_string(error), "bad_verification_code") == 0) {
                cmc_log(ERR, "Invalid XBox live token.");
                json_object_put(json_response);
                exit(EXIT_FAILURE);
            } else if(strcmp(json_object_get_string(error), "expired_token") == 0) {
                cmc_log(INFO, "XBox live token expired.");
                cmc_log(INFO, "Please retry with a new token.");
                json_object_put(json_response);
                exit(EXIT_FAILURE);
            } else if (strcmp(json_object_get_string(error), "authorization_pending") == 0) {
                json_object_put(json_response);
                cmc_log(INFO, "User is currently authenticating. Awaiting response...");
                sleep(3);
                poll_xbl_user_authentication(device_code, details);
            }
        } else if (response_code == 200) {
            json_object *json_response = json_tokener_parse((char *) response->bytes);
            json_object *json_token = json_object_object_get(json_response, "access_token");
            cmc_log(INFO, "Received json_token: %s", json_object_get_string(json_token));
            cmc_log(INFO, "Response: %s", json_object_get_string(json_response));
            buffer_write_little_endian(details->ms_access_token, json_object_get_string(json_token), json_object_get_string_len(json_token));
            json_object_get(json_token);
            json_object_put(json_response);
        }
    }
    curl_easy_cleanup(curl);
}


void authenticate_xbl(AuthenticationDetails *details) {
    CURL *curl;
    curl = curl_easy_init();
    if (curl){

        char *data = "scope=service::user.auth.xboxlive.com::MBI_SSL&client_id=00000000441cc96b&response_type=device_code";
        curl_easy_setopt(curl, CURLOPT_URL, "https://login.live.com/oauth20_connect.srf");


        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(data));

        NetworkBuffer *response = buffer_new();
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &save_response);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);

        curl_easy_perform(curl);

        json_object *json_response = json_tokener_parse((char *) response->bytes);
        json_object *user_code = json_object_object_get(json_response, "user_code");
        json_object *device_code = json_object_object_get(json_response, "device_code");
        json_object *verification_uri = json_object_object_get(json_response, "verification_uri");
        cmc_log(INFO,
                "Please go to %s and enter the code %s.",
                json_object_get_string(verification_uri),
                json_object_get_string(user_code)
                );

        buffer_remove(response, response->size);

        cmc_log(DEBUG, "Device code %s", json_object_get_string(device_code));

        poll_xbl_user_authentication(json_object_get_string(device_code), details);

        json_object_put(json_response);

        buffer_free(response);
    }

    curl_easy_cleanup(curl);

    auth_token_save(details);
}

void authenticate_xsts(AuthenticationDetails *details) {
    json_object *request = json_object_new_object();
    json_object *properties = json_object_new_object();

    json_object_object_add(properties, "SandboxId", json_object_new_string("RETAIL"));
    json_object *user_token = json_object_new_array_ext(1);
    json_object_array_add(user_token, json_object_new_string_len(details->xbl_token->bytes, details->xbl_token->size));
    json_object_object_add(properties, "UserTokens", user_token);

    json_object_object_add(request, "Properties", properties);
    json_object_object_add(request, "RelyingParty", json_object_new_string("rp://api.minecraftservices.com/"));
    json_object_object_add(request, "TokenType", json_object_new_string("JWT"));

    CURL *curl;

    curl = curl_easy_init();
    if (curl){
        curl_easy_setopt(curl, CURLOPT_URL, "https://xsts.auth.xboxlive.com/xsts/authorize");

        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);

        size_t json_len;
        const char *request_string = json_object_to_json_string_length(request, 0, &json_len);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_string);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, json_len);



        struct curl_slist *headers = curl_slist_append(NULL, "Content-Type: application/json");
        headers = curl_slist_append(headers, "Accept: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        NetworkBuffer *response = buffer_new();
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &save_response);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);

        cmc_log(INFO, "Sending: %s", request_string);

        CURLcode res = curl_easy_perform(curl);

        json_object *json_response = json_tokener_parse((char *) response->bytes);
        cmc_log(INFO, "XSTS token %s", json_object_get_string(json_response));
        json_object *json_token = json_object_object_get(json_response, "Token");
        buffer_write_little_endian(details->xsts_token, json_object_get_string(json_token), json_object_get_string_len(json_token));
        json_object *display_claims = json_object_object_get(json_response, "DisplayClaims");
        json_object *xui = json_object_object_get(display_claims, "xui");
        json_object *hash = json_object_array_get_idx(xui, 0);
        json_object *hash_gotten = json_object_object_get(hash, "uhs");
        buffer_write_little_endian(details->player_hash, json_object_get_string(hash_gotten), json_object_get_string_len(hash_gotten));

        json_object_put(request);
        json_object_put(json_response);
    }
}

void authenticate_minecraft(AuthenticationDetails *details) {
    json_object *request = json_object_new_object();
    NetworkBuffer *identity = buffer_new();
    char *start = "XBL3.0 x=";
    char *in_between = ";";

    buffer_write_little_endian(identity, start, strlen(start));
    buffer_write_little_endian(identity, details->player_hash->bytes, details->player_hash->size);
    buffer_write_little_endian(identity, in_between, strlen(in_between));
    buffer_write_little_endian(identity, details->xsts_token->bytes, details->xsts_token->size);
    json_object_object_add(request, "identityToken", json_object_new_string_len(identity->bytes, (int) identity->size));

    CURL *curl;

    curl = curl_easy_init();
    if (curl){
        curl_easy_setopt(curl, CURLOPT_URL, "https://api.minecraftservices.com/authentication/login_with_xbox");

        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);

        size_t json_len;
        const char *request_string = json_object_to_json_string_length(request, 0, &json_len);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_string);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, json_len);



        struct curl_slist *headers = curl_slist_append(NULL, "Content-Type: application/json");
        headers = curl_slist_append(headers, "Accept: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        NetworkBuffer *response = buffer_new();
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &save_response);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);

        cmc_log(DEBUG, "Sending: %s", request_string);

        CURLcode res = curl_easy_perform(curl);

        json_tokener *tokener = json_tokener_new();
        json_object *json_response = json_tokener_parse_ex(tokener, (char *) response->bytes, (int) response->size);

        json_object *json_token = json_object_object_get(json_response, "access_token");
        buffer_write_little_endian(details->mc_token,
                                   json_object_get_string(json_token),
                                   json_object_get_string_len(json_token)
       );

        json_object_put(request);
        json_object_put(json_response);
    }
}

void authenticate_server() {

}

void get_player_profile(AuthenticationDetails *details) {
    CURL *curl;

    curl = curl_easy_init();
    if (curl){
        curl_easy_setopt(curl, CURLOPT_URL, "https://api.minecraftservices.com/minecraft/profile");

        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);


        struct curl_slist *headers = curl_slist_append(NULL, "Accept: application/json");
        char *auth_header = malloc(5012);
        char *start = "Authorization: Bearer ";
        strcpy(auth_header, start);
        memmove(auth_header + strlen(start), details->mc_token->bytes, details->mc_token->size);
        memmove(auth_header + strlen(start) + details->mc_token->size, "\0", 1);

        headers = curl_slist_append(headers, auth_header);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        NetworkBuffer *response = buffer_new();
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &save_response);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);

        cmc_log(INFO, "Sending: %s", auth_header);

        CURLcode res = curl_easy_perform(curl);

        json_object *json_response = json_tokener_parse((char *) response->bytes);
        cmc_log(DEBUG, "Player profile response %s", json_object_get_string(json_response));
        json_object *name = json_object_object_get(json_response, "name");
        cmc_log(INFO, "Player Name %s", json_object_get_string(name));
        json_object_put(json_response);
    }
}

AuthenticationDetails *authenticate() {
    AuthenticationDetails *details = authentication_details_new();
    auth_token_get(details);
    if (details->ms_access_token->size == 0) {
        cmc_log(INFO, "No XBox live token saved, requesting new one ...");
        authenticate_xbl(details);
    }

    poll_xbl_access(details);
    authenticate_xsts(details);
    authenticate_minecraft(details);

    get_player_profile(details);

    return details;
}