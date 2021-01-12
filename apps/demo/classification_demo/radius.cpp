#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define DIM 16 
#define CLASSES	1024

void radiusCPU(const float *class_centres,const float *class_radii2,const float *data,int *result) {

    for(int j = 0; j < CLASSES; j++) {
    	float dist2 = 0.0;
    	for(int i = 0 ; i < DIM; i++) {
    		dist2 += (class_centres[i + DIM*j] - data[i])*(class_centres[i + DIM*j] - data[i]);
    	}
    	result[j] = (dist2 < class_radii2[j])? j : -1;
    }
}

int main(int argc, char **argv) {

	int points = atoi(argv[1]);
	srand(100);

	float *classes = (float *)malloc(DIM*CLASSES*sizeof(float));
	float *data = (float *)malloc(points*DIM*sizeof(float));
	float *radii2 = (float *)malloc(CLASSES*sizeof(float));
	int *out = (int *)malloc(points*CLASSES*sizeof(int));

	for(int i = 0; i < CLASSES*DIM; i++) {
		classes[i] = (float) rand() / 100000000;
	}
	for(int i = 0; i < DIM*points; i++){
		data[i] = (float) rand() / 100000000;
	}
	for(int i = 0; i < CLASSES; i++) {
		radii2[i] = (float) rand() / 100000000;
		radii2[i] = radii2[i] * radii2[i];
	}
	for(int i = 0 ; i < CLASSES*points; i++) {
		out[i] = 0;
	}
	for(int i = 0; i < points; i++) {
		#pragma artisan-hls vars { "classes": "R", "radii2": "R", "data": "R", "out": "W"}
		#pragma artisan-hls parallel { "is_parallel" : "True" }
		radiusCPU(classes, radii2, &data[DIM*i], &out[CLASSES*i]);
	}
	return 0;
}
