#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define DIM 16 
#define CLASSES	1024


void radiusCPU(
		const float *class_centres,
		const float *class_radii2,
		const float *data,
		int *result) {

    for(int j = 0; j < CLASSES; j++) {
    	float dist2 = 0.0;
    	for(int i = 0 ; i < DIM; i++) {
    		dist2 += (class_centres[i + DIM*j] - data[i])*(class_centres[i + DIM*j] - data[i]);
    	}
    	result[j] = (dist2 < class_radii2[j])? j : -1;
    }
}

int main(int argc, char **argv) {

	int POINTS = atoi(argv[1]);
	srand(100);
	float *classes = (float *)malloc(DIM*CLASSES*SIZE_FLOAT);
	float *data = (float *)malloc(POINTS*DIM*SIZE_FLOAT);
	float *radii2 = (float *)malloc(CLASSES*SIZE_FLOAT);
	int *out = (int *)malloc(POINTS*CLASSES*sizeof(int));

	for(int i = 0; i < CLASSES*DIM; i++) {
		classes[i] = (float) rand() / 100000000;
	}

	for(int i = 0; i < DIM*POINTS; i++){
			data[i] = (float) rand() / 100000000;
	}

	for(int i = 0; i < CLASSES; i++) {
		radii2[i] = (float) rand() / 100000000;
		radii2[i] = radii2[i] * radii2[i];
	}

	for(int i = 0 ; i < CLASSES*POINTS; i++) {
		out[i] = 0;
	}
	for(int i = 0; i < POINTS; i++) {
		radiusCPU(classes, radii2, &data[DIM*i], &out[CLASSES*i]);
	}
	int total_in_class = 0;
	int total_in_multiple = 0;
	for(int i = 0; i < CLASSES*POINTS; i+=CLASSES){
		int point = i / CLASSES;
		int *result = &out[i];
                int in_class = 0;
                int in_multiple = 0;
		for(int j =  0; j  < CLASSES;j++){
		    if(result[j] != -1){
   			 if (in_class == 1) { in_multiple  = 1; }
                         if (in_class == 0) { in_class = 1; }			 
                    }
                } 
                if (in_class) { total_in_class++; }
                if (in_multiple) { total_in_multiple++; }
	}

    printf("in class=%d multiple=%d\n", total_in_class, total_in_multiple);

	free(classes);
	free(data);
	free(radii2);
	free(out);

	return 0;
}
