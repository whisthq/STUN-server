/*
 * This file creates a server that connects two clients through UDP hole punch,
 * acting as a Fractal hole punching server. This hole punching serer is
 * built for AWS Lightsail with Ubuntu 18.04.
 *
 * Hole Punching Server version: 1.0
 *
 * Last modified: 12/28/2019
 *
 * By: Philippe Noël, Ming Ying
 *
 * Copyright Fractal Computers, Inc. 2019
**/

#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>

#define HOLEPUNCH_PORT 48800 // Fractal default holepunch port

#include <map>
#include <vector>

using namespace std;

// a small struct to hold a UDP client endpoint, pair struct in linkedlist.h
typedef struct {
    unsigned int ip;
    unsigned short private_port;
    unsigned short public_port;
} stun_entry_t;

typedef enum stun_request_type {
    ASK_INFO,
    POST_INFO
} stun_request_type_t;

typedef struct {
    stun_request_type_t type;
    stun_entry_t entry;
} stun_request_t;

typedef struct {
     double time;
     stun_entry_t entry;
} stun_map_entry_t;

map<int, vector<stun_map_entry_t>> stun_entries;

double time() {
     struct timeval tv;
     gettimeofday(&tv, NULL);
     return tv.tv_sec + tv.tv_usec / 1000000.0;
}

// main server loop
int main(void) {
  // punch vars
  struct sockaddr_in si_me, si_other; // our endpoint and the client's
  int s, recv_size; // counters
  socklen_t slen = sizeof(si_other); // addr len

  // initialize endpoints for a node for reassignment later
  int client_ip, server_ip;
  short client_port, server_port;

  // create the UDP socket
  if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
    printf("Could not create UDP socket.\n");
  }

  // set our endpoint (for this UDP hole punching server not behind a NAT)
  memset((char *) &si_me, 0, sizeof(si_me));
  si_me.sin_family = AF_INET;
  si_me.sin_port = htons(HOLEPUNCH_PORT);
  si_me.sin_addr.s_addr = htonl(INADDR_ANY);

  // bind socket to this endpoint
  if (bind(s, (struct sockaddr*) &si_me, sizeof(si_me)) < 0) {
    printf("Failed to bind socket. `sudo reboot` and try again.\n");
    return -2;
  }

  // main hole punching loop
  while (1) {
    stun_request_t request; // receive buffer
    printf("Receiving...\n");
    // when a new client sends a datagram connection request...
    if ((recv_size = recvfrom(s, &request, sizeof(request), 0, (struct sockaddr *) &si_other, &slen)) < 0) {
      printf("Could not receive UDP packet from client.\n");
      return -3;
    }
    // the client's public UDP endpoint data is now in si_other
    printf("Received packet from %s:%d.\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));

    if (recv_size == sizeof(request)) {
      if (request.type == ASK_INFO) {
        int ip = request.entry.ip;
	int port = request.entry.public_port;
	int private_port = -1;

	if (stun_entries.count(ip)) {
	  for(stun_map_entry_t map_entry : stun_entries[ip]) {
            if (time() - map_entry.time > 0.5) {
	      continue;
	    }
            stun_entry_t entry = map_entry.entry;
            if (port == entry.public_port) {
              private_port = entry.private_port;
	      printf("Found port %d to public %d!\n", ntohs(private_port), ntohs(port));
	    }
	  }
        }

	if (private_port == -1) {
          request.entry.private_port = -1;
          printf("Could not find private_port associated with %d!\n", port);
	} else {
          request.entry.private_port = private_port;	

	  struct sockaddr_in si_server;
	  si_server.sin_family = AF_INET; 
	  si_server.sin_addr.s_addr = request.entry.ip; 
	  si_server.sin_port = request.entry.private_port; 

	  stun_entry_t entry;
	  entry.ip = si_other.sin_addr.s_addr;
	  entry.private_port = si_other.sin_port;

	  // Notify the server about the STUN connection
          sendto(s, &entry, sizeof(entry), 0, (struct sockaddr *) &si_server, sizeof(si_server));
	}
        sendto(s, &request.entry, sizeof(request.entry), 0, (struct sockaddr *) &si_other, sizeof(si_other));
      } else if (request.type == POST_INFO) {
	int ip = si_other.sin_addr.s_addr;
	request.entry.ip = ip;
	request.entry.private_port = si_other.sin_port;
	bool found = false;
	for(stun_map_entry_t& map_entry : stun_entries[ip]) {
          stun_entry_t& entry = map_entry.entry;
	  if(entry.public_port == request.entry.public_port) {
	    found = true;
	    map_entry.time = time();
            entry = request.entry;
	  }
	}
	if (!found) {
	  if (stun_entries[ip].size() > 5) {
	    stun_entries[ip].erase(stun_entries[ip].begin());
	  }
	  stun_map_entry_t map_entry;
	  map_entry.time = time();
	  map_entry.entry = request.entry;
	  stun_entries[ip].push_back(map_entry);
	}
      }
    } else {
      printf("Incorrect size! %d instead of %d\n", recv_size, (int)sizeof(request));
    }
  } // end of connection listening for loop

  // hole punching loop exited, close everything
  close(s); // close socket
  return 0;
}
