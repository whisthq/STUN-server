# Fractal STUN Server

![STUN CI](https://github.com/fractalcomputers/STUN-server/workflows/STUN%20CI/badge.svg)

This repository contains the implementation of the Fractal STUN server that facilitates a connection between a streaming server, typically a Fractal VM/container, and a client device. The STUN server is necessary for enabling connections between devices behind NATs, but can be used for non-NAT devices as well.

Fractal STUN server(s) are all hosted on AWS Lightsail instances running Ubuntu 18.04.

For further documentation, check this repository's [Wiki](https://github.com/fractalcomputers/STUN-server/wiki). 

## Development

### Creating new STUN servers

- Create an Ubuntu 18.04 OS-only instance from the AWS Lightsail platform. You should have your own AWS IAM and can use it to log in. If you don't have one, contact Phil.
- Create a Static IP and attach it to your instance.
- Go to your instance's networking and open SSH on the regular port 22, and TCP and UDP on port 48800. You should remove everything else.

You then need to install dependencies and this codebase. You can do so via:

```
sudo apt-get update
sudo apt-get install git make g++ immortal
curl -s https://packagecloud.io/install/repositories/immortal/immortal/script.deb.sh | sudo bash
```

You can also install git, clone this repository and run `./setup.sh` -- this will take care of installing all those dependencies.

### Building & Running

You can build the code by typing `make`. 

The STUN server(s) are meant to run 24/7. We use Immortal as a process manager to immediately restart after a crash. When starting a STUN server for production usage, you should start it with:

```
immortal ./server
```

It will run in the background and restart automatically if it exits. To see if it is running, you can run `immortalctl`, which prints the running jobs and their names & PID. A proccess can be shutdown via `immortalctl -k <process-name>`. 

We have continuous integration set up in this project, using GitHub Actions. When a push or PR happens on branch `master` or `dev`, the executable will get compiled on Ubuntu and `clang-format` will be run, which will prompt you to format your code if it isn't formatted. It will also run unit and integration tests using Unity, including testing UDP and TCP connectivity. You can see those in the `/tests` folder. You should make sure that your commit passes the tests under the Actions tab before merging a pull request, if you are contributing.

## Publishing & Updating

Currently, we do not have an automated way to replace the STUN server in AWS Lightsail other than manually taking it down via SSH through the Lightsail portal, `git pull/make` and starting the new version. Once you have updated the production code, you should run `./update.sh` to notify the Fractal team via Slack.  

## Styling

We use `clang-format` for the coding style on this repository. We have [pre-commit hooks](https://pre-commit.com/) with `clang-format` support installed on this project, which you can initialize by first installing pre-commit via `pip install pre-commit` and then running `pre-commit install` to instantiate the hooks for clang-format.

You can always run `clang-format -i <file>` to run it on a specific file, or `clang-format -i *.cpp` to run it on all C++ files, for example.
