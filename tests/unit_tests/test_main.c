#include "unity/unity.h"

void test_init_queue(void);

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_init_queue);

    return UNITY_END();
}