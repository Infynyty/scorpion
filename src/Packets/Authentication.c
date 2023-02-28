#include "Authentication.h"
#include "NetworkBuffer.h"
#include "Logger.h"
#include "Packets.h"
#include <curl/curl.h>
#include <stdlib.h>
#include <json-c/json.h>
#include <unistd.h>
#include <openssl/bn.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <ctype.h>

#define MAX_TOKEN_LENGTH 2048

size_t save_response(char *ptr, size_t size, size_t nmemb, void *userdata) {
    NetworkBuffer *buffer = userdata;
    buffer_write(buffer, ptr, size * nmemb);
    return size * nmemb;
}

void auth_token_get(AuthenticationDetails *details) {
    FILE *file = fopen(".token", "rb");
    if (file) {
        char *token = malloc(MAX_TOKEN_LENGTH);
        size_t length = 0;
        fread((char **) &length, sizeof(size_t), 1, file);
        fread(token, sizeof(char), length, file);
        buffer_write(details->ms_access_token, token, length);
        fclose(file);
    }
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

        cmc_log(DEBUG, "Requesting XBL token with the following payload: %s", request_string);

        CURLcode res = curl_easy_perform(curl);

        json_object *json_response = json_tokener_parse((char *) response->bytes);
        cmc_log(DEBUG, "Received the following response for the XBL token request: %s", json_object_get_string(json_response));
        json_object *json_token = json_object_object_get(json_response, "Token");
        buffer_write(details->xbl_token, json_object_get_string(json_token), json_object_get_string_len(json_token));
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
            cmc_log(DEBUG, "Received Microsoft access token: %s", json_object_get_string(json_token));
            buffer_write(details->ms_access_token, json_object_get_string(json_token),
                         json_object_get_string_len(json_token));
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

        cmc_log(DEBUG, "Received a device code %s", json_object_get_string(device_code));

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

        cmc_log(DEBUG, "Requesting XSTS token with the following payload: %s", request_string);

        CURLcode res = curl_easy_perform(curl);

        json_object *json_response = json_tokener_parse((char *) response->bytes);
        cmc_log(DEBUG, "Received XSTS token: %s", json_object_get_string(json_response));
        json_object *json_token = json_object_object_get(json_response, "Token");
        buffer_write(details->xsts_token, json_object_get_string(json_token), json_object_get_string_len(json_token));
        json_object *display_claims = json_object_object_get(json_response, "DisplayClaims");
        json_object *xui = json_object_object_get(display_claims, "xui");
        json_object *hash = json_object_array_get_idx(xui, 0);
        json_object *hash_gotten = json_object_object_get(hash, "uhs");
        buffer_write(details->player_hash, json_object_get_string(hash_gotten), json_object_get_string_len(hash_gotten));

        json_object_put(request);
        json_object_put(json_response);
    }
}

void authenticate_minecraft(AuthenticationDetails *details) {
    json_object *request = json_object_new_object();
    NetworkBuffer *identity = buffer_new();
    char *start = "XBL3.0 x=";
    char *in_between = ";";

    buffer_write(identity, start, strlen(start));
    buffer_write(identity, details->player_hash->bytes, details->player_hash->size);
    buffer_write(identity, in_between, strlen(in_between));
    buffer_write(identity, details->xsts_token->bytes, details->xsts_token->size);
    json_object_object_add(request, "identityToken", json_object_new_string_len(identity->bytes, (int) identity->size));

    CURL *curl;

    curl = curl_easy_init();
    if (curl){
        curl_easy_setopt(curl, CURLOPT_URL, "https://api.minecraftservices.com/authentication/login_with_xbox");


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

        cmc_log(DEBUG, "Requesting Minecraft access token with the following payload: %s", request_string);

        CURLcode res = curl_easy_perform(curl);

        json_tokener *tokener = json_tokener_new();
        json_object *json_response = json_tokener_parse_ex(tokener, (char *) response->bytes, (int) response->size);

        cmc_log(DEBUG, "Received a Minecraft access token with the following response: %s", json_object_get_string(json_response));


        json_object *json_token = json_object_object_get(json_response, "access_token");
        buffer_write(details->mc_token,
                     json_object_get_string(json_token),
                     json_object_get_string_len(json_token)
        );

        json_object_put(request);
        json_object_put(json_response);
    }
}

void authenticate_server(EncryptionRequestPacket *packet, NetworkBuffer *unencrypted_secret, ClientState *client) {
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha1(), NULL);
    EVP_DigestUpdate(ctx, packet->server_id->bytes, packet->server_id->size);
    EVP_DigestUpdate(ctx, unencrypted_secret->bytes, unencrypted_secret->size);
    EVP_DigestUpdate(ctx, packet->public_key->bytes, packet->public_key->size);
    unsigned char *temp = malloc(128);
    unsigned int length;
    EVP_DigestFinal(ctx, temp, &length);
    BIGNUM *num = BN_bin2bn(temp, length, NULL);

    char *result = malloc(128);
    bool negative = false;
    if (BN_is_bit_set(num, 159))
    {
        negative = true;
        strcpy(result, "-");
        unsigned char *temp2 = malloc(BN_num_bytes(num));
        BN_bn2bin(num, temp2);
        for (int i = 0; i < BN_num_bytes(num); i++) {
            temp2[i] = ~temp2[i];
        }
        BN_bin2bn(temp2, BN_num_bytes(num), num);
        free(temp2);

        BN_add_word(num, 1);
    }
    char *hexdigest = BN_bn2hex(num);
    for(int i = 0; hexdigest[i]; i++){
        hexdigest[i] = tolower(hexdigest[i]);
    }
    while (*hexdigest == '0') {
        hexdigest++;
    }
    negative ? strcat(result, hexdigest) : strcpy(result, hexdigest);
    OPENSSL_free(hexdigest);
    BN_free(num);
    free(temp);


    json_object *request = json_object_new_object();
    json_object_object_add(request, "accessToken", json_object_new_string_len(
            client->auth_details->mc_token->bytes,
            client->auth_details->mc_token->size
            ));
    json_object_object_add(request, "selectedProfile", json_object_new_string_len(
            client->profile_info->uuid->bytes,
            32
            ));
    json_object_object_add(request, "serverId", json_object_new_string(result));

    CURL *curl;

    curl = curl_easy_init();
    if (curl){
        curl_easy_setopt(curl, CURLOPT_URL, "https://sessionserver.mojang.com/session/minecraft/join");

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

        cmc_log(DEBUG, "Requesting to join server with the following payload: %s", request_string);

        CURLcode res = curl_easy_perform(curl);

        json_object *json_response = json_tokener_parse((char *) response->bytes);
        cmc_log(DEBUG, "Received server auth response: %s", json_object_get_string(json_response));
    }
}

void get_player_profile(AuthenticationDetails *details, ClientState *state) {
    CURL *curl;

    curl = curl_easy_init();
    if (curl){
        curl_easy_setopt(curl, CURLOPT_URL, "https://api.minecraftservices.com/minecraft/profile");

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

        cmc_log(DEBUG, "Request profile information with the following payload: %s", auth_header);

        CURLcode res = curl_easy_perform(curl);

        json_object *json_response = json_tokener_parse((char *) response->bytes);
        cmc_log(DEBUG, "Received player profile information: %s", json_object_get_string(json_response));
        json_object *name = json_object_object_get(json_response, "name");
        NetworkBuffer *player_name = string_buffer_new(json_object_get_string(name));
        json_object *uuid = json_object_object_get(json_response, "id");
        NetworkBuffer *player_uuid = string_buffer_new(json_object_get_string(uuid));
        state->profile_info->uuid = player_uuid;
        state->profile_info->name = player_name;
        json_object_put(json_response);
    }
}

void authenticate(ClientState *state) {
    cmc_log(INFO, "Authenticating Minecraft account...");

    auth_token_get(state->auth_details);
    if (state->auth_details->ms_access_token->size == 0) {
        cmc_log(INFO, "No XBox live token saved, requesting new one ...");
        authenticate_xbl(state->auth_details);
    } else {
        cmc_log(INFO, "Using saved XBL token...");
    }

    poll_xbl_access(state->auth_details);
    authenticate_xsts(state->auth_details);
    authenticate_minecraft(state->auth_details);

    cmc_log(INFO, "Successfully authenticated with Minecraft.");

    get_player_profile(state->auth_details, state);

    cmc_log(INFO, "Successfully received player data for player %s.", state->profile_info->name->bytes);
}