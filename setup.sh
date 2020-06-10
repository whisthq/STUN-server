#!/bin/bash

sudo apt update
sudo apt install g++ make -y
curl -s https://packagecloud.io/install/repositories/immortal/immortal/script.deb.sh | sudo bash
sudo apt install immortal
