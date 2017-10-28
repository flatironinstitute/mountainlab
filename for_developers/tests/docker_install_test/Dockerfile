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

# Install a package
RUN apt-get update && apt-get install -y libfftw3-dev
RUN pip3 install sklearn
# Force to rebuild from this point forward so we can always get the latest source code from the plugin package repo (In the docker build command, use --build-arg FORCE_REBUILD_STRING="$(date)")
ARG FORCE_REBUILD_STRING=default
RUN export FORCE_REBUILD_STRING="$FORCE_REBUILD_STRING"
RUN cd packages && git clone https://github.com/flatironinstitute/mountainsort && cd mountainsort && ./compile_components.sh
RUN mp-list-processors

# Test to see if processors are all present
CMD mp-list-processors
