# Fractal UDP Hole Punching Server

This repository contains the implementation of the Fractal hole punching server that initiates connection between a Fractal cloud computer and a client device.

The code is in raw C and runs on Linux Ubuntu 18.04.

Fractal hole punching server(s) are all hosted on AWS Lightsail with Ubuntu 18.04. When creating a new hole punching server, you can simply create an Ubuntu 18.04 OS-only instance from the AWS Lightsail platform. You then need to create a Static IP, which is free if attached to an instance, and attach it to the new instance created. You then need to go into the instance's networking and open "All UDP" on the ports and remove everything else but SSH and "All UDP".

Each AWS Lightsail server, before making and running the server code, needs to be configured with:
- sudo apt-get update
- sudo apt-get install git
- sudo apt-get install make
- sudo apt-get install gcc

You can then clone this repository and build with `make`.

## Autostart and Autorestart

A hole punching server, once started, should run forever, and so we can set it to automatically restart after a crash with the following:
- Type `cd fractal-udpholepunch-server`
- Type `mv server-start.desktop ~/.config/autostart`

The bash script `server-script.sh` will now run at startup of the Lightsail instance and will re-run automatically if it crashes.

## Running

When opening an SSH terminal while the hole punching server code is still running, you will not see the code run but it will be in the background. If you try to run the server, the socket will still be bound to the port and it will be unable to bind again. In such a case, reboot the AWS Lightsail Ubuntu 18.04 instance with `sudo reboot` and restart the server.

The server can be run with `./server` and will run forever, hole-punching a direct UDP connection between a Fractal cloud computer and a client device sequentially as it receives the requests whenever a user logs in.

## Running Servers

Currently there are 1 UDP hole punching servers active:
- Fractal-UDPHolePunchServer-1
    - Public IP: 34.200.170.47
    - Location: US-East 1 (Northern Virginia)
