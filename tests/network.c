/***
 * @file network.c
 * @author Hamish Nicholson
 * @brief a stripped down version of the protocol network.c to allow for stun
 * testing.
 * Copyright Fractal Computers, Inc. 2020
 */
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
//#include <cstdarg>

#include "clock.h"
#include "network.h"
#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) < (b)) ? (a) : (b))

FILE *log_file = NULL;

void flog(const char *fmt, ...) {
    if (!log_file) {
        log_file = fopen("flog.txt", "a");
    }
    char *time_string = FractalCurrentTimeStr();

    printf("%s | ", time_string);
    fprintf(log_file, "%s | ", time_string);

    va_list args;
    va_start(args, fmt);

    vprintf(fmt, args);
    va_start(args, fmt);
    vfprintf(log_file, fmt, args);
    fflush(log_file);

    fseek(log_file, 0, SEEK_END);
    int sz = ftell(log_file);
    if (sz > 5 * 1024 * 1024) {
        printf("Moving flog.txt to old_flog.txt!\n");
        fclose(log_file);
        system("mv flog.txt old_flog.txt");
        log_file = fopen("flog.txt", "a");
    }
}

int GetLastNetworkError() { return errno; }

void set_timeout(SOCKET s, int timeout_ms) {
    // Sets the timeout for SOCKET s to be timeout_ms in milliseconds
    // Any recv calls will wait this long before timing out
    // -1 means that it will block indefinitely until a packet is received
    // 0 means that it will immediately return with whatever data is waiting in
    // the buffer
    if (timeout_ms < 0) {
        flog(
            "WARNING: This socket will blocking indefinitely. You will not be "
            "able to recover if a packet is never received");
        unsigned long mode = 0;

        FRACTAL_IOCTL_SOCKET(s, FIONBIO, &mode);

    } else if (timeout_ms == 0) {
        unsigned long mode = 1;
        FRACTAL_IOCTL_SOCKET(s, FIONBIO, &mode);
    } else {
        // Set to blocking when setting a timeout
        unsigned long mode = 0;
        FRACTAL_IOCTL_SOCKET(s, FIONBIO, &mode);

        fractal_clock_t read_timeout = FractalCreateClock(timeout_ms);

        if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char *)&read_timeout,
                       sizeof(read_timeout)) < 0) {
            flog("Failed to set timeout");
            return;
        }
    }
}

int recvp(SocketContext *context, void *buf, int len) {
    if (context == NULL) {
        flog("Context is NULL");
        return -1;
    }
    return recv(context->s, buf, len, 0);
}

int sendp(SocketContext *context, void *buf, int len) {
    if (context == NULL) {
        flog("Context is NULL");
        return -1;
    }
    if (len != 0 && buf == NULL) {
        flog("Passed non zero length and a NULL pointer to sendto");
        return -1;
    }
    // cppcheck-suppress nullPointer
    return sendto(context->s, buf, len, 0, (struct sockaddr *)(&context->addr),
                  sizeof(context->addr));
}

int Ack(SocketContext *context) { return sendp(context, NULL, 0); }

int CreateUDPClientContextStun(SocketContext *context, char *destination,
                               int port, int recvfrom_timeout_ms,
                               int stun_timeout_ms) {
    context->is_tcp = false;
    flog("CreateUDPClientContextStun \n");

    // Create UDP socket
    context->s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (context->s <= 0) {  // Windows & Unix cases
        flog("Could not create UDP socket %d\n", GetLastNetworkError());
        return -1;
    }
    set_timeout(context->s, stun_timeout_ms);

    // Client connection protocol
    context->is_server = false;

    struct sockaddr_in stun_addr;
    stun_addr.sin_family = AF_INET;
    stun_addr.sin_addr.s_addr = inet_addr(STUN_IP);
    stun_addr.sin_port = htons(STUN_PORT);

    stun_request_t stun_request;
    stun_request.type = ASK_INFO;
    stun_request.entry.ip = inet_addr(destination);
    stun_request.entry.public_port = htons((unsigned short)port);

    flog("Sending info request to STUN...");
    if (sendto(context->s, (const char *)&stun_request, sizeof(stun_request), 0,
               (struct sockaddr *)&stun_addr, sizeof(stun_addr)) < 0) {
        flog("Could not send message to STUN %d\n", GetLastNetworkError());
        closesocket(context->s);
        return -1;
    }

    stun_entry_t entry = {0};
    int recv_size;
    if ((recv_size = recvp(context, &entry, sizeof(entry))) < 0) {
        flog("Could not receive message from STUN %d\n", GetLastNetworkError());
        closesocket(context->s);
        return -1;
    }

    if (recv_size != sizeof(entry)) {
        flog("STUN Response of incorrect size!");
        closesocket(context->s);
        return -1;
    } else if (entry.ip != stun_request.entry.ip ||
               entry.public_port != stun_request.entry.public_port) {
        flog("STUN Response IP and/or Public Port is incorrect!");
        closesocket(context->s);
        return -1;
    } else {
        flog("Received STUN response! Public %d is mapped to private %d\n",
             ntohs((unsigned short)entry.public_port),
             ntohs((unsigned short)entry.private_port));
        context->addr.sin_family = AF_INET;
        context->addr.sin_addr.s_addr = entry.ip;
        context->addr.sin_port = entry.private_port;
    }

    flog("Connected to server on %s:%d! (Private %d)\n",
         inet_ntoa(context->addr.sin_addr), port,
         ntohs(context->addr.sin_port));
    set_timeout(context->s, recvfrom_timeout_ms);

    return 0;
}

int CreateUDPServerContextStun(SocketContext *context, int port,
                               int recvfrom_timeout_ms, int stun_timeout_ms) {
    flog("CreateUDPServerContextStun \n");
    context->is_tcp = false;

    // Create UDP socket
    context->s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (context->s <= 0) {  // Windows & Unix cases
        flog("Could not create UDP socket %d\n", GetLastNetworkError());
        return -1;
    }
    set_timeout(context->s, stun_timeout_ms);

    // Server connection protocol
    context->is_server = true;

    // Tell the STUN to log our requested virtual port
    struct sockaddr_in stun_addr;
    stun_addr.sin_family = AF_INET;
    stun_addr.sin_addr.s_addr = inet_addr(STUN_IP);
    stun_addr.sin_port = htons(STUN_PORT);

    stun_request_t stun_request;
    stun_request.type = POST_INFO;
    stun_request.entry.public_port = htons((unsigned short)port);

    flog("Sending stun entry to STUN...");
    if (sendto(context->s, (const char *)&stun_request, sizeof(stun_request), 0,
               (struct sockaddr *)&stun_addr, sizeof(stun_addr)) < 0) {
        flog("Could not send message to STUN %d\n", GetLastNetworkError());
        closesocket(context->s);
        return -1;
    }

    return 0;
}

bool tcp_connect(SOCKET s, struct sockaddr_in addr, int timeout_ms) {
    // Connect to TCP server
    int ret;
    set_timeout(s, 0);
    if ((ret = connect(s, (struct sockaddr *)(&addr), sizeof(addr))) < 0) {
        bool worked = GetLastNetworkError() == FRACTAL_EINPROGRESS;

        if (!worked) {
            flog(
                "Could not connect() over TCP to server: Returned %d, Error "
                "Code %d\n",
                ret, GetLastNetworkError());
            closesocket(s);
            return false;
        }
    }

    // Select connection
    fd_set set;
    FD_ZERO(&set);
    FD_SET(s, &set);
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    if ((ret = select((int)s + 1, NULL, &set, NULL, &tv)) <= 0) {
        flog(
            "Could not select() over TCP to server: Returned %d, Error Code "
            "%d\n",
            ret, GetLastNetworkError());
        closesocket(s);
        return false;
    }

    set_timeout(s, timeout_ms);
    return true;
}

int CreateTCPClientContextStun(SocketContext *context, char *destination,
                               int port, int recvfrom_timeout_ms,
                               int stun_timeout_ms) {
    if (context == NULL) {
        flog("Context is NULL");
        return -1;
    }
    if (destination == NULL) {
        flog("destiniation is NULL");
        return -1;
    }
    context->is_tcp = true;

    // Init stun_addr
    struct sockaddr_in stun_addr;
    stun_addr.sin_family = AF_INET;
    stun_addr.sin_addr.s_addr = inet_addr(STUN_IP);
    stun_addr.sin_port = htons(STUN_PORT);
    int opt;

    // Create TCP socket
    context->s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (context->s <= 0) {  // Windows & Unix cases
        flog("Could not create UDP socket %d\n", GetLastNetworkError());
        return -1;
    }
    set_timeout(context->s, stun_timeout_ms);

    // Tell the STUN to use TCP
    SOCKET udp_s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    // cppcheck-suppress nullPointer
    sendto(udp_s, NULL, 0, 0, (struct sockaddr *)&stun_addr, sizeof(stun_addr));
    closesocket(udp_s);
    // Client connection protocol
    context->is_server = false;

    // Reuse addr
    opt = 1;
    if (setsockopt(context->s, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt,
                   sizeof(opt)) < 0) {
        flog("Could not setsockopt SO_REUSEADDR");
        return -1;
    }

    // Connect to STUN server
    if (!tcp_connect(context->s, stun_addr, stun_timeout_ms)) {
        flog("Could not connect to STUN Server over TCP");
        return -1;
    }

    struct sockaddr_in origin_addr;
    socklen_t slen = sizeof(origin_addr);
    if (getsockname(context->s, (struct sockaddr *)&origin_addr, &slen) < 0) {
        flog("Could not get sock name");
        closesocket(context->s);
        return -1;
    }

    // Make STUN request
    stun_request_t stun_request;
    stun_request.type = ASK_INFO;
    stun_request.entry.ip = inet_addr(destination);
    stun_request.entry.public_port = htons((unsigned short)port);

    if (sendp(context, &stun_request, sizeof(stun_request)) < 0) {
        flog("Could not send STUN request to connected STUN server!");
        closesocket(context->s);
        return -1;
    }

    // Receive STUN response
    fractal_clock_t t;
    FractalStartTimer(&t);

    int recv_size = 0;
    stun_entry_t entry = {0};

    while (recv_size < (int)sizeof(entry) &&
           FractalGetTimer(t) < stun_timeout_ms) {
        int single_recv_size;
        if ((single_recv_size = recvp(context, ((char *)&entry) + recv_size,
                                      max(0, (int)sizeof(entry) - recv_size))) <
            0) {
            flog("Did not receive STUN response %d\n", GetLastNetworkError());
            closesocket(context->s);
            return -1;
        }
        recv_size += single_recv_size;
    }

    if (recv_size != sizeof(entry)) {
        flog("TCP STUN Response packet of wrong size! %d\n", recv_size);
        closesocket(context->s);
        return -1;
    }

    // Print STUN response
    struct in_addr a;
    a.s_addr = entry.ip;
    flog("TCP STUN responded that the TCP server is located at %s:%d\n",
         inet_ntoa(a), ntohs(entry.private_port));

    closesocket(context->s);

    context->s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (context->s <= 0) {  // Windows & Unix cases
        flog("Could not create UDP socket %d\n", GetLastNetworkError());
        return -1;
    }

    // Reuse addr
    opt = 1;
    if (setsockopt(context->s, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt,
                   sizeof(opt)) < 0) {
        flog("Could not setsockopt SO_REUSEADDR\n");
        return -1;
    }

    // Bind to port
    if (bind(context->s, (struct sockaddr *)(&origin_addr),
             sizeof(origin_addr)) < 0) {
        flog("Failed to bind to port! %d\n", GetLastNetworkError());
        closesocket(context->s);
        return -1;
    }
    set_timeout(context->s, stun_timeout_ms);

    context->addr.sin_family = AF_INET;
    context->addr.sin_addr.s_addr = entry.ip;
    context->addr.sin_port = entry.private_port;

    flog("Connecting to server...\n");

    sleep(0.200);

    // Connect to TCP server
    if (!tcp_connect(context->s, context->addr, stun_timeout_ms)) {
        flog("Could not connect to server over TCP\n");
        return -1;
    }

    flog("Connected on %s:%d!\n", destination, port);

    set_timeout(context->s, recvfrom_timeout_ms);
    return 0;
}

int CreateTCPServerContextStun(SocketContext *context, int port,
                               int recvfrom_timeout_ms, int stun_timeout_ms) {
    if (context == NULL) {
        flog("Context is NULL\n");
        return -1;
    }

    context->is_tcp = true;

    // Init stun_addr
    struct sockaddr_in stun_addr;
    stun_addr.sin_family = AF_INET;
    stun_addr.sin_addr.s_addr = inet_addr(STUN_IP);
    stun_addr.sin_port = htons(STUN_PORT);
    int opt;

    // Create TCP socket
    context->s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (context->s <= 0) {  // Windows & Unix cases
        flog("Could not create UDP socket %d\n", GetLastNetworkError());
        return -1;
    }
    set_timeout(context->s, stun_timeout_ms);

    SOCKET udp_s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    // cppcheck-suppress nullPointer
    sendto(udp_s, NULL, 0, 0, (struct sockaddr *)&stun_addr, sizeof(stun_addr));
    closesocket(udp_s);

    // Server connection protocol
    context->is_server = true;

    // Reuse addr
    opt = 1;
    if (setsockopt(context->s, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt,
                   sizeof(opt)) < 0) {
        flog("Could not setsockopt SO_REUSEADDR\n");
        return -1;
    }

    struct sockaddr_in origin_addr;
    // Connect over TCP to STUN
    flog("Connecting to STUN TCP...\n");
    if (!tcp_connect(context->s, stun_addr, stun_timeout_ms)) {
        flog("Could not connect to STUN Server over TCP\n");
        return -1;
    }

    socklen_t slen = sizeof(origin_addr);
    if (getsockname(context->s, (struct sockaddr *)&origin_addr, &slen) < 0) {
        flog("Could not get sock name\n");
        closesocket(context->s);
        return -1;
    }

    // Send STUN request
    stun_request_t stun_request;
    stun_request.type = POST_INFO;
    stun_request.entry.public_port = htons((unsigned short)port);

    if (sendp(context, &stun_request, sizeof(stun_request)) < 0) {
        flog("Could not send STUN request to connected STUN server!\n");
        closesocket(context->s);
        return -1;
    }

    sleep(1);
    // Receive STUN response
    fractal_clock_t t;
    FractalStartTimer(&t);

    int recv_size = 0;
    stun_entry_t entry = {0};

    while (recv_size < (int)sizeof(entry) &&
           FractalGetTimer(t) < stun_timeout_ms) {
        int single_recv_size;
        if ((single_recv_size = recvp(context, ((char *)&entry) + recv_size,
                                      max(0, (int)sizeof(entry) - recv_size))) <
            0) {
            flog("Did not receive STUN response %d\n", GetLastNetworkError());
            closesocket(context->s);
            return -1;
        }
        recv_size += single_recv_size;
    }

    if (recv_size != sizeof(entry)) {
        flog("TCP STUN Response packet of wrong size! %d\n", recv_size);
        closesocket(context->s);
        return -1;
    }

    // Print STUN response
    struct sockaddr_in client_addr;
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = entry.ip;
    client_addr.sin_port = entry.private_port;
    flog("TCP STUN notified of desired request from %s:%d\n",
         inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    closesocket(context->s);

    // Create TCP socket
    context->s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (context->s <= 0) {  // Windows & Unix cases
        flog("Could not create UDP socket %d\n", GetLastNetworkError());
        return -1;
    }

    opt = 1;
    if (setsockopt(context->s, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt,
                   sizeof(opt)) < 0) {
        flog("Could not setsockopt SO_REUSEADDR");
        return -1;
    }

    // Bind to port
    if (bind(context->s, (struct sockaddr *)(&origin_addr),
             sizeof(origin_addr)) < 0) {
        flog("Failed to bind to port! %d\n", GetLastNetworkError());
        closesocket(context->s);
        return -1;
    }

    flog("WAIT");

    // Connect to client
    if (!tcp_connect(context->s, client_addr, stun_timeout_ms)) {
        flog("Could not connect to Client over TCP\n");
        return -1;
    }

    context->addr = client_addr;
    flog("Client received at %s:%d!\n", inet_ntoa(context->addr.sin_addr),
         ntohs(context->addr.sin_port));
    set_timeout(context->s, recvfrom_timeout_ms);
    return 0;
}
