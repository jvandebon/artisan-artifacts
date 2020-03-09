cd device/lib
make
cd ../..
rm -r bin/kernel/reports
aoc -rtl device/kernel.cl -l device/lib/library.aoclib -o bin/kernel.aocr -parallel=7 -board=a10gx

 