#!/bin/bash

sudo apt-get update
sudo apt-get install -y g++ make
curl -s https://packagecloud.io/install/repositories/immortal/immortal/script.deb.sh | sudo bash
sudo apt-get install immortal
