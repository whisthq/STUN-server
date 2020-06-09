//
// Created by hamish on 9/6/20.
//

#include "network.h"
#include "unity.h"

void setUp(void) { int b = 2; }

void tearDown(void) { int a = 1; }

void test_server_context(void) {
    char destination[] = "127.0.0.1";
    SocketContext context;
    int result = CreateUDPServerContextStun(&context, 32262, 10, 500);
    TEST_ASSERT_EQUAL_INT(0, result);
}

void test_client_context(void) {
    char destination[] = "127.0.0.1";
    SocketContext context;
    int result =
        CreateUDPClientContextStun(&context, destination, 32262, 10, 500);
    TEST_ASSERT_EQUAL_INT(0, result);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_server_context);
    sleep(3);
    RUN_TEST(test_client_context);
    return UNITY_END();
}