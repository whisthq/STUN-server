//
// Created by hamish on 9/6/20.
//
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
