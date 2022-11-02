#!/usr/bin/env bash

CUDA_VERSION=11.1.0

# Load packages
module load cuda/${CUDA_VERSION}
module load python/3.8.2

# Create a Python env for the project.
if [ ! -d "env" ]; then
  echo "Virtualenv `env` not created. Creating one!!"
  python3 -m virtualenv env
fi

source ./env/bin/activate

# Activate spack
# NOTE: this only works locally.
. /g/g91/kesavan/spack/share/spack/setup-env.sh
export PATH=/g/g91/kesavan/spack/bin:$PATH

# Install fmt and googletest -> dependencies.
# TODO (surajk): Do not install if already installed.
spack install fmt@9.0.0 googletest@1.8.1 py-pybind11
spack load fmt googletest/gdlwgea py-pybind11/lwmygkz

# Find CUDA source path
CUDA_SOURCE_DIR=$(which nvcc | cut -d'/' -f-6)
echo "FOUND CUDA: " ${CUDA_SOURCE_DIR}

# Export variables
# TODO (surajk): Hard coded here because module load is inconsistent!
export CUDA_SOURCE_DIR=/usr/tce/packages/cuda/cuda-${CUDA_VERSION}
export FMT_SOURCE_DIR=`spack location -i fmt`
export GOOGLETEST_SOURCE_DIR=`spack location -i googletest`
export PYBIND_SOURCE_DIR=`spack location -i py-pybind11`

# For debugging.
echo "CUDA_SOURCE_DIR        = $CUDA_SOURCE_DIR"
echo "FMT_SOURCE_DIR         = $FMT_SOURCE_DIR"
echo "GOOGLETEST_SOURCE_DIR  = $GOOGLETEST_SOURCE_DIR"
echo "PYBIND_SOURCE_DIR      = $PYBIND_SOURCE_DIR"

# Clean up old build, if it exists.
rm -rf CMakeFiles CMakeCache.txt compile_commands.json cmake_install.cmake Makefile libdmv.a

# Load required module/version
module load spectrum-mpi/rolling-release
module load cmake/3.16.8
module load gcc/8.3.1
module load python/3.8.2

# Force the compiler to pick the correct versions.
export CC=`which gcc`
export CXX=`which g++`

# Compile the sources
cmake . -DCUDA_SOURCE_DIR=$CUDA_SOURCE_DIR -DFMT_SOURCE_DIR=$FMT_SOURCE_DIR -DGOOGLETEST_SOURCE_DIR=$GOOGLETEST_SOURCE_DIR

# Build!
cmake --build .

# Add repo to the env variable.
export DMV_SOURCE_DIR=$(pwd)
echo "DMV_SOURCE_DIR        = $DMV_SOURCE_DIR"
