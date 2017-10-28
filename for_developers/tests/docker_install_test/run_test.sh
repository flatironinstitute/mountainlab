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

# --privileged=true is needed in order to run docker inside docker
docker run mountainlab_install_test for_developers/tests/docker_install_test/test_script.sh