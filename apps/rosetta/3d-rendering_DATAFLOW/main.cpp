// standard C/C++ headers
#include <cstdio>
#include <cstdlib>
#include <getopt.h>
#include <string>
#include <time.h>
#include <sys/time.h>
#include <fstream>
#include <iostream>

#include "typedefs.h"
#pragma artisan-hls header {"file":"typedefs.h", "path":"./"}
// data
#include "input_data.h"
#pragma artisan-hls header {"file":"input_data.h", "path":"./", "host_only":"True"}
void print_usage(char* filename);
void check_results(bit8 output[MAX_X][MAX_Y]);
void rendering_sw(Triangle_3D triangle_3ds[NUM_3D_TRI], bit8 output[MAX_X][MAX_Y]);

int main(int argc, char ** argv) 
{
    printf("3D Rendering Application\n");
    // for this benchmark, data is included in array triangle_3ds

    // timers
    struct timeval start, end;

    // output
    bit8 output[MAX_X][MAX_Y];

    // run and time sw function
    gettimeofday(&start, 0);
    rendering_sw(triangle_3ds, output);
    gettimeofday(&end, 0);

    // check results
    printf("Checking results:\n");
    check_results(output);
    
    // print time
    long long elapsed = (end.tv_sec - start.tv_sec) * 1000000LL + end.tv_usec - start.tv_usec;   
    printf("elapsed time: %lld us\n", elapsed);

    return EXIT_SUCCESS;
}

/**** UTILS.CPP ****/

void print_usage(char* filename)
{
    printf("usage: %s <options>\n", filename);
    printf("  -f [kernel file]\n");
}

/**** CHECK_RESULT.CPP ****/

void check_results(bit8 output[MAX_X][MAX_Y])
{
  // print result
  std::ofstream ofile;
  ofile.open("outputs.txt");
  if (ofile.is_open())
  {
    ofile << "Image After Rendering: \n";
    for (int j = MAX_X - 1; j >= 0; j -- )
    {
      for (int i = 0; i < MAX_Y; i ++ )
      {
        int pix;
        pix = output[i][j];
        if (pix) {
          ofile << "1";
        } else {
          ofile << "0";

        }
      }
      ofile << std::endl;
    }
  }
  else
  {
    std::cout << "Failed to create output file!" << std::endl;
  }
}

/**** RENDERING_SW.CPP ****/

/*======================UTILITY FUNCTIONS========================*/

int check_clockwise( Triangle_2D triangle_2d )
{
  int cw;

  cw = (triangle_2d.x2 - triangle_2d.x0) * (triangle_2d.y1 - triangle_2d.y0)
       - (triangle_2d.y2 - triangle_2d.y0) * (triangle_2d.x1 - triangle_2d.x0);

  return cw;

}

// swap (x0, y0) (x1, y1) of a Triangle_2D
void clockwise_vertices( Triangle_2D *triangle_2d ) {

  bit8 tmp_x, tmp_y;

  tmp_x = triangle_2d->x0;
  tmp_y = triangle_2d->y0;

  triangle_2d->x0 = triangle_2d->x1;
  triangle_2d->y0 = triangle_2d->y1;

  triangle_2d->x1 = tmp_x;
  triangle_2d->y1 = tmp_y;

}

bool pixel_in_triangle( bit8 x, bit8 y, Triangle_2D triangle_2d ) {

  int pi0, pi1, pi2;

  pi0 = (x - triangle_2d.x0) * (triangle_2d.y1 - triangle_2d.y0) - (y - triangle_2d.y0) * (triangle_2d.x1 - triangle_2d.x0);
  pi1 = (x - triangle_2d.x1) * (triangle_2d.y2 - triangle_2d.y1) - (y - triangle_2d.y1) * (triangle_2d.x2 - triangle_2d.x1);
  pi2 = (x - triangle_2d.x2) * (triangle_2d.y0 - triangle_2d.y2) - (y - triangle_2d.y2) * (triangle_2d.x0 - triangle_2d.x2);

  return (pi0 >= 0 && pi1 >= 0 && pi2 >= 0);
}

// find the min from 3 integers
bit8 find_min( bit8 in0, bit8 in1, bit8 in2 )
{
  if (in0 < in1)
  {
    if (in0 < in2) {
      return in0;
    } else {
      return in2;
      
    }   
  }
  else 
  {
    if (in1 < in2) {
      return in1;
    } else {
      return in2;
    }
  }
}


// find the max from 3 integers
bit8 find_max( bit8 in0, bit8 in1, bit8 in2 )
{
  if (in0 > in1)
  {
    if (in0 > in2){
      return in0;
    } else {
      return in2;
    }
  }
  else 
  {
    if (in1 > in2) {
      return in1;
    } else {
      return in2;
    }
  }
}

/*======================PROCESSING STAGES========================*/

// project a 3D triangle to a 2D triangle
void projection ( Triangle_3D triangle_3ds[NUM_3D_TRI], Triangle_2D triangle_2ds[NUM_3D_TRI]) {
  
  for (int i = 0; i < NUM_3D_TRI; i++) {
    triangle_2ds[i].x0 = triangle_3ds[i].x0;
    triangle_2ds[i].y0 = triangle_3ds[i].y0;
    triangle_2ds[i].x1 = triangle_3ds[i].x1;
    triangle_2ds[i].y1 = triangle_3ds[i].y1;
    triangle_2ds[i].x2 = triangle_3ds[i].x2;
    triangle_2ds[i].y2 = triangle_3ds[i].y2;
    triangle_2ds[i].z  = triangle_3ds[i].z0 / 3 + triangle_3ds[i].z1 / 3 + triangle_3ds[i].z2 / 3;
  }
}

void rasterization1 ( Triangle_2D triangle_2ds[NUM_3D_TRI], bit8 max_min[NUM_3D_TRI][5], int max_index[NUM_3D_TRI], bool flag[NUM_3D_TRI]) {

  for (int i = 0; i < NUM_3D_TRI; i++) {

    Triangle_2D triangle_2d = triangle_2ds[i];

    if ( check_clockwise( triangle_2d ) == 0 ) {
      flag[i] = 1;
      continue;
    }

    if ( check_clockwise( triangle_2d ) < 0 ) {
      clockwise_vertices( &triangle_2d );
    }

    // find the rectangle bounds of 2D triangles
    max_min[i][0] = find_min( triangle_2d.x0, triangle_2d.x1, triangle_2d.x2 );
    max_min[i][1] = find_max( triangle_2d.x0, triangle_2d.x1, triangle_2d.x2 );
    max_min[i][2] = find_min( triangle_2d.y0, triangle_2d.y1, triangle_2d.y2 );
    max_min[i][3] = find_max( triangle_2d.y0, triangle_2d.y1, triangle_2d.y2 );
    max_min[i][4] = max_min[i][1] - max_min[i][0];

    // calculate index for searching pixels
    max_index[i] = (max_min[i][1] - max_min[i][0]) * (max_min[i][3] - max_min[i][2]);

    flag[i] = 0;
  }
}

void rasterization2 ( bool flag[NUM_3D_TRI], bit8 max_min[NUM_3D_TRI][5], int max_index[NUM_3D_TRI], 
                    Triangle_2D triangle_2ds[NUM_3D_TRI], CandidatePixel **fragment, 
                    int size_fragment[NUM_3D_TRI ]){

  for (int j = 0; j < NUM_3D_TRI; j++) {
    
    if ( flag[j] ) {
      size_fragment[j] = 0;
      continue;
    }

    bit8 color = 100;
    int i = 0;

    for ( int k = 0; k < max_index[j]; k ++ ) {
      bit8 x = max_min[j][0] + k % max_min[j][4];
      bit8 y = max_min[j][2] + k / max_min[j][4];

      if( pixel_in_triangle( x, y, triangle_2ds[j] ) ) {
        fragment[j][i].x = x;
        fragment[j][i].y = y;
        fragment[j][i].z = triangle_2ds[j].z;
        fragment[j][i].color = color;
        i++;
      }
    }
    size_fragment[j] = i;
  }
}

// filter hidden pixels
void zculling (CandidatePixel **fragments, int size_fragment[NUM_3D_TRI], 
              Pixel **pixels, int size_pixels[NUM_3D_TRI]) {

  for (int k = 0; k < NUM_3D_TRI; k++) {
    int size = size_fragment[k];
    static bit8 z_buffer[MAX_X][MAX_Y];
    if (k == 0) {
      for ( int i = 0; i < MAX_X; i ++ ) {
        for ( int j = 0; j < MAX_Y; j ++ ) {
          z_buffer[i][j] = 255;
        }
      }
    }
    // pixel counter
    int pixel_cntr = 0;
    // update z-buffer and pixels
    for (int n = 0; n < size; n ++ ) {
      if( fragments[k][n].z < z_buffer[fragments[k][n].y][fragments[k][n].x] ) {
        pixels[k][pixel_cntr].x     = fragments[k][n].x;
        pixels[k][pixel_cntr].y     = fragments[k][n].y;
        pixels[k][pixel_cntr].color = fragments[k][n].color;
        pixel_cntr++;
        z_buffer[fragments[k][n].y][fragments[k][n].x] = fragments[k][n].z;
      }
    }
    size_pixels[k] = pixel_cntr;
  }
}

// color the frame buffer
void coloringFB(int size_pixels[NUM_3D_TRI], Pixel **pixels, bit8 frame_buffer[MAX_X][MAX_Y])
{

  for (int k = 0; k < NUM_3D_TRI; k++) {
    if ( k == 0 ) {
      for ( int i = 0; i < MAX_X; i ++ ) {
        for ( int j = 0; j < MAX_Y; j ++ ) {
          frame_buffer[i][j] = 0;
        }
      }
    }
    for ( int i = 0; i < size_pixels[k]; i ++ ) {
      frame_buffer[ pixels[k][i].x ][ pixels[k][i].y ] = pixels[k][i].color;
    }
  }

}



/*========================TOP FUNCTION===========================*/
void rendering_sw_internal (Triangle_3D triangle_3ds[NUM_3D_TRI], Pixel **pixels,CandidatePixel **fragment,bit8 output[MAX_X][MAX_Y]) {

  Triangle_2D triangle_2ds[NUM_3D_TRI];
  bit8 max_min[NUM_3D_TRI][5];
  int max_index[NUM_3D_TRI];
  bool flag[NUM_3D_TRI];
  int size_fragment[NUM_3D_TRI];
  int size_pixels[NUM_3D_TRI];

  projection(triangle_3ds, triangle_2ds);
  rasterization1(triangle_2ds, max_min, max_index, flag);
  rasterization2( flag, max_min, max_index, triangle_2ds, fragment, size_fragment);
  zculling(fragment, size_fragment, pixels, size_pixels);
  coloringFB (size_pixels, pixels, output);
  
}


void rendering_sw( Triangle_3D triangle_3ds[NUM_3D_TRI], bit8 output[MAX_X][MAX_Y]) {
  
  Pixel **pixels = (Pixel **) malloc(NUM_3D_TRI*sizeof(Pixel *)); 
  CandidatePixel **fragment = (CandidatePixel **) malloc(NUM_3D_TRI*sizeof(CandidatePixel *));
  for (int i = 0; i < NUM_3D_TRI; i++) {
    fragment[i] = (CandidatePixel *) malloc(500*sizeof(CandidatePixel));
    pixels[i] = (Pixel *) malloc(500*sizeof(Pixel));
  }

  rendering_sw_internal(triangle_3ds, pixels, fragment, output);
}



