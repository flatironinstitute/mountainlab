#!/bin/bash

set -e

if [ -d "fresh_clone" ]; then
  rm -r fresh_clone
fi
mkdir fresh_clone
git clone https://github.com/flatironinstitute/mountainlab fresh_clone/mountainlab
docker build -t mountainlab_install_test .