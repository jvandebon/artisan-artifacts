#include <stdlib.h>
#include <cstdio>
#include <iostream>
#include <unistd.h>
#include <sys/time.h>
#include <cmath>
#include <stdint.h>
#include <fstream>

#pragma artisan-hls types {"uint64_t": "unsigned int"}

#define NUM_FEATURES 12
#define VOL_INC 50000

#define root2pi 2.50662827463100050242
#define p0 220.2068679123761
#define p1 221.2135961699311
#define p2 112.0792914978709
#define p3 33.91286607838300
#define p4 6.373962203531650
#define p5 0.7003830644436881
#define p6 0.3526249659989109E-01
#define q0 440.4137358247522
#define q1 793.8265125199484
#define q2 637.3336333788311
#define q3 296.5642487796737
#define q4 86.78073220294608
#define q5 16.06417757920695
#define q6 1.755667163182642
#define q7 0.8838834764831844E-1
#define cutoff 7.071
#define beta 0.001

using namespace std;


void check_results(float *post_m, float *post_s, int volume);

float PDF(float z) {
   return exp(-z * z / 2) / root2pi;
}
float CDF(float z) {
        float zabs = fabs(z);
        float expntl = exp(-.5*zabs*zabs);
        float pdf = expntl/root2pi;

        int c1 = z > 37.0;
        int c2 = z < -37.0;
        int c3 = zabs < cutoff;
        float pA =  expntl*((((((p6*zabs + p5)*zabs + p4)*zabs + p3)*zabs +
          p2)*zabs + p1)*zabs + p0)/(((((((q7*zabs + q6)*zabs +
          q5)*zabs + q4)*zabs + q3)*zabs + q2)*zabs + q1)*zabs +
          q0);
   float pB = pdf/(zabs + 1.0/(zabs + 2.0/(zabs + 3.0/(zabs + 4.0/
          (zabs + 0.65)))));

   float pX = c3? pA : pB;
   float p = (z < 0.0) ? pX : 1 - pX;
   return c1? 1.0 : (c2 ? 0.0 : p);
}

float V (float t) {
   float cdf = CDF(t);
   int c0 = (cdf == 0.0);
   return c0? 0.0 : (PDF(t) / cdf);
}

float W (float t) {
   float v = V(t);
   return v * (v + t);
}

int main(int argc, char *argv[]) {
  int volume = atoi(argv[1]);
  string inputs = argv[2];

  string pm_file = inputs+"/prior_m";
  string pv_file = inputs+"/prior_v";
  string y_file = inputs+"/y";

  uint64_t total_size = volume * NUM_FEATURES;
  float *prior_m = new float[total_size];
  float *prior_v = new float[total_size];
  float *post_m = new float[total_size];
  float *post_s = new float[total_size];
  float *y = new float[volume];

  FILE *fp_y = fopen(y_file.c_str(),"r");
  FILE *fp_pm = fopen(pm_file.c_str(),"r");
  FILE *fp_pv = fopen(pv_file.c_str(), "r");

  /* read from files in chunks */
  int remaining = volume;
  int done = 0;

  for(int k = 0; k <= volume/VOL_INC; k++){
    int current = (remaining > VOL_INC) ? VOL_INC : remaining;
    if (current == 0){
      break;
    }
    int status;
    if ( (status = fread(&y[done], sizeof(float), current, fp_y)) != current){
      printf("Something went wrong reading y: %d.\n", status);
    } 
    if ( (status = fread(&prior_m[done*NUM_FEATURES], sizeof(float), current*NUM_FEATURES, fp_pm)) != current*NUM_FEATURES) {
      printf("Something went wrong reading prior_m: %d.\n", status);
    }
    if ( (status = fread(&prior_v[done*NUM_FEATURES], sizeof(float), current*NUM_FEATURES, fp_pv)) != current*NUM_FEATURES) {
      printf("Something went wrong reading prior_v: %d.\n", status);
    }
    done += current;
    remaining -= current;
  }

  fclose(fp_y); fclose(fp_pm); fclose(fp_pv);
  fp_y = NULL; fp_pm = NULL; fp_pv = NULL;

  for (uint64_t st = 0; st < volume; st++) {
    #pragma artisan-hls parallel {"is_parallel": "True"}
    #pragma artisan-hls vars {"prior_m": "R", "prior_v": "R", "y": "R", "post_m" : "W", "post_s": "W"}
    float s = 0.0;
    float m = 0.0;
    for (int i = 0; i < NUM_FEATURES; i++) {
      uint64_t idx = st * NUM_FEATURES + i;
      m += prior_m[idx];
      s += prior_v[idx];
    }
    float S = sqrt(beta * beta + s);
    float t = (y[st] * m) / S;
    for (int i = 0; i < NUM_FEATURES; i++) {
      post_m[st * NUM_FEATURES + i] = prior_m[st * NUM_FEATURES + i] + y[st] * (prior_v[st * NUM_FEATURES + i] / S) *  V(t);
      post_s[st * NUM_FEATURES + i] = sqrt(fabs(prior_v[st * NUM_FEATURES + i] * (1 - (prior_v[st * NUM_FEATURES + i] / (S * S)) *  W(t))));
    }
  }
  printf("Checking results...\n");
  check_results(post_m, post_s, volume);

  delete[] prior_m;
  delete[] prior_v;
  delete[] post_m;
  delete[] post_s;

  return 0;
}

// check results
void check_results(float *post_m, float *post_s, int volume)
{
  std::ofstream ofile;
  ofile.open("outputs.txt");
  if (ofile.is_open())
  {
    ofile << "\nst | i | post_m | post_s \n";
    for (uint st = 0; st < volume; st++) {
      for (int i = 0; i < 12; i++) {
        uint idx = st * 12 + i;
        if (st % (volume / 10) == 0 && i == 0) {
          ofile << st << " " << i << " " << post_m[idx] << " " << post_s[idx] << std::endl; 
        }
      }
    }
  }
  else
  {
    std::cout << "Failed to create output file!" << std::endl;
  }
}
