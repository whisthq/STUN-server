/*
 * This file creates a server that connects to two clients through UDP hole
 * punch, acting as a Fractal hole punching server. This hole punching serer is
 * built for AWS Lightsail with Ubuntu 18.04.
 *
 * Hole Punching Server version: 1.0
 *
 * Last modification: 12/20/2019
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



#include <errno.h>



#include "linkedlist.h" // header file for the linked list functions

#define BUFLEN 128 // len of receive buffer
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
  char recv_buff[BUFLEN], tmp[128]; // buffer to receive UDP packets and tmp store for memcpy
  socklen_t addr_size = sizeof(request_addr); // length of request address struct
  struct client new_client; // for the request we received
  uint32_t curr_client_target_ipv4, curr_vm_ipv4; // IPv4 of vm, client compared for match

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
  if ((punch_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
    printf("Unable to create socket.\n");
    return 1;
  }
  printf("UDP socket created.\n");

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
  printf("UDP socket bound to port %d.\n", HOLEPUNCH_PORT);

  // set the socket to permit broadcast addresses, as those behind NATs can be
  // considered as broadcast addresses because of the nature of the NAT
  if (setsockopt(punch_socket, SOL_SOCKET, SO_BROADCAST, &(int) {1}, sizeof(int)) < 0) {
    printf("Unable to set socket to broadcast.\n");
    return 3;
  }

  // loop forever to keep connecting VM-client pairs
  while (1) {
    // empty memory for request address
    memset(&request_addr, 0, sizeof(request_addr));

    printf("Waiting for connection requests...\n");
    // receive a UDP packet with the IPv4 address of the sender to act as a
    // connection request with a tag to indicate whether this is from a client
    // or a VM. The packet format is "x.x.x.xT", where T = C is this is a local
    // client, and V if it is from a VM
    if ((recv_size = recvfrom(punch_socket, recv_buff, BUFLEN, 0, (struct sockaddr *) &request_addr, &addr_size)) < 0) {
      printf("Unable to receive connection-request UDP packet.\n");
      return 4;
    }
    printf("Received connection request from a client.\n");

    // acknowledge receipt of the packet to ensure we can re-send if the packet
    // failed, since this is UDP, we just send an empty packet
    if (sendto(punch_socket, "", 0, 0, (struct sockaddr *) &request_addr, addr_size) < 0) {
      printf("Unable to send connection receipt acknowledgement UDP packet.\n");
      return 5;
    }
    printf("Sent connection receipt acknowledgement to the client.\n");

    // fill struct pointer to hold this new client
    memset(&new_client, 0, sizeof(struct client));
    new_client.port = request_addr.sin_port; // port stays intact through NAT

    // copy the IPv4 and store into our client struct
    memcpy(&tmp, &recv_buff, recv_size - 1); // copy the IPv4 of the sender without the tag
    new_client.ipv4 = inet_addr(tmp); // convert to network byte order

    // it's a client if it has a "C" tag at the last position in the recv_buff
    if (recv_buff[recv_size - 1] == 'C') {
      // a client also needs to send the IPv4 of the VM it wants to be paired
      // with, which it obtained through authenticating, so we receive another
      // packet containing the target IPv4
      if ((recv_size = recvfrom(punch_socket, recv_buff, BUFLEN, 0, (struct sockaddr *) &request_addr, &addr_size)) < 0) {
        printf("Unable to receive client target IPv4 UDP packet.\n");
        return 6;
      }
      printf("Received the local client target VM IPv4 request.\n");

      // acknowledge receipt again to make sure we can send it again if necessary
      if (sendto(punch_socket, "", 0, 0, (struct sockaddr *) &request_addr, addr_size) < 0) {
        printf("Unable to send target IPv4 receipt acknowledgment UDP packet.\n");
        return 7;
      }
      printf("Sent target IPv4 receipt acknowledgement to the local client.\n");

      // copy the target IPv4 and store into our client struct
      memcpy(&tmp, &recv_buff, recv_size); // copy the target IPv4, no tag this time
      new_client.target_ipv4 = inet_addr(tmp); // convert to network byte order

      // create a node for this new client and add it to the linked list
      if (gll_push_end(client_list, &new_client) < 0) {
        printf("Unable to add client struct to end of client list.\n");
        return 5;
      }
      printf("Received new client pairing request: Client #%d.\n", clients_n);
      clients_n++; // increment count
    }
    // this is a VM waiting for a connection, no need to receive anything else
    else {
      // create a node for this new vm and add it to the linked list
      if (gll_push_end(vm_list, &new_client) < 0) {
        printf("Unable to add vm struct to end of vm list.\n");
        return 6;
      }
      printf("Received new VM pairing request: VM #%d.\n", vms_n);
      vms_n++; // increment count
    }

    // check if we have any possible pairing to make
    // if there is at least 1 VM and 1 client waiting for a pairing
    if (vms_n > 0 && clients_n > 0) {
      // loop over each client waiting and see if we can pair anything
      for (i = 0; i < clients_n; i++) {
        // get the node  of the client data at this node index
        curr_client = gll_find_node(client_list, i);

        // get the target IPv4 of that client in network byte format
        curr_client_target_ipv4 = curr_client->data->target_ipv4;

        // for a specific client, loop over all VMs to see if target IP match
        for (j = 0; j < vms_n; j++) {
          // get the node of the client data at this node index
          curr_vm = gll_find_node(vm_list, j);

          // get the IPv4 of that VM in network byte format
          curr_vm_ipv4 = curr_vm->data->ipv4;

          // if the client wants to connect to this VM, we send their endpoints
          if (curr_client_target_ipv4 == curr_vm_ipv4) {
            // we send memory to avoid endianness byte issue
            memcpy(client_endpoint, &curr_client->data, sizeof(struct client));
            memcpy(vm_endpoint, &curr_vm->data, sizeof(struct client));







            // set memory of  structs for address to send to
            memset(&client_addr, 0, sizeof(client_addr));
            memset(&vm_addr, 0, sizeof(vm_addr));

            // fill client address struct
            client_addr.sin_family = AF_INET;
            client_addr.sin_port = curr_client->data->port; // already in network byte order
            client_addr.sin_addr.s_addr = curr_client->data->ipv4; // already in network byte order

            // fill VM address struct
            vm_addr.sin_family = AF_INET;
            vm_addr.sin_port = curr_vm->data->port; // already in network byte order
            vm_addr.sin_addr.s_addr = curr_vm->data->ipv4; // already in network byte order

            // send the endpoint of the client to the VM
            if (sendto(punch_socket, client_endpoint, sizeof(struct client), 0, (struct sockaddr *) &vm_addr, addr_size) < 0) {
              printf("Unable to send client endpoint to VM.\n");


              printf("errno: %d\n", errno);

              return 7;
            }

            // send the endpoint of the vm to the client
            if (sendto(punch_socket, vm_endpoint, sizeof(struct client), 0, (struct sockaddr *) &client_addr, addr_size) < 0) {
              printf("Unable to send VM endpoint to client.\n");
              return 8;
            }
            // the two are now paired and can start communicating over UDP

            // now that we paired the two, we remove them from the request lists
            gll_remove(client_list, i); // remove client
            gll_remove(vm_list, j); // remove VM

            // status message
            printf("Paired client #%d with VM #%d.\n", i, j);

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
