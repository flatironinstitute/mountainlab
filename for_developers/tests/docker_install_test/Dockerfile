# Set the base image to Ubuntu
FROM ubuntu:16.04

MAINTAINER Jeremy Magland

# Install qt5
RUN apt-get update && apt-get install -y software-properties-common
RUN apt-get update && apt-add-repository ppa:ubuntu-sdk-team/ppa
RUN apt-get update && apt-get install -y qtdeclarative5-dev qt5-default qtbase5-dev qtscript5-dev make g++

# Install nodejs and npm
RUN apt-get update && apt-get install -y nodejs npm

# Install python3, python3-pip, and packages
RUN apt-get update && apt-get install -y python3 python3-pip
RUN pip3 install numpy scipy pybind11 cppimport

# Install docker (optional) for using mldock
RUN apt-get update && apt-get install -y docker.io

# Clone a fresh version of this repo
ADD fresh_clone/mountainlab /mountainlab

# Build
WORKDIR /mountainlab
RUN ./compile_components.sh
ENV PATH="/mountainlab/bin:${PATH}"

# Test
RUN mproc --help
RUN mp-list-processors

# Note: run container using --privileged=true in order to run docker inside docker (mldock uses docker)
CMD service docker start && mldock install https://github.com/flatironinstitute/mountainsort#master:packages/ms3 ms3 && mp-list-processors
