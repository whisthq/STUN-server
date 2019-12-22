# Fractal UDP Hole Punching Server

This repository contains the implementation for the Fractal hole punching server that initiates connection between a Fractal cloud computer and a client device.

The code is in C and runs on Linux Ubuntu 18.04.

Fractal hole punching server(s) are all hosted on AWS Lightsail with Ubuntu 18.04.

Each AWS Lightsail server, before making and running the server code, needs to be configured with:
- sudo apt-get update
- sudo apt-get install git
- sudo apt-get install make
- sudo apt-get install gcc

You can then clone this repository and build with `make`.

When running the server quickly after it just quit, the socket will still be bound to the port and it will be unable to bind again. In such a case, reboot the AWS Lightsail Ubuntu 18.04 instance and restart the server.

The server can be run with `./server` and will run forever, hole-punching a direct UDP connection between a Fractal cloud computer and a client device sequentially as it receives the requests whenever a user logs in.

Currently there are 1 hole punching servers:
- Fractal-UDPHolePunchServer-1
    - Public IP: 34.200.170.47
    - Location: US-East 1 (Northern Virginia)
