#include <openssl/rsa.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include "NetworkBuffer.h"
#include "Packets.h"



NetworkBuffer *encrypt_rsa(NetworkBuffer *data, NetworkBuffer *key) {
    RSA *rsa = d2i_RSA_PUBKEY(NULL, key->bytes, key->size);
    unsigned char *temp = malloc(128);
    RSA_public_encrypt(data->size, data->bytes, temp, rsa, RSA_PKCS1_PADDING);
    NetworkBuffer *encrypted = buffer_new();
    buffer_write_little_endian(encrypted, temp, 128);
    RSA_free(rsa);
    return encrypted;
}

void generate_secret(NetworkBuffer *dest) {
    char *temp = malloc(16);
    RAND_priv_bytes(temp, 16);
    buffer_write_little_endian(dest, temp, 16);
}

EncryptionResponsePacket *encryption_response_generate(
        NetworkBuffer *public_key,
        NetworkBuffer *verify_token,
        NetworkBuffer *secret
        ) {
    generate_secret(secret);
    NetworkBuffer *shared_secret = buffer_new();
    buffer_write_little_endian(shared_secret, secret->bytes, 16);

    shared_secret = encrypt_rsa(shared_secret, public_key);
    verify_token = encrypt_rsa(verify_token, public_key);

    return encryption_response_packet_new(shared_secret, true, verify_token, 0, NULL);
}

