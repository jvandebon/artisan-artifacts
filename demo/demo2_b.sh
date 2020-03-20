#!/bin/bash
cd apps/adp_test/swi_project/project

cd device/lib ; make clean ; make   # build HLS kernel
cd ../..
source build_kernel.sh              # synthesize hardware
source program_device.sh            # program FPGA
make                                # build host

./bin/host 8388608 `pwd`/../../../inputs/adpredictor/   # run program