# Fractal UDP Hole Punching Server

This repository contains the implementation for the Fractal hole punching server that initiates connection between a Fractal cloud computer and a client device.

The code is in C and runs on Linux Ubuntu 18.04.

Fractal hole punching server(s) are all hosted on AWS Lightsail with Ubuntu 18.04. When creating a new hole punching server, you can simply create an Ubuntu 18.04 OS-only instance from the AWS Lightsail platform. You then need to create a Static IP, which is free if attached to an instance, and attach it to the new instance created. You then need to go into the instance's networking and open "All UDP" on the ports and remove everything else but SSH.

Each AWS Lightsail server, before making and running the server code, needs to be configured with:
- sudo apt-get update
- sudo apt-get install git
- sudo apt-get install make
- sudo apt-get install gcc

You can then clone this repository and build with `make`.

When running the server quickly after it just quit, the socket will still be bound to the port and it will be unable to bind again. In such a case, reboot the AWS Lightsail Ubuntu 18.04 instance and restart the server.

The server can be run with `./server` and will run forever, hole-punching a direct UDP connection between a Fractal cloud computer and a client device sequentially as it receives the requests whenever a user logs in.

Currently there are 1 UDP hole punching servers active:
- Fractal-UDPHolePunchServer-1
    - Public IP: 34.200.170.47
    - Location: US-East 1 (Northern Virginia)
