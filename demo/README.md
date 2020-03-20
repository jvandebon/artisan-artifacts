
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

4. Move to the adprecitor application directory: ```cd apps/adpredictor```
5. Optimise the application: ```python3 ../../metaprograms/udt/udt_openmp.py```
6. Run the optimised code: ```cd openmp_project ; make ; make run```

The UDT metaprogram in step 5 automatiically optimisess the application as follows:
- The input program source (```/apps/adpredictor/main.cpp```) is instrumented with timers on each outer loop, built, and run to identify a program hotspot comprising > 50% of the overall execution time. 
- The result of this analysis will be stored in the subdirectory ```/hotspot-result``` (see ```/hotspot-result/loop_times.json``` and ```hotspot-result/hotspots.json``` for the reports).
- This hotspot loop is extracted into a separate function, and the code is instrumented to insert an OpenMP pragma above the loop instructing it to run with 8 threads. 
- The resulting optimised code will be in the ```/openmp_project``` diriectory. 

## Demo 2: Single Work Item FPGA Kernel Optimisation (U-2)

### Motivation

### Requirements
- Artisan and Docker 
- Intel Quartus Prime 19.3+ with support for OpenCL FPGA SDK for and Intel HLS installed on host machine

### Steps
