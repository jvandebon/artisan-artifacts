    #pragma artisan-hls parallel { "is_parallel" : "True" }
//*********************************************************************//
// N-Body Simulation
//
// Author:  Maxeler Technologies
//
// Imperial College Summer School, July 2012
//
//*********************************************************************//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>
#include <unistd.h>
#include "typedefs.h"


#pragma artisan-hls header {"typedefs.h":"./"}
// /**
//  * 3-D coordinates
//  */
// typedef struct {
//     float x;
//     float y;
//     float z;
// } coord3d_t;

// /** Descriptor of a particle state */
// typedef struct {
//     coord3d_t p;
//     coord3d_t v;
// } particle_t;


/**
 * \brief Run the N-body simulation on the CPU.
 * \param [in]  n               Number of particles
 * \param [in]  EPS             Damping factor
 * \param [in]  m               Masses of the N particles
 * \param [in]  in_particles    Initial state of the N particles
 * \param [out] out_particles   Final state of the N particles after nt time-steps
 */

// #define EPS 100

void run_cpu(int n, const float *m,
        const particle_t *in_particles, particle_t *out_particles)
{
    particle_t *p = (particle_t *) malloc(n * sizeof(particle_t));
    memcpy(p, in_particles, n * sizeof(particle_t));

    coord3d_t *a = (coord3d_t *) malloc(n * sizeof(coord3d_t));

    memset(a, 0, n * sizeof(coord3d_t));
    

    for (int q = 0; q < n; q++) {
        #pragma artisan-hls parallel { "is_parallel" : "True" }
        for (int j = 0; j < n; j++) {
            float rx = rx = p[j].p.x - p[q].p.x;
            float ry = p[j].p.y - p[q].p.y;
            float rz = p[j].p.z - p[q].p.z;
            float dd = rx*rx + ry*ry + rz*rz + EPS;
            float d = 1/ (dd*sqrt(dd));
            float s = m[j] * d;
            a[q].x += rx * s;
            a[q].y += ry * s;
            a[q].z += rz * s;
        }
    }

    for (int i = 0; i < n; i++) {
        p[i].p.x += p[i].v.x;
        p[i].p.y += p[i].v.y;
        p[i].p.z += p[i].v.z;
        p[i].v.x += a[i].x;
        p[i].v.y += a[i].y;
        p[i].v.z += a[i].z;
    }

    memcpy(out_particles, p, n * sizeof(particle_t));

    free(p);
    free(a);
}

int main(int argc, char **argv)
{

    int n = atoi(argv[1]);

    particle_t *particles;
    float *m;

    particles = (particle_t *) malloc(n * sizeof(particle_t));
    m = (float *) malloc(n * sizeof(float));

    srand(100);
    for (int i = 0; i < n; i++)
    {
        m[i] = (float)rand()/100000;
        particles[i].p.x = (float)rand()/100000;
        particles[i].p.y = (float)rand()/100000;
        particles[i].p.z = (float)rand()/100000;
        particles[i].v.x = (float)rand()/100000;
        particles[i].v.y = (float)rand()/100000;
        particles[i].v.z = (float)rand()/100000;
    }

    particle_t *out_particles = NULL;
    out_particles = (particle_t *) malloc(n * sizeof(particle_t));

    printf("Running on CPU with %d particles...\n", n);
    struct timeval t1, t2;
    gettimeofday(&t1, NULL);
    run_cpu(n, m, particles, out_particles);
    gettimeofday(&t2, NULL);
    printf("CPU computation took %lfs\n", (double)(t2.tv_usec-t1.tv_usec)/1000000 + (double)(t2.tv_sec-t1.tv_sec));

    free(particles);
    free(m);
    free(out_particles);

    return 0;
}
