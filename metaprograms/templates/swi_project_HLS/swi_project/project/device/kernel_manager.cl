extern void lib_func(*FUNC_ARGS*); 

kernel void kernel_lib(*KERNEL_ARGS*){
    lib_func(*RUNTIME_ARGS*);
}