#!/bin/bash

# This should run inside the docker container

# Add a package using mldock
RUN mldock install https://github.com/flatironinstitute/mountainsort#master:packages/ms3 ms3
RUN mp-list-processors

