//
// Created by hamish on 9/6/20.
//

#include "network.h"
#include "unity.h"
#include <pthread.h>
#include <unistd.h>


#define PORT_SERVER_TO_CLIENT 32263
#define PORT_CLIENT_TO_SERVER 32262

#define TCP_PORT 32264

void setUp(void) { int b = 2; }

void tearDown(void) { int a = 1; }

void test_UDP_server_context(void) {
    char destination[] = "127.0.0.1";
    SocketContext context;
    int result = CreateUDPServerContextStun(&context, PORT_SERVER_TO_CLIENT, 10, 500);
    TEST_ASSERT_EQUAL_INT(0, result);
}

void test_UDP_client_context(void) {
    char destination[] = "127.0.0.1";
    SocketContext context;
    int result =
        CreateUDPClientContextStun(&context, destination, PORT_CLIENT_TO_SERVER, 10, 500);
    TEST_ASSERT_EQUAL_INT(0, result);
}

void test_TCP_server_context_no_client(void){
    SocketContext context;
    int result = CreateTCPServerContextStun(&context, TCP_PORT, 500, 500);
// should fail as no client will be trying to connect at this point.
    TEST_ASSERT_EQUAL_INT(-1, result);
}

void * server_TCP_loop(void *args){
    SocketContext context;
    while(1){
        int result = CreateTCPServerContextStun(&context, TCP_PORT, 500, 500);
        sleep(2);

        if (result >= 0){
            Ack(&context);
            break;
        }
    }
}

void test_TCP_client_context(void){
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, server_TCP_loop, NULL);
    SocketContext context;
    char destination[] = "127.0.0.1";
    sleep(1);
    CreateTCPClientContextStun(&context, destination, TCP_PORT, 500,  500);
    pthread_join(thread_id, NULL);

}



int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_UDP_server_context);
    sleep(1);
    RUN_TEST(test_UDP_client_context);
    RUN_TEST(test_TCP_server_context_no_client);
    RUN_TEST(test_TCP_client_context);
    return UNITY_END();
}