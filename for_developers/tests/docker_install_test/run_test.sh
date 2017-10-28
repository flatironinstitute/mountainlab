#!/bin/bash

# If you are not authorized to run docker, you must run this command with sudo privileges
# (But then the fresh_clone directory will be owned by root -- any advice from anyone?)

set -e

if [ -d "fresh_clone" ]; then
  rm -r fresh_clone
fi
mkdir fresh_clone
git clone https://github.com/flatironinstitute/mountainlab fresh_clone/mountainlab
docker build -t mountainlab_install_test .

docker run mountainlab_install_test