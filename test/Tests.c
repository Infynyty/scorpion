#include "munit.h"
#include "MCVarInt.h"
#include "Packets.h"
#include "Logger.h"

MunitResult test_varint_basic(const MunitParameter params[], void* user_data_or_fixture) {
    MCVarInt *varint = varint_encode(127);
    int32_t varint_dec = varint_decode(varint->bytes);
    munit_assert_int32(127, ==, varint_dec);
    free(varint);

    varint = varint_encode(-1);
    varint_dec = varint_decode(varint->bytes);
    munit_assert_int32(-1, ==, varint_dec);
    free(varint);

    int32_t random = munit_rand_int_range(INT32_MIN, INT32_MAX);
    varint = varint_encode(random);
    varint_dec = varint_decode(varint->bytes);
    munit_assert_int32(random, ==, varint_dec);
    free(varint);
    return MUNIT_OK;
}

MunitResult test_packet_decoding(const MunitParameter params[], void* user_data_or_fixture) {
    NetworkBuffer *mock_packet = buffer_new();
    MCVarInt *varInt = varint_encode(256);
    buffer_write(mock_packet, varInt->bytes, varInt->size);
    SetCompressionPacket packet = {._header = set_compression_header_new()};
    packet_decode(&packet._header, mock_packet);

    void *ptr = (&(packet._header)) + 1;
    sc_log(INFO, "Should point to %p", ptr);
    MCVarInt **var_ptr = ptr;
    sc_log(INFO, "Memory at %p: %p", ptr,  (*var_ptr));


    int32_t id = varint_decode(packet._header->packet_id->bytes);
    munit_assert_int8(id, ==, 0x03);


    munit_assert_int8(packet.threshold->size, ==, 2);

    int32_t threshold = varint_decode(packet.threshold->bytes);
    munit_assert_int32(threshold, ==, 256);

    packet_free(&packet._header);
    free(varInt);
    buffer_free(mock_packet);
    return MUNIT_OK;
}

static MunitTest test_suite_tests[] = {
        {
                "VarInt",
                test_varint_basic,
                NULL,
                NULL,
                MUNIT_TEST_OPTION_NONE,
                NULL
        },
        {
                "PacketDecoder",
                test_packet_decoding,
                NULL,
                NULL,
                MUNIT_TEST_OPTION_NONE,
                NULL
        },



        { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};

const MunitSuite test_suite = {
        (char*) "Util/",
        test_suite_tests,
        NULL,
        1,
        MUNIT_SUITE_OPTION_NONE
};

int run_tests() {
    return munit_suite_main(&test_suite, NULL, 0, NULL);
}