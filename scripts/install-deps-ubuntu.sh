#!/bin/bash

UBUNTU_CODENAME=$(lsb_release -cs)

apt-get update

apt-get install -y      \
        wget            \
        libx11-dev      \
        libxrandr-dev   \
        libxinerama-dev \
        libxcursor-dev  \
        libxi-dev       \
        vulkan-tools    \
        doxygen         \
        dotter

wget -qO - http://packages.lunarg.com/lunarg-signing-key-pub.asc | apt-key add -
wget -qO /etc/apt/sources.list.d/lunarg-vulkan-$UBUNTU_CODENAME.list http://packages.lunarg.com/vulkan/lunarg-vulkan-$UBUNTU_CODENAME.list
apt-get update
apt-get install -y vulkan-sdk

