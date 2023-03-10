#include "munit.h"
#include "MCVarInt.h"



MunitResult my_test(const MunitParameter params[], void* user_data_or_fixture) {
    MCVarInt *varint = varint_encode(127);
    int32_t varint_dec = varint_decode(varint->bytes);
    munit_assert_int32(127, ==, varint_dec);

    return MUNIT_OK;
}

static MunitTest test_suite_tests[] = {
        {
            "test",
            my_test,
            NULL,
            NULL,
            MUNIT_TEST_OPTION_NONE,
            NULL
        }
};

const MunitSuite test_suite = {
        (char*) "first",
        test_suite_tests,
        NULL,
        1,
        MUNIT_SUITE_OPTION_NONE
};

void run_tests() {
    munit_suite_main(&test_suite, NULL, 0, NULL);
}