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
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "include/socket.h" // header file for reliable UDP sending
#include "include/linkedlist.h" // header file for the linked list functions

#define BUFLEN 512 // len of receive buffer
#define HOLEPUNCH_PORT 48800 // Fractal default holepunch port
#define MAX_QUEUE_LEN 100 // arbitrary, maximum concurrent pairing requests waiting

// a small struct to hold a UDP client endpoint
struct client {
    int host;
    short port;
};

// a small struct to hold a client pair endpoints
struct pair {
    int client_ip;
    short client_port;
    int server_ip;
    short server_port;
};

// main server loop
int main(void) {
<<<<<<< HEAD
  // punch vars
  struct sockaddr_in si_me, si_other; // our endpoint and the client's
  int s, i, j, recv_size, paired, slen = sizeof(si_other), n = 0; // counters
  char buf[BUFLEN]; // receive buffer

  // linked list to hold the connection requests (for asynchronous handling)
  struct gll_t *pairs_list = gll_init();

  // vars to hold the current node and current client or pair we are looking at
  struct gll_node_t *curr_node, paired_node;
  struct client client_endpoint; // struct to hold VM or local client endpoint for sending
  struct pair tmp; // new pair node to be inserted in the list

  // initialize endpoints for a node for reassignment later
  int client_ip, server_ip;
  short client_port, server_port;


  // create the UDP socket
  if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
    printf("Could not create UDP socket.\n");
=======
  // hole punching server variables
  int punch_socket; // socket ID
  ssize_t recv_size; // received packets size
  struct sockaddr_in my_addr, request_addr; // endpoint of server and requests
  socklen_t addr_size = sizeof(request_addr); // length of request address struct
  int i, j, clients_n = 0, vms_n = 0; // counter vars

  // buffer and reception vars
  char recv_buff[BUFLEN], client_ip[BUFLEN], server_ip[BUFLEN]; // buffer to receive UDP packets and tmp store for memcpy
  uint32_t curr_client_target_ipv4, curr_vm_ipv4; // IPv4 of vm, client compared for match
  struct client new_client, new_vm; // for the request we receive

  // linked lists to hold the pairing requests to be fulfilled
  struct gll_t *client_list = gll_init();
  struct gll_t *vm_list = gll_init();
  struct gll_node_t *curr_client, *curr_vm; // linked list nodes for sending endpoint data

  // endpoints to send over the sockets for pairing
  unsigned char client_endpoint[sizeof(struct client)]; // client
  unsigned char vm_endpoint[sizeof(struct client)]; // vm
  struct sockaddr_in client_addr, vm_addr; // addresses to send to

  // create listening socket listening for VM-client pairs to connect
  if ((punch_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
    printf("Unable to create socket.\n");
>>>>>>> 615686bd4e80fc621e316be347f9db6fbbdb1f45
    return -1;
  }

  // set our endpoint (for this UDP hole punching server not behind a NAT)
  memset((char *) &si_me, 0, sizeof(si_me));
  si_me.sin_family = AF_INET;
  si_me.sin_port = htons(HOLEPUNCH_PORT);
  si_me.sin_addr.s_addr = htonl(INADDR_ANY);

  // bind socket to this endpoint
  if (bind(s, (struct sockaddr*) &si_me, sizeof(si_me)) < 0) {
    printf("Failed to bind socket. `Sudo reboot` and try again.\n");
    return -2;
  }

  // main hole punching loop
  while (1) {
<<<<<<< HEAD
    // index of matched pair in list, -1 if no pair matched yet
    paired = -1;

    // when a new client sends a datagram connection request...
    if ((recv_size = recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *) &si_other, &slen)) < 0) {
      printf("Could not receive UDP packet from client.\n");
      return -3;
    }
    // the client's public UDP endpoint data is now in si_other
    printf("Received packet from %s:%d.\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));

    // parse the buffer
    char origin = buf[recv_size - 1]; // whether it's from a VM or local client
    char target_ip[recv_size - 1]; // target IP, if from a local client
    strcpy(target_ip, buf); // fill target IP
    target_ip[recv_size - 1] = 0;

    // while we don't have a full pair yet, we look through the list and see
    // if this new request can match a pair
    for (i = 0; i < n && (paired == -1); i++) {
      // get the current node we are
      curr_node = gll_find_node(pairs_list, i);

      // if the request is from a local client
      if (origin == 'C') {
        // if the request target IP matches the server IP of the node, it's a pair
        if (inet_addr(target_ip) == curr_node->data->server_ip) {
          // fill the node wih the client endpoint
          curr_node->data->client_ip = si_other.sin_addr.s_addr;
          curr_node->data->client_port = si_other.sin_port;

          // set index of paired node for sending
          paired = i;
          printf("Matched client and server on %s.\n", target_ip);
        }
      }
      // if the request is from a VM
      else {
        // if the target IP matches the received VM IP matches, it's a pair
        if (curr_node->data->server_ip == si_other.sin_addr.s_addr) {
          // fill the VM endpoint
          curr_node->data->server_port = si_other.sin_port;

          // set index of paired node for sending
          paired = i;
          printf("Matched server and client on %s.\n", inet_ntoa(si_other.sin_addr));
        }
      }
    }
=======
    // empty memory for request address and buffers
    memset(&request_addr, 0, sizeof(request_addr));
    memset(&client_ip, 0, BUFLEN); // to avoid garbage init memory
    memset(&server_ip, 0, BUFLEN); // to avoid garbage init memory
    memset(&recv_buff, 0, BUFLEN); // to avoid garbage init memory

    printf("Waiting for a connection request...\n"); // for transparency
    // receive a UDP packet with the IPv4 address of the sender to act as a
    // connection request with a tag to indicate whether this is from a client
    // or a VM. The packet format is "x.x.x.xT", where T = C is this is a local
    // client, and V if it is from a VM
    if ((recv_size = reliable_udp_recvfrom(punch_socket, recv_buff, BUFLEN, request_addr, addr_size)) < 0) {
      // very unlikely it can fail with error code -1
      printf("Unable to receive connection-request UDP packet.\n");
      return -3;
    }
    printf("Received a connection request.\n");


	printf("recvsize: %ld\n", recv_size);

	printf("recbuff[recvsize - 1] == %c\n", recv_buff[recv_size - 1]);



    // it's a client if it has a "C" tag at the last position in the recv_buff
    if (recv_buff[recv_size - 1] == 'C') {

	printf("do we make it here\n");


      // fill struct pointer to hold this new client
      memset(&new_client, 0, sizeof(struct client));

      // port stays intact through NAT, just copy it
      new_client.port = request_addr.sin_port;

      // copy the IPv4 and store into our client struct
      memcpy(&tmp, &recv_buff, recv_size - 1); // copy without tag
      printf("temp address is %s\n", tmp);
      new_client.ipv4 = inet_addr(tmp); // convert to network byte order

      // empty memory of buffers for next IP address since this is a client
//      memset(&request_addr, 0, sizeof(request_addr));
//      memset(&recv_buff, 0, BUFLEN);
//      memset(&tmp, 0, BUFLEN);

>>>>>>> 615686bd4e80fc621e316be347f9db6fbbdb1f45

    // if we have a matched pair
    if (paired > -1) {
      // get the node of the matched pair
      paired_node = gll_find_node(pairs_list, paired);

      // store the node endpoints to avoid modifying the linked list
      client_ip = paired_node->data->client_ip;
      client_port = paired_node->data->client_port;
      server_ip = paired_node->data->server_ip;
      server_port = paired_node->data->server_port;

      // prepare endpoint for sending to the local client for hole punching
      si_other.sin_addr.s_addr = client_ip;
      si_other.sin_port = client_port;

      // create client struct to send endpoint
      client_endpoint = {0};
      client_endpoint.host = server_ip;
      client_endpoint.port = server_port;

      // send client endpoint to VM
      if (sendto(s, &client_endpoint, sizeof(client_endpoint), 0, (struct sockaddr *) &si_other, slen) < 0) {
        printf("Could not send VM endpoint to client.\n");
        return -4;
      }
      printf("Sent VM info to client %u:%d.\n", client.host, client.port);

      // prepare endpoint for sending to the VM for hole punching
      si_other.sin_addr.s_addr = server_ip;
      si_other.sin_port = server_port;

      // fill client struct to send endpoint
      client_endpoint.host = client_ip;
      client_endpoint.port = client_port;

<<<<<<< HEAD
      // send VM endpoint to client
      if (sendto(s, &client_endpoint, sizeof(client_endpoint), 0, (struct sockaddr *) &si_other, slen) < 0) {
        printf("Could not send client endpoint to VM.\n");
        return -5;
=======
      // a client also needs to send the IPv4 of the VM it wants to be paired
      // with, which it obtained through authenticating, so we receive another
      // packet containing the target IPv4
      if ((recv_size = reliable_udp_recvfrom(punch_socket, recv_buff, BUFLEN, request_addr, addr_size)) < 0) {
        // very unlikely it can fail with error code -1
        printf("Unable to receive client target IPv4 UDP packet.\n");
        return -4;
      }
      printf("Connection request is from a user client.\n");

      // copy the target IPv4 and store into our client struct
      memcpy(&tmp, &recv_buff, recv_size); // copy the target IPv4, no tag this time
      printf("Temp IP is %s\n", temp);
      new_client.target_ipv4 = inet_addr(tmp); // convert to network byte order

      // if there's still space on our client queue
      if (clients_n < MAX_QUEUE_LEN) {
        // create a node for this new client and add it to the linked list
        if (gll_push_end(client_list, &new_client) < 0) {
          printf("Unable to add client struct to end of client list.\n");
          return -5;
        }
        printf("Inserted new client pairing request in queue: Client #%d.\n", clients_n);
        clients_n++; // increment count
>>>>>>> 615686bd4e80fc621e316be347f9db6fbbdb1f45
      }
      printf("Sent client info to VM %u:%d.\n", client.host, client.port);

      // now that the pairing and hole punching happened, we can remove this node
      gll_remove(pairs_list, paired); // remove whole struct for a pair
      n -= 1; // decrement linked list node count
    }
    // if we don't have a matched pair, we will add to the linked list
    else {
      // only add to the list if it's smaller than the max queue size
      if (n < MAX_QUEUE_LEN) {
        // if it's a request from a local client
        if (origin == 'C') {
          // create a pair struct to insert into the linked list  and fill it
          tmp.client_ip = si_other.sin_addr.s_addr;
          tmp.client_port = si_other.sin_port;
          tmp.server_ip = inet_addr(target_ip);

          // create a node for this new client and add it to the linked list
          if (gll_push_end(pairs_list, &tmp) < 0) {
            printf("Unable to add pair struct to end of client list.\n");
            return -6;
          }
          printf("Added client with host IP %s:%d and target IP %s to the queue\n",
                  inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port), target_ip);
        }
        // if it's a request from a VM
        else {
          // create a pair struct to insert into the linked list  and fill it
          tmp.server_ip = si_other.sin_addr.s_addr;
          tmp.server_port = si_other.sin_port;

          // create a node for this new VM and add it to the linked list
          if (gll_push_end(pairs_list, &tmp) < 0) {
            printf("Unable to add pair struct to end of client list.\n");
            return -7;
          }
          printf("Added server with host IP %s:%d to the queue\n",
              inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
        }
        // increment list size
        n++;
      }
    } // end of if < MAX_QUEUE_LEN
  } // end of connection listening for loop

  // hole punching loop exited, close everything
  gll_destroy(pairs_list);
  close(s); // close socket
  return 0;
}
