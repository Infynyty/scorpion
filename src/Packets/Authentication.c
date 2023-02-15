#include "Authentication.h"
#include "NetworkBuffer.h"
#include "Logger.h"
#include <curl/curl.h>


NetworkBuffer *authenticate_xbl() {
    CURL *curl;
    CURLcode res;
    char *buffer = malloc(500);
    size_t received_bytes;

    int i = 0;
    while (curl_version_info(CURLVERSION_NOW)->feature_names[i] != NULL) {
        cmc_log(INFO, "SSL: %s", curl_version_info(CURLVERSION_NOW)->feature_names[i]);
        i++;
    }


    curl = curl_easy_init();
    if (curl){

        char *data = "scope=service::user.auth.xboxlive.com::MBI_SSL&client_id=00000000441cc96b&response_type=device_code";
        curl_easy_setopt(curl, CURLOPT_URL, "https://login.live.com/oauth20_connect.srf");

        CURLcode is = curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        if (is != CURLE_OK) {
            cmc_log(ERR, "Not supported");
        }
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);


        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(data));

        res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            curl_easy_recv(curl, buffer, 500, &received_bytes);
            cmc_log(INFO, "Response: %s", buffer);
        } else {
            cmc_log(ERR, "Error code: %d", res);
        }

    }


    curl_easy_cleanup(curl);

    return NULL;
}

NetworkBuffer *authenticate_xsts() {

}

NetworkBuffer *authenticate_minecraft() {

}

void authenticate_server() {

}