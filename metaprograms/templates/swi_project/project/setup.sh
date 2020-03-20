#!/bin/bash

INTEL_FPGA_ROOT=/home/cc/intelFPGA_pro/19.3
export QUARTUS_ROOTDIR=$INTEL_FPGA_ROOT/quartus
export QUARTUS_ROOTDIR_OVERRIDE=$INTEL_FPGA_ROOT/quartus

export PATH=$PATH:$QUARTUS_ROOTDIR/bin
export PATH=$PATH:$INTEL_FPGA_ROOT/modelsim_ase/bin

export AOCL_BOARD_PACKAGE_ROOT=$INTEL_FPGA_ROOT/hld/board/a10_ref

source $INTEL_FPGA_ROOT/hls/init_hls.sh
source $INTEL_FPGA_ROOT/hld/init_opencl.sh