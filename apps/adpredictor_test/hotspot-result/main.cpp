#define __ARTISAN__INIT__
#include <artisan.hpp>
#include <stdlib.h>
#include <cstdio>
#include <iostream>
#include <unistd.h>
#include <sys/time.h>
#include <cmath>
#include <stdint.h>
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

float PDF(float z) {
   return ((std::exp(-z * z / 2)) / 2.50662827463100050242);
}


float CDF(float z) {
   float zabs = std::fabs(z);
   float expntl = (exp(- .5 * zabs * zabs));
   float pdf = (expntl / 2.50662827463100050242);
   float c1 = (z > 37.0);
   float c2 = (z < - 37.0);
   float c3 = (zabs < 7.071);
   float pA = (expntl * ((((((0.3526249659989109E-01 * zabs + 0.7003830644436881) * zabs + 6.373962203531650) * zabs + 33.91286607838300) * zabs + 112.0792914978709) * zabs + 221.2135961699311) * zabs + 220.2068679123761) / (((((((0.8838834764831844E-1 * zabs + 1.755667163182642) * zabs + 16.06417757920695) * zabs + 86.78073220294608) * zabs + 296.5642487796737) * zabs + 637.3336333788311) * zabs + 793.8265125199484) * zabs + 440.4137358247522));
   float pB = (pdf / (zabs + 1.0 / (zabs + 2.0 / (zabs + 3.0 / (zabs + 4.0 / (zabs + 0.65))))));
   float pX = c3?pA : pB;
   float p = z < 0.0?pX : 1 - pX;
   return (c1?1.0 : ((c2?0.0 : p)));
}


float V(float t) {
   float cdf = CDF(t);
   float c0 = (cdf == 0.0);
   return (c0?0.0 : (PDF(t) / cdf));
}


float W(float t) {
   float v = V(t);
   return v * (v + t);
}


int main_(int argc,char *argv[]) {
   Artisan::Timer __timer__("main", Artisan::op_add);
   int volume = atoi(argv[1]);
   std::__cxx11::string inputs = (argv[2]);
   std::__cxx11::string pm_file = inputs+"/prior_m";
   std::__cxx11::string pv_file = inputs+"/prior_v";
   std::__cxx11::string y_file = inputs+"/y";
   uint64_t total_size = (volume * 12);
   float *prior_m = new float [total_size];
   float *prior_v = new float [total_size];
   float *post_m = new float [total_size];
   float *post_s = new float [total_size];
   float *y = new float [volume];
   FILE *fp_y = fopen((y_file . c_str()),"r");
   FILE *fp_pm = fopen((pm_file . c_str()),"r");
   FILE *fp_pv = fopen((pv_file . c_str()),"r");
/* read from files in chunks */
   int remaining = volume;
   int done = 0;
   for (int k = 0; k <= volume / 50000; k++) {
      class Artisan::Timer __timer__(("main_for_a"),Artisan::op_add);
      int current = remaining > 50000?50000 : remaining;
      if (current == 0) {
         break;
      }
      int status;
      if ((status = (fread((&y[done]),sizeof(float ),current,fp_y))) != current) {
         printf("Something went wrong reading y: %d.\n",status);
      }
      if ((status = (fread((&prior_m[done * 12]),sizeof(float ),(current * 12),fp_pm))) != current * 12) {
         printf("Something went wrong reading prior_m: %d.\n",status);
      }
      if ((status = (fread((&prior_v[done * 12]),sizeof(float ),(current * 12),fp_pv))) != current * 12) {
         printf("Something went wrong reading prior_v: %d.\n",status);
      }
      done += current;
      remaining -= current;
   }
   fclose(fp_y);
   fclose(fp_pm);
   fclose(fp_pv);
   fp_y = 0L;
   fp_pm = 0L;
   fp_pv = 0L;
   for (::uint64_t st = 0; st < volume; st++) {
      class Artisan::Timer __timer__(("main_for_b"),Artisan::op_add);
      float s = 0.0;
      float m = 0.0;
      for (int i = 0; i < 12; i++) {
         ::uint64_t idx = st * 12 + i;
         m += prior_m[idx];
         s += prior_v[idx];
      }
      float S = (sqrt(0.001 * 0.001 + s));
      float t = y[st] * m / S;
      for (int i = 0; i < 12; i++) {
         post_m[st * 12 + i] = prior_m[st * 12 + i] + y[st] * (prior_v[st * 12 + i] / S) * V(t);
         post_s[st * 12 + i] = std::sqrt((std::fabs(prior_v[st * 12 + i] * (1 - prior_v[st * 12 + i] / (S * S) * W(t)))));
      }
   }
// for (uint st = 0; st < volume; st++) {
//   for (int i = 0; i < 12; i++) {
//     uint idx = st * 12 + i;
//     if (st % (volume / 10) == 0 && i == 0) {
//       printf("%d %d - %.4f %.4f\n", st, i, post_m[idx], post_s[idx]);
//     }
//   }
// }
   delete []prior_m;
   delete []prior_v;
   delete []post_m;
   delete []post_s;
   return 0;
}


int main(int argc,char *argv[]) {
   int __retval = main_(argc,argv);
   Artisan::report("loop_times.json");
   return __retval;
}

