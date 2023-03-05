#include <openssl/rsa.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/sha.h>
#include "NetworkBuffer.h"
#include "Packets.h"
#include "Logger.h"
#include <openssl/err.h>
#include <openssl/aes.h>
#include <openssl/bn.h>
#include "curl/curl.h"


void authenticateAccount() {

}
void authenticateJoin(NetworkBuffer *secret) {
    CURL *curl;
    CURLcode res;

    curl = curl_easy_init();
    if (curl){
        curl_easy_setopt(curl, CURLOPT_URL, "https://sessionserver.mojang.com/session/minecraft/join");
    }

    curl_easy_cleanup(curl);
}

NetworkBuffer *encrypt_rsa(NetworkBuffer *data, NetworkBuffer *key) {
	unsigned char *key_copy = malloc(key->size);
	memmove(key_copy, key->bytes, key->size);
	RSA *rsa = d2i_RSA_PUBKEY(NULL, &key_copy, key->size);
	unsigned char *temp = malloc(RSA_size(rsa));
	int bytes_encrypted = RSA_public_encrypt(data->size, data->bytes, temp, rsa, RSA_PKCS1_PADDING);
	NetworkBuffer *encrypted = buffer_new();
    buffer_write(encrypted, temp, bytes_encrypted);
	free(temp);
	RSA_free(rsa);
	return encrypted;
}

void generate_secret(NetworkBuffer *dest) {
	unsigned char *temp = malloc(AES_BLOCK_SIZE);
	RAND_priv_bytes(temp, AES_BLOCK_SIZE);
    buffer_write(dest, temp, AES_BLOCK_SIZE);
    free(temp);
}

void generate_hash(NetworkBuffer *secret, NetworkBuffer *public_key) {
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha1(), NULL);
    EVP_DigestUpdate(ctx, "", 1);
    EVP_DigestUpdate(ctx, secret->bytes, secret->size);
    EVP_DigestUpdate(ctx, public_key->bytes, public_key->size);
    unsigned char *buffer = malloc(20);
    EVP_DigestFinal(ctx, buffer, NULL);
    BIGNUM *bn = BN_bin2bn(buffer, 20, NULL);
}

void encryption_response_generate(
        EncryptionResponsePacket *packet,
        NetworkBuffer *public_key,
        NetworkBuffer *verify_token,
        NetworkBuffer *secret
        ) {
	generate_secret(secret);
	NetworkBuffer *shared_secret = buffer_new();
    buffer_write(shared_secret, secret->bytes, AES_BLOCK_SIZE);

    unsigned char *key_copy = malloc(public_key->size);
    unsigned char *temp = key_copy;
    memmove(key_copy, public_key->bytes, public_key->size);


    RSA *rsa = d2i_RSA_PUBKEY(NULL, &key_copy, public_key->size);

    unsigned char *encrypted_secret = malloc(RSA_size(rsa));
    int bytes_encrypted = RSA_public_encrypt(shared_secret->size, shared_secret->bytes, encrypted_secret, rsa, RSA_PKCS1_PADDING);
    NetworkBuffer *encrypted_secret_buf = buffer_new();
    buffer_write(encrypted_secret_buf, encrypted_secret, bytes_encrypted);

    unsigned char *encrypted_nonce = malloc(RSA_size(rsa));
    bytes_encrypted = RSA_public_encrypt(verify_token->size, verify_token->bytes, encrypted_nonce, rsa, RSA_PKCS1_PADDING);
    NetworkBuffer *encrypted_nonce_buf = buffer_new();
    buffer_write(encrypted_nonce_buf, encrypted_nonce, bytes_encrypted);

    shared_secret = encrypted_secret_buf;
    verify_token = encrypted_nonce_buf;

    free(encrypted_secret);
    free(encrypted_nonce);
    RSA_free(rsa);
    free(temp);

    packet->shared_secret = shared_secret;
    packet->verify_token = verify_token;
}

