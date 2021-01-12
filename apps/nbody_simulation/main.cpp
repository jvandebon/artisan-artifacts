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
#include <fstream>
#include <iostream>
#pragma artisan-hls header {"file":"typedefs.h", "path":"./"}

/**
 * \brief Run the N-body simulation on the CPU.
 * \param [in]  n               Number of particles
 * \param [in]  EPS             Damping factor
 * \param [in]  m               Masses of the N particles
 * \param [in]  in_particles    Initial state of the N particles
 * \param [out] out_particles   Final state of the N particles after nt time-steps
 */

void check_results(particle_t *particles, particle_t *out_particles, int n);
void run_cpu(int n, const float *m,
        const particle_t *in_particles, particle_t *out_particles)
{
    particle_t *p_ = (particle_t *) malloc(n * sizeof(particle_t));
    memcpy(p_, in_particles, n * sizeof(particle_t));

    coord3d_t *a_ = (coord3d_t *) malloc(n * sizeof(coord3d_t));

    memset(a_, 0, n * sizeof(coord3d_t));
    
    for (int q = 0; q < n; q++) {
        #pragma artisan-hls parallel { "is_parallel" : "True" }
        for (int j = 0; j < n; j++) {
            float rx = p_[j].p.x - p_[q].p.x;
            float ry = p_[j].p.y - p_[q].p.y;
            float rz = p_[j].p.z - p_[q].p.z;
            float dd = rx*rx + ry*ry + rz*rz + EPS;
            float d = 1/ (dd*sqrt(dd));
            float s = m[j] * d;
            a_[q].x += rx * s;
            a_[q].y += ry * s;
            a_[q].z += rz * s;
        }
    }

    for (int i = 0; i < n; i++) {
        p_[i].p.x += p_[i].v.x;
        p_[i].p.y += p_[i].v.y;
        p_[i].p.z += p_[i].v.z;
        p_[i].v.x += a_[i].x;
        p_[i].v.y += a_[i].y;
        p_[i].v.z += a_[i].z;
    }

    memcpy(out_particles, p_, n * sizeof(particle_t));

    free(p_);
    free(a_);
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

    printf("Checking results...\n");
    check_results(particles, out_particles, n);

    free(particles);
    free(m);
    free(out_particles);

    return 0;
}

void check_results(particle_t *particles, particle_t *out_particles, int n)
{
  std::ofstream ofile;
  ofile.open("outputs.txt");
  if (ofile.is_open())
  {
    for (uint i = 0; i < n; i++) {
        if (i%(n/10) == 0){
          ofile << "i = " << i << std::endl;
          ofile << "in particle position: " << particles[i].p.x << " " << particles[i].p.y << " " << particles[i].p.z << std::endl;
          ofile << "in particle velocity: " << particles[i].v.x << " " << particles[i].v.y << " " << particles[i].v.z << std::endl;
          ofile << "out particle position: " << out_particles[i].p.x  << " " << out_particles[i].p.y << " " << out_particles[i].p.z << std::endl;
          ofile << "out particle velocity: " << out_particles[i].v.x  << " " << out_particles[i].v.y << " " << out_particles[i].v.z << std::endl;
        }
      
    }
  }
  else
  {
    std::cout << "Failed to create output file!" << std::endl;
  }
}