#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <math.h>
#include <fstream>
#include <stdint.h>

#define NUM_FEATURES 12
#define SQRT_2PI 2.50662827463100050242

using namespace std;

double PDF(double x) {
  return exp(-x * x / 2) / SQRT_2PI; 
}
double PDF(double x, double m, double s) {
  return PDF((x - m) / s) / s;
}
double CDF(double x) {
  if (x < -8.0) {
    return 0.0;
  }
  if (x > 8.0){
    return 1.0;
  }
  double s = 0.0, t = x;
  for (int i = 3; s + t != s; i += 2) {
    s = s + t;
    t = t * x * x / i;
  }
  return 0.5 + s * PDF(x);
}

double CDF(double x, double m, double s) { return CDF((x - m) / s); }

float V(float t) {
  float cdf =  CDF(t, 0, 1);
  if (cdf == 0.0) {
    return 0.0;
  }

  return  PDF(t, 0, 1) / cdf;
}

float W(float t) {
  float v = V(t);
  return v * (v + t);
}

static float frand(int min, int max) {
  return min + (float)rand() / ((float)RAND_MAX / (max - min));
}

static float rand_y() { return (rand() % 2) ? 1.0 : -1.0; }
static float rand_m() { return frand(-6, 6); }
static float rand_v() { return frand(0, 100); }

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

  ifstream pmfile(pm_file.c_str());
  ifstream pvfile(pv_file.c_str());
  ifstream yfile(y_file.c_str());

  for (uint64_t i = 0; i < volume; i++) {
    yfile >> y[i];
    uint64_t base = i * NUM_FEATURES;
    for (int j = 0; j < NUM_FEATURES; j++) {
      pmfile >> prior_m[base + j];
      pvfile >> prior_v[base + j];
    }
  }

  pvfile.close();
  pmfile.close();
  yfile.close();
  
  for (uint64_t st = 0; st < volume; st++) {
    float s = 0.0;
    float m = 0.0;
    for (int i = 0; i < NUM_FEATURES; i++) {
      uint64_t idx = st * NUM_FEATURES + i;
      m += prior_m[idx];
      s += prior_v[idx];
    }
    float beta = 0.001;
    float S = sqrt(beta * beta + s);
    float t = (y[st] * m) / S;
    for (int i = 0; i < NUM_FEATURES; i++) {
      post_m[st * NUM_FEATURES + i] = prior_m[st * NUM_FEATURES + i] + y[st] * (prior_v[st * NUM_FEATURES + i] / S) *  V(t);
      post_s[st * NUM_FEATURES + i] = sqrt(fabs(prior_v[st * NUM_FEATURES + i] * (1 - (prior_v[st * NUM_FEATURES + i] / (S * S)) *  W(t))));
    }
  }

  delete[] prior_m;
  delete[] prior_v;
  delete[] post_m;
  delete[] post_s;

  return 0;
}
