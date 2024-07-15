# Yoinked from https://github.com/lukstep/raspberry-pi-pico-docker-sdk/blob/main/Dockerfile

FROM ubuntu:22.04

ARG DEBIAN_FRONTEND=noninteractive

# Install toolchain
RUN apt update && \
    apt upgrade && \
    apt install -y git \
                python3 \
                python3-pip \
                build-essential \
                cmake \
                libusb-dev \
                gcc-arm-none-eabi \
                gdb-multiarch

RUN apt install pkg-config

# Raspberry Pi Pico SDK
ARG SDK_PATH=/usr/local/picosdk
RUN git clone https://github.com/raspberrypi/pico-sdk $SDK_PATH && \
    cd $SDK_PATH && \
    git submodule update --init
    

ENV PICO_SDK_PATH=$SDK_PATH

RUN apt install -y libusb-1.0-0-dev

# Picotool installation
RUN git clone https://github.com/raspberrypi/picotool.git /home/picotool && \
    cd /home/picotool && \
    mkdir build && \
    cd build && \
    cmake .. && \
    make && \
    cp /home/picotool/build/picotool /bin/picotool && \
    rm -rf /home/picotool

RUN apt install -y tmux
