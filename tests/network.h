#ifndef STUN_SERVER_TEST_NETWORK_H
#define STUN_SERVER_TEST_NETWORK_H
/**
 * Copyright Fractal Computers, Inc. 2020
 * @file network.h
 * @brief A stripped down version of the protocol network.h to allow for STUN
 *        testing
============================
Usage
============================

Use these functions to create UDP and TCP ports to listen for connections.
*/

/*
============================
Includes
============================
*/

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stdbool.h>
#include <sys/socket.h>

/*
============================
Defines
============================
*/

#define SOCKET int
#define closesocket close
#define FRACTAL_IOCTL_SOCKET ioctl
#define FRACTAL_CLOSE_SOCKET close
#define FRACTAL_ETIMEDOUT ETIMEDOUT
#define FRACTAL_EWOULDBLOCK EWOULDBLOCK
#define FRACTAL_EAGAIN EAGAIN
#define FRACTAL_EINPROGRESS EINPROGRESS

#define STUN_IP "127.0.0.1"
#define STUN_PORT 48800

/*
============================
Custom Types
============================
*/

typedef struct {
    unsigned int ip;
    unsigned short private_port;
    unsigned short public_port;
} stun_entry_t;

typedef enum stun_request_type { ASK_INFO, POST_INFO } stun_request_type_t;

typedef struct {
    stun_request_type_t type;
    stun_entry_t entry;
} stun_request_t;

typedef struct SocketContext {
    bool is_server;
    bool is_tcp;
    SOCKET s;
    struct sockaddr_in addr;
    int ack;
} SocketContext;

/*
============================
Public Functions
============================
*/

int CreateUDPClientContextStun(SocketContext* context, char* destination,
                               int port, int recvfrom_timeout_ms,
                               int stun_timeout_ms);

int CreateUDPServerContextStun(SocketContext* context, int port,
                               int recvfrom_timeout_ms, int stun_timeout_ms);

int CreateTCPClientContextStun(SocketContext* context, char* destination,
                               int port, int recvfrom_timeout_ms,
                               int stun_timeout_ms);

int CreateTCPServerContextStun(SocketContext* context, int port,
                               int recvfrom_timeout_ms, int stun_timeout_ms);

int Ack(SocketContext* context);

#endif  // STUN_SERVER_TEST_NETWORK_H
