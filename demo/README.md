
For both demos, you will need to have artisan installed locally and be running inside the artisan Docker container. For installation, follow the steps on the [artisan repository](https://github.com/ckiddo74/artisan). You will also need this repository, if you haven't already done so, clone: ```git clone git@github.com:jvandebon/artisan-artefacts.git```.

![Workflow](https://github.com/jvandebon/artisan-artefacts/blob/master/demo/workflow.png)

## Demo 1: Multi-Threaded CPU Optimisaton (U-1)

### Motivation
This demonstration shows how to automatically optimise a C++ application (AdPredictor) for a multi-CPU target platform using OpenMP. 

### Requirements
- Artisan and Docker 

### Steps

1. Move into the base directory: ```cd artisan-artefacts```
2. Start the artisan Docker container: ```artisan```
3. Run the provided script, ```source demo/demo1.sh``` to automatically execute steps 4-6, or continue manually as follows:

4. Move to the adpredictor application directory: ```cd apps/adpredictor```
5. Optimise the application: ```python3 ../../metaprograms/udt/udt_openmp.py```
6. Run the optimised code: ```cd openmp_project ; make ; make run```

The UDT metaprogram in step 5 automatically optimises the application as follows:
- The input program source (```/apps/adpredictor/main.cpp```) is instrumented with timers on each outer loop, built, and run to identify a program hotspot comprising > 50% of the overall execution time. 
- The result of this analysis will be stored in the subdirectory ```/hotspot-result``` (see ```/hotspot-result/loop_times.json``` and ```hotspot-result/hotspots.json``` for the reports).
- This hotspot loop is extracted into a separate function, and the code is instrumented to insert an OpenMP pragma above the loop instructing it to run with 8 threads. 
- The resulting optimised code will be in the ```/openmp_project``` directory. 

## Demo 2: Single Work Item FPGA Kernel Optimisation (U-2)

### Motivation

This demonstration shows how to automatically optimise a C++ application (AdPredictor) for an CPU+FPGA target platform using a single work-item FPGA kernel with unrolled loops to maximise pipeline parallelism without overmapping FPGA resources.

### Requirements
- Artisan and Docker 
- Intel Quartus Prime 19.3+ with support for OpenCL FPGA SDK for and Intel HLS installed on host machine
- Arria10 GX Development Kit 

### Steps

1. Move into the base directory: ```cd artisan-artefacts```
2. Start the artisan Docker container: ```artisan```

3. Run the provided script, ```source demo/demo2_a.sh``` to automatically execute steps 4 and 5, or continue manually as follows:

4. Move to the adpredictor application directory: ```cd apps/adpredictor```
5. Optimise the application: ```python3 ../../metaprograms/udt/udt_swi_unroll.py```

6. Exit from the Docker environment: ```exit```.  The remaining steps should be executed on the host machine. 

7. Run the provided script, ```source demo/demo2_b.sh``` to automatically execute steps 8-12, or continue manually as follows:

8. Build the HLS kernel: ```cd swi_project/project/device/lib ; make clean ; make```
9. Synthesize the design: ```cd apps/adpredictor/swi_project/project ; source build_kernel.sh``` (note: may take hours)
10. Program the FPGA: ```source program_device.sh```
11. Build the host application: ```make```
12. Run the synthesized hardware: ```./bin/host 8388608 `pwd`/../../../inputs/adpredictor/```

The UDT metaprogram in step 5 automatically optimises the application as follows:
- The input program source (```/apps/adpredictor/main.cpp```) is instrumented with timers on each outer loop, built, and run to identify a program hotspot comprising > 50% of the overall execution time. 
- The result of this analysis will be stored in the subdirectory ```/hotspot-result``` (see ```/hotspot-result/loop_times.json``` and ```hotspot-result/hotspots.json``` for the reports).
- A single work item FPGA project is generated based on the identified hotspot. This includes:
    - HLS kernel containing the hotspot function (```/swi_project/project/device/lib/library.cpp```)
    - OpenCL kernel manager interfacing the host application and HLS kernel (```/swi_project/project/device/kernel.cl```) 
    - C++ host application (```/swi_project/project/host/src/*```)
- The HLS kernel is instrumented to unroll all fixed bounds loops by some factor, starting at 2 and doubled iteratively in a design space exploration process
- For each factor, intermediate RTL compilation is performed to generate design reports which are parsed to obtain resource utilisation estimates
- The unroll factor is fixed to the value that maximises resource utilisation without overmapping the device
- The resulting optimised code will be in the ```/swi_project``` directory. 
