#!/bin/bash

git submodule update --init --recursive
./scripts/download_dependencies.sh
source ./scripts/env_vars.sh
./scripts/autogen.sh
make -j $(nproc) -f Makefile.prospero libpsbc
./scripts/package_lib.sh
