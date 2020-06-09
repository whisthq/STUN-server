/**
 * Copyright Fractal Computers, Inc. 2020
 * @file main.cpp
 * @brief This file creates a STUN server that connects two clients through
 *        UDP and TCP hole punching, acting as a Fractal hole punching server.
 *        This hole punching server is built for AWS Lightsail running
 *        Ubuntu 18.04.
 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <cerrno>

#define HOLEPUNCH_PORT 48800  // Fractal default holepunch port
#define STUN_ENTRY_TIMEOUT 30000

FILE* log_file = NULL;

void log(const char *fmt, ...) {
    if (!log_file) {
        log_file = fopen("log.txt", "a");
    }

    time_t rawtime;
    struct tm* timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    char* time_string = asctime(timeinfo);
    time_string[strlen(time_string) - 1] = '\0';

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
        printf("Moving log.txt to old_log.txt!\n");
        fclose(log_file);
        system("mv log.txt old_log.txt");
        log_file = fopen("log.txt", "a");
    }
}

#include <atomic>
#include <map>
#include <vector>

using namespace std;

// a small struct to hold a UDP client endpoint, pair struct in linkedlist.h
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

typedef struct {
    double time;
    int tcp_socket;
    stun_entry_t entry;
} stun_map_entry_t;

map<int, vector<stun_map_entry_t>> stun_entries;

double time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

atomic<int> tcp_socket;
atomic<bool> tcp_connection_pending;

typedef struct {
    int new_tcp_socket;
    struct sockaddr_in si_client;
    stun_request_t request;
    int recv_size;
} tcp_connection_data_t;

pthread_mutex_t tcp_connection_data_mutex = PTHREAD_MUTEX_INITIALIZER;
tcp_connection_data_t tcp_connection_data;

void* handle_tcp_response(void* vargp) {
    tcp_connection_data_t* handle_tcp_response_data =
        (tcp_connection_data_t*)vargp;
    struct sockaddr_in si_client = handle_tcp_response_data->si_client;
    int new_tcp_socket = handle_tcp_response_data->new_tcp_socket;
    delete handle_tcp_response_data;

    int recv_size;
    stun_request_t request;
    if ((recv_size = read(new_tcp_socket, &request, sizeof(request))) < 0) {
        log("Failed to TCP read(3)\n");
        return NULL;
    }

    while (true) {
        while (tcp_connection_pending)
            ;

        pthread_mutex_lock(&tcp_connection_data_mutex);
        if (tcp_connection_pending) {
            pthread_mutex_unlock(&tcp_connection_data_mutex);
            continue;
        }

        tcp_connection_data.si_client = si_client;
        tcp_connection_data.new_tcp_socket = new_tcp_socket;
        tcp_connection_data.request = request;
        tcp_connection_data.recv_size = recv_size;
        tcp_connection_pending = true;
        pthread_mutex_unlock(&tcp_connection_data_mutex);
        break;
    }
    return NULL;
}

void* grab_tcp_connection(void* vargp) {
    while (true) {
        if (listen(tcp_socket, 3) < 0) {
            log("Failed to TCP listen(2)\n");
            return NULL;
        }

        struct sockaddr_in si_client;
        socklen_t slen = sizeof(si_client);

        int new_tcp_socket;
        if ((new_tcp_socket = accept(tcp_socket, (struct sockaddr*)&si_client,
                                     &slen)) < 0) {
            log("Failed to TCP accept(3)\n");
            continue;
        }

        tcp_connection_data_t *handle_tcp_response_data =
            new tcp_connection_data_t;
        handle_tcp_response_data->si_client = si_client;
        handle_tcp_response_data->new_tcp_socket = new_tcp_socket;

        pthread_t thread_id;
        pthread_create(&thread_id, NULL, handle_tcp_response,
                       handle_tcp_response_data);
    }
}

// main server loop
int main(void) {
    log("Starting STUN Server...\n");

    // punch vars
    struct sockaddr_in si_me, si_client;  // our endpoint and the client's
    int s, recv_size;                     // counters
    socklen_t slen = sizeof(si_client);   // addr len

    // initialize endpoints for a node for reassignment later
    int client_ip, server_ip;
    short client_port, server_port;

    // create the UDP socket
    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        log("Could not create UDP socket.\n");
    }

    if ((tcp_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        log("Could not create UDP socket.\n");
    }

    // set our endpoint (for this UDP hole punching server not behind a NAT)
    memset((char*)&si_me, 0, sizeof(si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(HOLEPUNCH_PORT);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);

    // bind socket to this endpoint
    if (bind(s, (struct sockaddr*)&si_me, sizeof(si_me)) < 0) {
        log("Failed to bind socket. `sudo reboot` and try again.\n");
        return -2;
    }

    int opt = 1;
    if (setsockopt(tcp_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                   sizeof(opt)) < 0) {
        log("Failed to set reuseaddr socket. `sudo reboot` and try again.\n");
        return -2;
    }

    if (bind(tcp_socket, (struct sockaddr*)&si_me, sizeof(si_me)) < 0) {
        log("Failed to bind socket. `sudo reboot` and try again.\n");
        return -2;
    }

    tcp_connection_pending = false;
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, grab_tcp_connection, NULL);

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 1 * 1000;

    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout,
                   sizeof(timeout)) < 0) {
        log("Could not set timeout!\n");
        return -1;
    }

    // main hole punching loop
    while (1) {
        stun_request_t request;  // receive buffer

        // When a new client sends a datagram connection request...
        if ((recv_size = recvfrom(s, &request, sizeof(request), 0,
                                  (struct sockaddr*)&si_client, &slen)) < 0) {
            if (errno == EAGAIN) {
                recv_size = 0;
            } else {
                log("Could not receive UDP packet from client: %d\n", errno);
                return -1;
            }
        }

        int tcp_connection_socket = -1;

        // Triggered a TCP listen
        if (recv_size == 0) {
            if (tcp_connection_pending) {
                log("TCP Connection found!\n");
                si_client = tcp_connection_data.si_client;
                tcp_connection_socket = tcp_connection_data.new_tcp_socket;
                request = tcp_connection_data.request;
                recv_size = tcp_connection_data.recv_size;
                tcp_connection_pending = false;
            } else {
                // Received neither TCP nor UDP
                continue;
            }
        }

        const char* type = "UDP";
        int connection_socket = s;
        if (tcp_connection_socket > 0) {
            type = "TCP";
            connection_socket = tcp_connection_socket;
        }

        // the client's public UDP endpoint data is now in si_client
        // log("Received packet from %s:%d.\n", inet_ntoa(si_client.sin_addr),
        // ntohs(si_client.sin_port));

        if (recv_size == sizeof(request)) {
            if (request.type == ASK_INFO) {
                log("Received %s REQUEST packet from %s:%d.\n", type,
                    inet_ntoa(si_client.sin_addr), ntohs(si_client.sin_port));

                struct in_addr requested_addr;
                requested_addr.s_addr = request.entry.ip;

                char *original = inet_ntoa(si_client.sin_addr);

                log("%s:%d Wants to connect to public %s:%d.\n", original,
                    ntohs(si_client.sin_port), inet_ntoa(requested_addr),
                    ntohs(request.entry.public_port));

                int ip = request.entry.ip;
                int port = request.entry.public_port;
                int private_port = -1;
                int server_socket = s;

                if (stun_entries.count(ip)) {
                    for (stun_map_entry_t &map_entry : stun_entries[ip]) {
                        if (time() - map_entry.time >
                            STUN_ENTRY_TIMEOUT / 1000.0) {
                            continue;
                        }
                        stun_entry_t entry = map_entry.entry;
                        if (port == entry.public_port) {
                            if (map_entry.tcp_socket > 0) {
                                server_socket = map_entry.tcp_socket;
                                map_entry.time = 0;
                            }
                            private_port = entry.private_port;
                            log("Found port %d to public %d!\n\n",
                                ntohs(private_port), ntohs(port));
                            break;
                        }
                    }
                }

                if (private_port == -1) {
                    request.entry.private_port = -1;
                    log("Could not find private_port entry associated with "
                        "%s:%d!\n\n",
                        inet_ntoa(requested_addr), ntohs(port));
                } else {
                    request.entry.private_port = private_port;

                    struct sockaddr_in si_server;
                    si_server.sin_family = AF_INET;
                    si_server.sin_addr.s_addr = request.entry.ip;
                    si_server.sin_port = request.entry.private_port;

                    stun_entry_t entry;
                    entry.ip = si_client.sin_addr.s_addr;
                    entry.private_port = si_client.sin_port;

                    // Notify the server about the STUN connection
                    sendto(server_socket, &entry, sizeof(entry), MSG_NOSIGNAL,
                           (struct sockaddr*)&si_server, sizeof(si_server));
                }

                // Return answer to STUN request
                log("Responding to STUN request\n");
                sendto(connection_socket, &request.entry, sizeof(request.entry),
                       MSG_NOSIGNAL, (struct sockaddr*)&si_client,
                       sizeof(si_client));
            } else if (request.type == POST_INFO) {
                int ip = si_client.sin_addr.s_addr;
                request.entry.ip = ip;
                request.entry.private_port = si_client.sin_port;
                bool found = false;
                for (stun_map_entry_t &map_entry : stun_entries[ip]) {
                    if (map_entry.entry.public_port ==
                        request.entry.public_port) {
                        if (time() - map_entry.time >
                            STUN_ENTRY_TIMEOUT / 1000.0) {
                            log("Received %s POST_INFO packet from %s:%d.\n\n",
                                type, inet_ntoa(si_client.sin_addr),
                                ntohs(si_client.sin_port));
                        }
                        found = true;
                        map_entry.time = time();
                        map_entry.entry = request.entry;
                        map_entry.tcp_socket = -1;
                        if (tcp_connection_socket > 0) {
                            map_entry.tcp_socket = tcp_connection_socket;
                        }
                    }
                }
                if (!found) {
                    log("Received %s POST_INFO packet from %s:%d.\n\n", type,
                        inet_ntoa(si_client.sin_addr),
                        ntohs(si_client.sin_port));
                    if (stun_entries[ip].size() > 5) {
                        stun_entries[ip].erase(stun_entries[ip].begin());
                    }
                    stun_map_entry_t map_entry;
                    map_entry.time = time();
                    map_entry.entry = request.entry;
                    map_entry.tcp_socket = -1;
                    if (tcp_connection_socket > 0) {
                        map_entry.tcp_socket = tcp_connection_socket;
                    }
                    stun_entries[ip].push_back(map_entry);
                }
            }
        } else {
            log("Incorrect size! %d instead of %d\n", recv_size,
                (int)sizeof(request));
        }
    }  // end of connection listening for loop

    // hole punching loop exited, close everything
    close(s);  // close socket
    return 0;
}
