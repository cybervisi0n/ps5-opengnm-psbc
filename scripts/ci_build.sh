#!/bin/bash

git submodule update --init --recursive
./scripts/download_dependencies.sh
source ./scripts/env_vars.sh
./scripts/autogen.sh
make -f Makefile.prospero libpsbc
# for some reason the autogen files arent inclued in the lib unless you run make a second time :(
make -f Makefile.prospero libpsbc
./scripts/package_lib.sh
