# Fractal STUN Server

This repository contains the implementation of the Fractal hole punching server that initiates connection between a Fractal cloud computer and a client device.

The code is in raw C and runs on Linux Ubuntu 18.04.

Fractal hole punching server(s) are all hosted on AWS Lightsail with Ubuntu 18.04. When creating a new hole punching server, you can simply create an Ubuntu 18.04 OS-only instance from the AWS Lightsail platform. You then need to create a Static IP, which is free if attached to an instance, and attach it to the new instance created. You then need to go into the instance's networking and open "All UDP" and "All TCP" on the ports and remove everything else but SSH and "All UDP" and "All TCP".

Each AWS Lightsail server, before making and running the server code, needs to be configured with:

`sudo apt-get update`

`sudo apt-get install git`

`sudo apt-get install make`

`sudo apt-get install g++`

`curl -s https://packagecloud.io/install/repositories/immortal/immortal/script.deb.sh | sudo bash`

`sudo apt-get install immortal`

You can simply run `setup.sh` and it will take care of installing all those dependencies.

You can then clone this repository, cd into it and build with `make`.

## Running and Autorestart

A hole punching server, once started, should run forever, and so we can start it so that it to automatically restart after a crash. We are using Immortal for this, which gets imported with curl during the configuration. We can then start the hole punching server with the following command:

`immortal ./server`

It will run in the background and restart automatically if it exits. To see if it is running, we can run `immortalctl`, which prints the running jobs. A proccess can be shutdown with `immortalctl -k <name>`. 

## Running Servers

Currently there are 1 hole punching servers active:

The servers are hosted at: https://lightsail.aws.amazon.com/ls/webapp/home/instances
