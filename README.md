# Fractal UDP Hole Punching Server

This repository contains the implementation of the Fractal hole punching server that initiates connection between a Fractal cloud computer and a client device.

The code is in raw C and runs on Linux Ubuntu 18.04.

Fractal hole punching server(s) are all hosted on AWS Lightsail with Ubuntu 18.04. When creating a new hole punching server, you can simply create an Ubuntu 18.04 OS-only instance from the AWS Lightsail platform. You then need to create a Static IP, which is free if attached to an instance, and attach it to the new instance created. You then need to go into the instance's networking and open "All UDP" on the ports and remove everything else but SSH and "All UDP".

Each AWS Lightsail server, before making and running the server code, needs to be configured with:
- sudo apt-get update
- sudo apt-get install git
- sudo apt-get install make
- sudo apt-get install gcc
- sudo apt-get install daemon

You can then clone this repository and build with `make`.

## Running and Autorestart

A hole punching server, once started, should run forever, and so we can start it so that it to automatically restart after a crash with the following:

`daemon --name="holepunchserver" --respawn --output=holepunchlog.txt server`

The program will now run in the background, meaning it is safe to exit the SSH shell connection to the Lightsail instance, without the program stopping, and it will automatically restart if it crashes thanks to the `--respawn` command.

If you want to stop the daemon process, you can do it with the following command:

`daemon --name="holepunchserver" --stop`

## Running Servers

Currently there are 1 UDP hole punching servers active:
- Fractal-UDPHolePunchServer-1
    - Public IP: 34.200.170.47
    - Location: US-East 1 (Northern Virginia)
