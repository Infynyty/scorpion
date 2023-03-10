#include "munit.h"
#include "MCVarInt.h"

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

static MunitTest test_suite_tests[] = {
        {
                "VarInt",
                test_varint_basic,
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

void run_tests() {
    munit_suite_main(&test_suite, NULL, 0, NULL);
}