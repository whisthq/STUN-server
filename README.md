# Fractal Hole Punching Server

This repository contains the implementation for the Fractal hole punching server that initiates connection between a Fractal cloud computer and a client device.

The code is in C and runs on Linux Ubuntu 18.04.

Fractal hole punching server(s) are all hosted on AWS Lightsail with Ubuntu 18.04.

Each AWS Lightsail server, before making and running the server code, needs to be configured with:
- sudo apt-get update
- sudo apt-get install git
- sudo apt-get install make
- sudo apt-get install gcc

You can then clone this repository and build with `make`.

The server can be run with `./server` and will run forever, hole-punching a direct UDP connection between a Fractal cloud computer and a client device sequentially as it receives the requests whenever a user logs in. 

Currently there are 1 hole punching servers:
- Fractal-HolePunchServer-1
    - Static IP: 52.86.203.149
    - Location: US-East 1 (Northern Virginia)
