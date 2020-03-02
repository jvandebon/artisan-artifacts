/*===============================================================*/
/*                                                               */
/*                    optical_flow_host.cpp                      */
/*                                                               */
/*      Main host function for the Optical Flow application.     */
/*                                                               */
/*===============================================================*/

// standard C/C++ headers
#include <cstdio>
#include <cstdlib>
#include <getopt.h>
#include <string>
#include <time.h>
#include <sys/time.h>

#include <cmath>
#include <iostream>
#include <fstream>

#include "./imageLib/imageLib.h"
#include "typedefs.h"

void print_usage(char* filename);

void parse_sdaccel_command_line_args(
    int argc,
    char** argv,
    std::string& kernelFile,
    std::string& dataPath,
    std::string& outFile);

void parse_sdsoc_command_line_args(
    int argc,
    char** argv,
    std::string& dataPath,
    std::string& outFile  ); 

void check_results(velocity_t output[MAX_HEIGHT][MAX_WIDTH], CFloatImage refFlow, std::string outFile);

const int GRAD_WEIGHTS[] =  {1,-8,0,8,-1};
const pixel_t GRAD_FILTER[] = {0.0755, 0.133, 0.1869, 0.2903, 0.1869, 0.133, 0.0755};
const pixel_t TENSOR_FILTER[] = {0.3243, 0.3513, 0.3243};

// top-level function
void optical_flow_sw(pixel_t frame0[MAX_HEIGHT][MAX_WIDTH],
                     pixel_t frame1[MAX_HEIGHT][MAX_WIDTH],
                     pixel_t frame2[MAX_HEIGHT][MAX_WIDTH],
                     pixel_t frame3[MAX_HEIGHT][MAX_WIDTH],
                     pixel_t frame4[MAX_HEIGHT][MAX_WIDTH],
                     velocity_t outputs[MAX_HEIGHT][MAX_WIDTH]);

int main(int argc, char ** argv) 
{
  printf("Optical Flow Application\n");

  // parse command line arguments
  std::string dataPath("");
  std::string outFile("");

  parse_sdsoc_command_line_args(argc, argv, dataPath, outFile);

  // create actual file names according to the datapath
  std::string frame_files[5];
  std::string reference_file;
  frame_files[0] = dataPath + "/frame1.ppm";
  frame_files[1] = dataPath + "/frame2.ppm";
  frame_files[2] = dataPath + "/frame3.ppm";
  frame_files[3] = dataPath + "/frame4.ppm";
  frame_files[4] = dataPath + "/frame5.ppm";
  reference_file = dataPath + "/ref.flo";

  // read in images and convert to grayscale
  printf("Reading input files ... \n");

  CByteImage imgs[5];
  for (int i = 0; i < 5; i++) 
  {
    CByteImage tmpImg;
    ReadImage(tmpImg, frame_files[i].c_str());
    imgs[i] = ConvertToGray(tmpImg);
  }

  // read in reference flow file
  printf("Reading reference output flow... \n");

  CFloatImage refFlow;
  ReadFlowFile(refFlow, reference_file.c_str());

  // timers
  struct timeval start, end;

  static pixel_t frames[5][MAX_HEIGHT][MAX_WIDTH];
  static velocity_t outputs[MAX_HEIGHT][MAX_WIDTH];

  gettimeofday(&start, NULL);
  // use native C datatype arrays
  for (int f = 0; f < 5; f ++ ) {
    for (int i = 0; i < MAX_HEIGHT; i ++ ) {
      for (int j = 0; j < MAX_WIDTH; j ++ ) {
        frames[f][i][j] = imgs[f].Pixel(j, i, 0) / 255.0f;
      }
    }
  }

  // run
  optical_flow_sw(frames[0], frames[1], frames[2], frames[3], frames[4], outputs);
  gettimeofday(&end, NULL);


  // check results
  printf("Checking results:\n");

  check_results(outputs, refFlow, outFile);

  // print time
  long long elapsed = (end.tv_sec - start.tv_sec) * 1000000LL + end.tv_usec - start.tv_usec;   
  printf("elapsed time: %lld us\n", elapsed);

  return EXIT_SUCCESS;

}



/* check_reuslt.cpp */

void check_results(velocity_t output[MAX_HEIGHT][MAX_WIDTH], CFloatImage refFlow, std::string outFile)
{
  // copy the output into the float image
  CFloatImage outFlow(MAX_WIDTH, MAX_HEIGHT, 2);
  for (int i = 0; i < MAX_HEIGHT; i++) 
  {
    for (int j = 0; j < MAX_WIDTH; j++) 
    {
      double out_x = output[i][j].x;
      double out_y = output[i][j].y;

      if (out_x * out_x + out_y * out_y > 25.0) 
      {
        outFlow.Pixel(j, i, 0) = 1e10;
        outFlow.Pixel(j, i, 1) = 1e10;
      } 
      else 
      {
        outFlow.Pixel(j, i, 0) = out_x;
        outFlow.Pixel(j, i, 1) = out_y;
      }
    }
  }

  WriteFlowFile(outFlow, outFile.c_str());

  double accum_error = 0;
  int num_pix = 0;
  for (int i = 0; i < MAX_HEIGHT; i++) 
  {
    for (int j = 0; j < MAX_WIDTH; j++) 
    {
      double out_x = outFlow.Pixel(j, i, 0);
      double out_y = outFlow.Pixel(j, i, 1);

      if (unknown_flow(out_x, out_y)) { continue; }

      double out_deg = atan2(-out_y, -out_x) * 180.0 / M_PI;
      double ref_x = refFlow.Pixel(j, i, 0);
      double ref_y = refFlow.Pixel(j, i, 1);
      double ref_deg = atan2(-ref_y, -ref_x) * 180.0 / M_PI;

      // Normalize error to [-180, 180]
      double error = out_deg - ref_deg;
      while (error < -180)  { error += 360; }
      while (error > 180) { error -= 360; }
    
      accum_error += fabs(error);
      num_pix++;
    }
  }

  double avg_error = accum_error / num_pix;
  std::ofstream ofile;
  ofile.open("output.txt");
  if (ofile.is_open())
  {
    ofile << "Average error: " << avg_error << " degrees" << std::endl;
    ofile.close();
  }
  else
  {
    std::cout << "Failed to create output file!" << std::endl;
  }

}

/* utils.cpp */

void print_usage(char* filename)
{
    printf("usage: %s <options>\n", filename);
    printf("  -f [kernel file]\n");
    printf("  -p [path to data]\n");
    printf("  -o [path to output]\n");
}

void parse_sdsoc_command_line_args(
    int argc,
    char** argv,
    std::string& dataPath,
    std::string& outFile  ) 
{

  int c = 0;

  while ((c = getopt(argc, argv, "p:o:")) != -1) 
  {
    switch (c) 
    {
      case 'p':
        dataPath = optarg;
        break;
      case 'o':
        outFile = optarg;
        break;
     default:
      {
        print_usage(argv[0]);
        exit(-1);
      }
    } // matching on arguments
  } // while args present
}

/* optical_flow_sw.cpp */

// compute x, y gradient
void gradient_xy_calc(pixel_t frame[MAX_HEIGHT][MAX_WIDTH],
    pixel_t gradient_x[MAX_HEIGHT][MAX_WIDTH],
    pixel_t gradient_y[MAX_HEIGHT][MAX_WIDTH])
{
  pixel_t x_grad, y_grad;
  for (int r = 0; r < MAX_HEIGHT + 2; r ++ )
  {
    for (int c = 0; c < MAX_WIDTH + 2; c ++)
    {
      x_grad = 0;
      y_grad = 0;
      if (r >= 4 && r < MAX_HEIGHT && c >= 4 && c < MAX_WIDTH)
      {
        for (int i = 0; i < 5; i++)
        {
          x_grad += frame[r-2][c-i] * GRAD_WEIGHTS[4-i];
          y_grad += frame[r-i][c-2] * GRAD_WEIGHTS[4-i];
        }
        gradient_x[r-2][c-2] = x_grad / 12;
        gradient_y[r-2][c-2] = y_grad / 12;
      }
      else if (r >= 2 && c >= 2)
      {
        gradient_x[r-2][c-2] = 0;
        gradient_y[r-2][c-2] = 0;
      }
    }
  }
}

// compute z gradient
void gradient_z_calc(pixel_t frame0[MAX_HEIGHT][MAX_WIDTH], 
                     pixel_t frame1[MAX_HEIGHT][MAX_WIDTH],
                     pixel_t frame2[MAX_HEIGHT][MAX_WIDTH],
                     pixel_t frame3[MAX_HEIGHT][MAX_WIDTH],
                     pixel_t frame4[MAX_HEIGHT][MAX_WIDTH],
                     pixel_t gradient_z[MAX_HEIGHT][MAX_WIDTH])
{
  for (int r = 0; r < MAX_HEIGHT; r ++)
  {
    for (int c = 0; c < MAX_WIDTH; c ++)
    {
      gradient_z[r][c] = 0.0f;
      gradient_z[r][c] += frame0[r][c] * GRAD_WEIGHTS[0]; 
      gradient_z[r][c] += frame1[r][c] * GRAD_WEIGHTS[1]; 
      gradient_z[r][c] += frame2[r][c] * GRAD_WEIGHTS[2]; 
      gradient_z[r][c] += frame3[r][c] * GRAD_WEIGHTS[3]; 
      gradient_z[r][c] += frame4[r][c] * GRAD_WEIGHTS[4]; 
      gradient_z[r][c] /= 12.0f;
    }
  }
}

// compute y weight
void gradient_weight_y(pixel_t gradient_x[MAX_HEIGHT][MAX_WIDTH],
                       pixel_t gradient_y[MAX_HEIGHT][MAX_WIDTH],
                       pixel_t gradient_z[MAX_HEIGHT][MAX_WIDTH],
                       gradient_t filt_grad[MAX_HEIGHT][MAX_WIDTH])
{
  for (int r = 0; r < MAX_HEIGHT + 3; r ++)
  {
    for (int c = 0; c < MAX_WIDTH; c ++)
    {
      gradient_t acc;
      acc.x = 0;
      acc.y = 0;
      acc.z = 0;
      if (r >= 6 && r < MAX_HEIGHT)
      { 
        for (int i = 0; i < 7; i ++)
        {
          acc.x += gradient_x[r-i][c] * GRAD_FILTER[i];
          acc.y += gradient_y[r-i][c] * GRAD_FILTER[i];
          acc.z += gradient_z[r-i][c] * GRAD_FILTER[i];
        }
        filt_grad[r-3][c] = acc;            
      }
      else if (r >= 3)
      {
        filt_grad[r-3][c] = acc;
      }
    }
  }
}

// compute x weight
void gradient_weight_x(gradient_t y_filt[MAX_HEIGHT][MAX_WIDTH],
                       gradient_t filt_grad[MAX_HEIGHT][MAX_WIDTH])
{
  for (int r = 0; r < MAX_HEIGHT; r ++)
  {
    for (int c = 0; c < MAX_WIDTH + 3; c ++)
    {
      gradient_t acc;
      acc.x = 0;
      acc.y = 0;
      acc.z = 0;
      if (c >= 6 && c < MAX_WIDTH)
      {
        for (int i = 0; i < 7; i ++)
        {
          acc.x += y_filt[r][c-i].x * GRAD_FILTER[i];
          acc.y += y_filt[r][c-i].y * GRAD_FILTER[i];
          acc.z += y_filt[r][c-i].z * GRAD_FILTER[i];
        }
        filt_grad[r][c-3] = acc;
      }
      else if (c >= 3)
      {
        filt_grad[r][c-3] = acc;
      }
    }
  }
}
 
// outer product
void outer_product(gradient_t gradient[MAX_HEIGHT][MAX_WIDTH],
                   outer_t outer_product[MAX_HEIGHT][MAX_WIDTH])
{ 
  for (int r = 0; r < MAX_HEIGHT; r ++)
  {
    for (int c = 0; c < MAX_WIDTH; c ++)
    {
      gradient_t grad = gradient[r][c];
      outer_t out;
      out.val[0] = grad.x * grad.x;
      out.val[1] = grad.y * grad.y;
      out.val[2] = grad.z * grad.z;
      out.val[3] = grad.x * grad.y;
      out.val[4] = grad.x * grad.z;
      out.val[5] = grad.y * grad.z;
      outer_product[r][c] = out;
    }
  }
}

// tensor weight y
void tensor_weight_y(outer_t outer[MAX_HEIGHT][MAX_WIDTH],
                     tensor_t tensor_y[MAX_HEIGHT][MAX_WIDTH])
{
  for (int r = 0; r < MAX_HEIGHT + 1; r ++)
  {
    for(int c = 0; c < MAX_WIDTH; c ++)
    {
      tensor_t acc;
      for (int k = 0; k < 6; k ++)
      {
        acc.val[k] = 0;
      }

      if (r >= 2 && r < MAX_HEIGHT) 
      {
        for (int i = 0; i < 3; i ++)
        {
          for(int component = 0; component < 6; component ++)
          {
            acc.val[component] += outer[r-i][c].val[component] * TENSOR_FILTER[i];
          }
        }
      }
      if (r >= 1)
      { 
        tensor_y[r-1][c] = acc;            
      }
    }
  }
}

// tensor weight x
void tensor_weight_x(tensor_t tensor_y[MAX_HEIGHT][MAX_WIDTH],
                     tensor_t tensor[MAX_HEIGHT][MAX_WIDTH])
{
  for (int r = 0; r < MAX_HEIGHT; r ++)
  {
    for (int c = 0; c < MAX_WIDTH + 1; c ++)
    {
      tensor_t acc;
      for(int k = 0; k < 6; k++)
      {
        acc.val[k] = 0;
      }
      if (c >= 2 && c < MAX_WIDTH) 
      {
        for (int i = 0; i < 3; i ++)
        {
          for (int component = 0; component < 6; component ++)
          {
            acc.val[component] += tensor_y[r][c-i].val[component] * TENSOR_FILTER[i];
          }
        }
      }
      if (c >= 1)
      {
        tensor[r][c-1] = acc;
      }
    }
  }
}

// compute flow
void flow_calc(tensor_t tensors[MAX_HEIGHT][MAX_WIDTH],
               velocity_t output[MAX_HEIGHT][MAX_WIDTH])
{
  for(int r = 0; r < MAX_HEIGHT; r ++)
  {
    for(int c = 0; c < MAX_WIDTH; c ++)
    {
      if (r >= 2 && r < MAX_HEIGHT - 2 && c >= 2 && c < MAX_WIDTH - 2)
      {
        pixel_t denom = tensors[r][c].val[0] * tensors[r][c].val[1] -
                        tensors[r][c].val[3] * tensors[r][c].val[3];
        output[r][c].x = (tensors[r][c].val[5] * tensors[r][c].val[3] -
                          tensors[r][c].val[4] * tensors[r][c].val[1]) / denom;
        output[r][c].y = (tensors[r][c].val[4] * tensors[r][c].val[3] -
                          tensors[r][c].val[5] * tensors[r][c].val[0]) / denom;
      }
      else
      {
        output[r][c].x = 0;
        output[r][c].y = 0;
      }
    }
  }
}

// top-level sw function
void optical_flow_sw(pixel_t frame0[MAX_HEIGHT][MAX_WIDTH],
                     pixel_t frame1[MAX_HEIGHT][MAX_WIDTH],
                     pixel_t frame2[MAX_HEIGHT][MAX_WIDTH],
                     pixel_t frame3[MAX_HEIGHT][MAX_WIDTH],
                     pixel_t frame4[MAX_HEIGHT][MAX_WIDTH],
                     velocity_t outputs[MAX_HEIGHT][MAX_WIDTH])
{
  // intermediate arrays
  static pixel_t gradient_x[MAX_HEIGHT][MAX_WIDTH];
  static pixel_t gradient_y[MAX_HEIGHT][MAX_WIDTH];
  static pixel_t gradient_z[MAX_HEIGHT][MAX_WIDTH];
  static gradient_t y_filtered[MAX_HEIGHT][MAX_WIDTH];
  static gradient_t filtered_gradient[MAX_HEIGHT][MAX_WIDTH];
  static outer_t out_product[MAX_HEIGHT][MAX_WIDTH];
  static tensor_t tensor_y[MAX_HEIGHT][MAX_WIDTH];
  static tensor_t tensor[MAX_HEIGHT][MAX_WIDTH];

  // compute
  gradient_xy_calc(frame2, gradient_x, gradient_y);
  gradient_z_calc(frame0, frame1, frame2, frame3, frame4, gradient_z);
  gradient_weight_y(gradient_x, gradient_y, gradient_z, y_filtered);
  gradient_weight_x(y_filtered, filtered_gradient);
  outer_product(filtered_gradient, out_product);
  tensor_weight_y(out_product, tensor_y);
  tensor_weight_x(tensor_y, tensor);
  flow_calc(tensor, outputs);
}