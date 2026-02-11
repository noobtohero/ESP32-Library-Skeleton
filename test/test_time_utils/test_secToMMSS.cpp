#include <unity.h>

#include "utils/lcd/lcd_utils.h"

static void test_secToMMSS_basic()
{
    char out[6] = {0};
    secToMMSS(0, out, sizeof(out));
    TEST_ASSERT_EQUAL_STRING("00:00", out);

    secToMMSS(61, out, sizeof(out));
    TEST_ASSERT_EQUAL_STRING("01:01", out);

    secToMMSS(3599, out, sizeof(out));
    TEST_ASSERT_EQUAL_STRING("59:59", out);

    secToMMSS(3600, out, sizeof(out));
    TEST_ASSERT_EQUAL_STRING("60:00", out);
}

static void test_secToMMSS_invalid_output()
{
    char out[5] = {'x', 'x', 'x', 'x', '\0'};
    secToMMSS(10, out, sizeof(out));
    TEST_ASSERT_EQUAL_STRING("xxxx", out);

    secToMMSS(10, NULL, 6);
}

int main(int argc, char **argv)
{
    UNITY_BEGIN();
    RUN_TEST(test_secToMMSS_basic);
    RUN_TEST(test_secToMMSS_invalid_output);
    return UNITY_END();
}
