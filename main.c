/*
 * This file creates a server that connects to two clients through UDP hole
 * punch, acting as a Fractal hole punching server. This hole punching serer is
 * built for AWS Lightsail with Ubuntu 18.04.
 *
 * Hole Punching Server version: 1.0
 *
 * Last modification: 12/19/2019
 *
 * By: Philippe Noël
 *
 * Copyright Fractal Computers, Inc. 2019
**/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "linkedlist.h" // header file for the linked list functions

#define HOLEPUNCH_PORT 48488 // Fractal default holepunch port
#define QUEUE_LEN 50 // arbitrary, at most 50 concurrent pairing requests

/// @brief listens for UDP VM-client pairs to connect through hole punching
/// @details hole punches through NATs by saving received address and port
int32_t main(int32_t argc, char **argv) {
  // unused argv
  (void) argv;

  // usage check
  if (argc != 1) {
    printf("Usage: ./main\n"); // no argument needed
    return 1;
  }

  // hole punching server variables
  int punch_socket; // socket ID
  ssize_t recv_size; // received packets size
  struct sockaddr_in my_addr, request_addr; // endpoint of server and requests
  char recv_buff[BUFLEN]; // buffer to receive UDP packets
  socklen_t addr_size = sizeof(request_addr); // length of request address struct
  struct client *new_client; // for the request we received
  uint32_t target_ipv4; // IPv4 of the vm a client wants to connect to

  // linked lists to hold the pairing requests to be fulfilled
  struct gll_t *client_list = gll_init();
  struct gll_t *vm_list = gll_init();
  int i, j, clients_n = 0, vms_n = 0; // counter vars

  // linked list nodes for sending endpoint data
  struct gll_node_t *curr_client, *curr_vm;

  // endpoints to send over the sockets for pairing
  unsigned char client_endpoint[sizeof(struct client)]; // client
  unsigned char vm_endpoint[sizeof(struct client)]; // vm
  struct sockaddr_in client_addr, vm_addr; // addresses to send to

  // create listening socket listening for VM-client pairs to connect
  if ((punch_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < -1) {
    printf("Unable to create socket.\n");
    return 1;
  }

  // fill address struct over the default Fractal hole punching port for any
  memset(&my_addr, 0, sizeof(my_addr));
  my_addr.sin_family = AF_INET;
  my_addr.sin_port = htons(HOLEPUNCH_PORT);
  my_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  // bind the listensocket to our server address
  if (bind(punch_socket, (struct sockaddr *) &my_addr, sizeof(my_addr)) < 0) {
    printf("Unable to bound socket to port %d.\n", HOLEPUNCH_PORT);
    return 2;
  }

   // loop forever to keep connecting VM-client pairs
   while (1) {
     // empty memory for request address
     memset(&request_addr, 0, sizeof(request_addr));

     // receive a UDP packet for a connection request
     if ((recv_size = recvfrom(punch_socket, recv_buff, BUFLEN, 0, (struct sockaddr *) &request_addr, &addr_size)) < 0) {
       printf("Unable to receive UDP packet.\n");
       return 3;
     }
     // if the packet is empty, then it's a VM waiting to be connected to, if the
     // packet is not, it contains the IPv4 of the server to connect to
     // store endpoint information to pair later

     // fill struct pointer to hold this new client for elements in common to both
     memset(&new_client, 0, sizeof(struct client));
     new_client->ipv4 = request_addr.sin_addr.s_addr;
     new_client->port = request_addr.sin_port;

     // if it's a client, it has an IP address of a target as payload
     if (recv_size > 0) {
       // copy IP address of taret
       memcpy(&new_client->target, &recv_buff, (size_t) recv_size);

       // create a node for this new client and add it to the linked list
       if (gll_push_end(client_list, new_client) < 0) {
         printf("Unable to add client struct to end of client list.\n");
         return 4;
       }
       clients_n++; // increment count
     }
     else { // this is a VM waiting for a connection
       // no payload, create a node for this new vm and add it to the linked list
       if (gll_push_end(vm_list, new_client) < 0) {
         printf("Unable to add vm struct to end of vm list.\n");
         return 5;
       }
       vms_n++; // increment count
     }

     // check if we have any possible pairing to make
     // if there is at least 1 VM and 1 client waiting for a pairing
     if (vms_n > 0 && clients_n > 0) {
       // loop over each client waiting and see if we can pair anything
      for (i = 0; i < clients_n; i++) {
        // get the target IPv4 of the client data at this node index
        curr_client = gll_find_node(client_list, i);

        // get the target IP of that client in network byte format
        target_ipv4 = htonl((char *) curr_client->data->target);

        // for a specific client, loop over all VMs to see if target IP match
        for (j = 0; j < vms_n; j++) {
          // get the IPv4 of the client data at this node index
          curr_vm = gll_find_node(vm_list, j);

          // if the client wants to connect to this VM, we send their endpoints
          if (target_ipv4 == curr_vm->data->ipv4) {
            // we send memory to avoid endianness byte issue
            memcpy(client_endpoint, &curr_client->data, sizeof(struct client));
            memcpy(vm_endpoint, &curr_vm->data, sizeof(struct client));

            // set memory of  structs for address to send to
            memset(&client_addr, 0, sizeof(client_addr));
            memset(&vm_addr, 0, sizeof(vm_addr));

            // fill client address struct
            client_addr.sin_family = AF_INET;
            client_addr.sin_port = htons(curr_client->data->port);
            client_addr.sin_addr.s_addr = htonl(curr_client->data->ipv4);

            // fill VM address struct
            vm_addr.sin_family = AF_INET;
            vm_addr.sin_port = htons(curr_vm->data->port);
            vm_addr.sin_addr.s_addr = htonl(curr_vm->data->ipv4);

            // send the endpoint of the client to the VM
            if (sendto(punch_socket, &client_endpoint, sizeof(struct client), 0, (struct sockaddr *) &vm_addr, addr_size) < 0) {
              printf("Unable to send client endpoint to VM.\n");
              return 6;
            }

            // send the endpoint of the vm to the client
            if (sendto(punch_socket, &vm_endpoint, sizeof(struct client), 0, (struct sockaddr *) &client_addr, addr_size) < 0) {
              printf("Unable to send VM endpoint to client.\n");
              return 7;
            }
            // the two are now paired and can start communicating over UDP

            // now that we paired the two, we remove them from the request lists
            gll_remove(client_list, i); // remove client
            gll_remove(vm_list, j); // remove VM

            // and decrease the counts left to connect
            clients_n--;
            vms_n--;
          }
        } // vms for loop
      } // clients for loop
    } // if there is a non-zero # of VMs and clients to connect
  } // forever while loop
  // server loop exited, close linked lists to exit
  gll_destroy(client_list);
  gll_destroy(vm_list);

  // close socket and exit
  close(punch_socket);
  return 0;
}
