/*===============================================================*/
/*                                                               */
/*                        spam_filter.cpp                        */
/*                                                               */
/*      Main host function for the Spam Filter application.      */
/*                                                               */
/*===============================================================*/

// standard C/C++ headers
#include <cstdio>
#include <cstdlib>
#include <getopt.h>
#include <string>
#include <time.h>
#include <sys/time.h>
#include <fstream>
#include <iostream>

#include "math.h"

const int NUM_FEATURES  = 1024;
const int NUM_SAMPLES   = 5000;
const int NUM_TRAINING  = 4500;
const int NUM_TESTING   = 500;
const int STEP_SIZE     = 60000; 
const int NUM_EPOCHS    = 5;
const int DATA_SET_SIZE = NUM_FEATURES * NUM_SAMPLES;
#define PAR_FACTOR 32

void print_usage(char* filename);

void parse_sdsoc_command_line_args(
    int argc,
    char** argv,
    std::string& path_to_data);

void check_results(float* param_vector, float* data_points, unsigned char* labels);

void SgdLR_sw( float    data[NUM_FEATURES * NUM_TRAINING],
               unsigned char   label[NUM_TRAINING],
               float theta[NUM_FEATURES]);


int main(int argc, char *argv[]) 
{
  setbuf(stdout, NULL);

  printf("Spam Filter Application\n");

  // parse command line arguments
  std::string path_to_data("");
  parse_sdsoc_command_line_args(argc, argv, path_to_data);

  // allocate space
  // for software verification
  float*    data  = new float[DATA_SET_SIZE];
  unsigned char*   label      = new unsigned char  [NUM_SAMPLES];
  float* theta = new float[NUM_FEATURES];

  // read in dataset
  std::string str_points_filepath = path_to_data + "/shuffledfeats.dat";
  std::string str_labels_filepath = path_to_data + "/shuffledlabels.dat";

  FILE* data_file;
  FILE* label_file;

  data_file = fopen(str_points_filepath.c_str(), "r");
  if (!data_file)
  {
    printf("Failed to open data file %s!\n", str_points_filepath.c_str());
    return EXIT_FAILURE;
  }
  for (int i = 0; i < DATA_SET_SIZE; i ++ )
  {
    float tmp;
    fscanf(data_file, "%f", &tmp);
    data[i] = tmp;
  }
  fclose(data_file);

  label_file = fopen(str_labels_filepath.c_str(), "r");
  if (!label_file)
  {
    printf("Failed to open label file %s!\n", str_labels_filepath.c_str());
    return EXIT_FAILURE;
  }
  for (int i = 0; i < NUM_SAMPLES; i ++ )
  {
    int tmp;
    fscanf(label_file, "%d", &tmp);
    label[i] = tmp;
  }
  fclose(label_file);

  // reset parameter vector
  for (size_t i = 0; i < NUM_FEATURES; i++) {
    theta[i] = 0;
  }

  SgdLR_sw(data, label, theta);

  // check results
  printf("Checking results:\n");
  check_results( theta, data, label );

  // cleanup
  delete []data;
  delete []label;
  delete []theta;

  return EXIT_SUCCESS;
}

/* utils.cpp */

void print_usage(char* filename)
{
    printf("usage: %s <options>\n", filename);
    printf("  -f [kernel file]\n");
    printf("  -p [path to data]\n");
}

void parse_sdsoc_command_line_args(
    int argc,
    char** argv,
    std::string& path_to_data) 
{

  int c = 0;

  while ((c = getopt(argc, argv, "f:p:")) != -1) 
  {
    switch (c) 
    {
      case 'p':
        path_to_data = optarg;
        break;
      default:
      {
        print_usage(argv[0]);
        exit(-1);
      }
    } // matching on arguments
  } // while args present
}

/* check_results.cpp */

// data structure only used in this file
typedef struct DataSet_s 
{
  float*    data_points;
  unsigned char*   labels;
  float* parameter_vector;
  size_t num_data_points;
  size_t num_features;
} DataSet;


// sub-functions for result checking
// dot product
float dotProduct(float* param_vector, float* data_point_i, const size_t num_features)
{
  float result = 0.0f;

  for (int i = 0; i < num_features; i ++ ) {
    result += param_vector[i] * data_point_i[i];
  }

  return result;
}

// predict
unsigned char getPrediction(float* parameter_vector, float* data_point_i, size_t num_features, const double treshold = 0) 
{
  float parameter_vector_dot_x_i = dotProduct(parameter_vector, data_point_i, num_features);
  return (parameter_vector_dot_x_i > treshold) ? 1 : 0;
}

// compute error rate
double computeErrorRate(
    DataSet data_set,
    double& cumulative_true_positive_rate,
    double& cumulative_false_positive_rate,
    double& cumulative_error)
{

  size_t true_positives = 0, true_negatives = 0, false_positives = 0, false_negatives = 0;

  for (size_t i = 0; i < data_set.num_data_points; i++) 
  {
    unsigned char prediction = getPrediction(data_set.parameter_vector, &data_set.data_points[i * data_set.num_features], data_set.num_features);
    if (prediction != data_set.labels[i])
    {
      if (prediction == 1)
        false_positives++;
      else
        false_negatives++;
    } 
    else 
    {
      if (prediction == 1)
        true_positives++;
      else
        true_negatives++;
    }
  }

  double error_rate = (double)(false_positives + false_negatives) / data_set.num_data_points;

  cumulative_true_positive_rate += (double)true_positives / (true_positives + false_negatives);
  cumulative_false_positive_rate += (double)false_positives / (false_positives + true_negatives);
  cumulative_error += error_rate;

  return error_rate;
}

// check results
void check_results(float* param_vector, float* data_points, unsigned char* labels)
{
  std::ofstream ofile;
  ofile.open("output.txt");
  if (ofile.is_open())
  {
    ofile << "\nmain parameter vector: \n";
    for(int i = 0; i < 30; i ++ ){
      ofile << "m[" << i << "]: " << param_vector[i] << " | ";
    }
    ofile << std::endl;

    // Initialize benchmark variables
    double training_tpr = 0.0;
    double training_fpr = 0.0;
    double training_error = 0.0;
    double testing_tpr = 0.0;
    double testing_fpr = 0.0;
    double testing_error = 0.0;

    // Get Training error
    DataSet training_set;
    training_set.data_points = data_points;
    training_set.labels = labels;
    training_set.num_data_points = NUM_TRAINING;
    training_set.num_features = NUM_FEATURES;
    training_set.parameter_vector = param_vector;
    computeErrorRate(training_set, training_tpr, training_fpr, training_error);

    // Get Testing error
    DataSet testing_set;
    testing_set.data_points = &data_points[NUM_FEATURES * NUM_TRAINING];
    testing_set.labels = &labels[NUM_TRAINING];
    testing_set.num_data_points = NUM_TESTING;
    testing_set.num_features = NUM_FEATURES;
    testing_set.parameter_vector = param_vector;
    computeErrorRate(testing_set, testing_tpr, testing_fpr, testing_error);

    training_tpr *= 100.0;
    training_fpr *= 100.0;
    training_error *= 100.0;
    testing_tpr *= 100.0;
    testing_fpr *= 100.0;
    testing_error *= 100.0;

    ofile << "Training TPR: " << training_tpr << std::endl; 
    ofile << "Training FPR: " << training_fpr << std::endl; 
    ofile << "Training Error: " << training_error << std::endl; 
    ofile << "Testing TPR: " << testing_tpr << std::endl; 
    ofile << "Testing FPR: " << testing_fpr << std::endl; 
    ofile << "Testing Error: " << testing_error << std::endl; 
  }
  else
  {
    std::cout << "Failed to create output file!" << std::endl;
  }
}

/* sgd_sq.cpp */

// Function to compute the dot product of data (feature) vector and parameter vector
float dotProduct(float param[NUM_FEATURES],
                       float    feature[NUM_FEATURES])
{
  float result = 0;
  for (int i = 0; i < NUM_FEATURES; i++) {
    result += param[i] * feature[i];
  }
  return result;
}

float Sigmoid(float exponent) 
{
  return 1.0f / (1.0f + expf(-exponent));
}

// Compute the gradient of the cost function
void computeGradient(
    float grad[NUM_FEATURES],
    float    feature[NUM_FEATURES],
    float scale)
{
  for (int i = 0; i < NUM_FEATURES; i++){
    grad[i] = scale * feature[i];
  }
}

// Update the parameter vector
void updateParameter(
    float param[NUM_FEATURES],
    float grad[NUM_FEATURES],
    float scale)
{
  for (int i = 0; i < NUM_FEATURES; i++)  {
    param[i] += scale * grad[i];
  }
}

// top-level function 
void SgdLR_sw(float data[NUM_FEATURES * NUM_TRAINING], unsigned char label[NUM_TRAINING], float theta[NUM_FEATURES]) {
  float gradient[NUM_FEATURES];
  for (int epoch = 0; epoch < NUM_EPOCHS; epoch ++) {
    for( int training_id = 0; training_id < NUM_TRAINING; training_id ++ ) { 
      float dot = dotProduct(theta, &data[NUM_FEATURES * training_id]);
      float prob = Sigmoid(dot);
      computeGradient(gradient, &data[NUM_FEATURES * training_id], (prob - label[training_id]));
      updateParameter(theta, gradient, -STEP_SIZE);
    }
  }
}