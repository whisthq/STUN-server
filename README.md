# Fractal STUN Server

![STUN CI](https://github.com/fractalcomputers/STUN-server/workflows/STUN%20CI/badge.svg)

This repository contains the implementation of the Fractal STUN/hole-punching server that initiates a connection between a streaming server, typically a Fractal cloud computer or a customer's personal computer using Fractal to stream, and a client device. The STUN server is necessary for enabling connections between devices behind NATs, which is not the case for Fractal cloud computers hosted on Azure/AWS, but can be used for non-NAT devices as well.

Fractal STUN server(s) are all hosted on AWS Lightsail with Ubuntu 18.04. To create a new STUN-server instance:

- Create an Ubuntu 18.04 OS-only instance from the AWS Lightsail platform. You can access the identifiers via the Fractal Google Drive, under Business Development, or via the Admin Dashboard.
- Create a Static IP and attach it to your instance.
- Go to your instance's networking and open "All UDP", "All TCP", and "SSH". You should remove everything else.

You then need to install dependencies and this codebase. You can do so via:

```
sudo apt-get update
sudo apt-get install git make g++ immortal
curl -s https://packagecloud.io/install/repositories/immortal/immortal/script.deb.sh | sudo bash
```

You can also install git, clone this repository and simply run `./setup.sh` and it will take care of installing all those dependencies.

## Building & Running

You can build the code by typing `make`. 

The STUN server(s) are meant to run 24/7. We use Immortal as a process manager to immediately restart after a crash. When starting a STUN-server for production usage, you should start it with:

```
immortal ./server
```

It will run in the background and restart automatically if it exits. To see if it is running, you can run `immortalctl`, which prints the running jobs and their names & PID. A proccess can be shutdown via `immortalctl -k <process-name>`. 

We have continuous integration in this project, using GitHub Actions. When a push or PR happens on master or dev, the executable will get compiled on Ubuntu and clang-format will be run. It will also run unit and integration tests using Unity, including testing UDP and TCP connectivity. You can see those in the `/tests` folder. You should make sure that your commit passes the tests under the Actions tab before making a pull request, if you are contributing.

## Styling

We use clang-format for the coding style on this repository. You can easily run `clang-format -i <file>` to run it on a specific file, or `clang-format -i *.cpp` to run it on all C++ files, for example.

## Running Servers

There are currently 1 STUN server(s) active: https://lightsail.aws.amazon.com/ls/webapp/home/instances

## Publishing & Updating

Currently, we do not have an automated way to replace the STUN server in Lightsail other than manually take it down and git pull/make the new version. Once you have updated the production code, you should run `./update.sh` to notify the Fractal team via Slack.  
