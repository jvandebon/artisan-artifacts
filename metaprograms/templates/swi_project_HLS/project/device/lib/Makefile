HLS_INC=/home/cc/intelFPGA_pro/19.3/hls/include
GCC_TOOLCHAIN=--gcc-toolchain=/home/cc/intelFPGA_pro/19.3/hls/gcc/
library.aoclib: library.aoco
	fpga_libtool  --target aoc --create library.aoclib library.aoco

library.aoco: library.cpp
	fpga_crossgen  --target aoc $(GCC_TOOLCHAIN) -I $(HLS_INC) library.cpp 

clean:
	rm *.aoco *.i *.aoclib