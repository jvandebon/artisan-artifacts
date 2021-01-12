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

// other headers
// #include "utils.h"
#include "typedefs.h"
#pragma artisan-hls header {"file":"typedefs.h", "path":"./"}

void check_results(FeatureType* param_vector, DataType* data_points, LabelType* labels);
void print_usage(char* filename);
void parse_sdaccel_command_line_args(int argc, char** argv, std::string& kernelFile, std::string& path_to_data);
void parse_sdsoc_command_line_args(int argc, char** argv, std::string& path_to_data);
void SgdLR_sw(DataType data[NUM_FEATURES * NUM_TRAINING], LabelType label[NUM_TRAINING], FeatureType theta[NUM_FEATURES]);

int main(int argc, char *argv[]) 
{
  setbuf(stdout, NULL);

  printf("Spam Filter Application\n");

  // parse command line arguments
  std::string path_to_data("");
  // sdaccel version and sdsoc/sw version have different command line options
  parse_sdsoc_command_line_args(argc, argv, path_to_data);

  // allocate space
  // for software verification
  DataType*    data_points  = new DataType[DATA_SET_SIZE];
  LabelType*   labels       = new LabelType  [NUM_SAMPLES];
  FeatureType* param_vector = new FeatureType[NUM_FEATURES];

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
    data_points[i] = tmp;
  }
  fclose(data_file);


  int non_zero = 0;
  for (int i = 0; i < DATA_SET_SIZE; i ++ )
  {
    if (data_points[i] != 0) {
      non_zero++;
   }
  }
  printf("NON-ZERO: %d/%d\n", non_zero, DATA_SET_SIZE);

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
    labels[i] = tmp;
  }
  fclose(label_file);

  non_zero = 0;
  for (int i = 0; i < NUM_SAMPLES; i ++ )
  {
    if (labels[i] != 0) {
      non_zero++;
   }
  }
  printf("NON-ZERO: %d/%d\n", non_zero, NUM_SAMPLES);


  // reset parameter vector
  for (size_t i = 0; i < NUM_FEATURES; i++) {
    param_vector[i] = 0;
  }

  // timers
  struct timeval start, end;

  // sw version host code
  gettimeofday(&start, NULL);
  SgdLR_sw(data_points, labels, param_vector);
  gettimeofday(&end, NULL);

  // check results
  printf("Checking results:\n");
  check_results( param_vector, data_points, labels );
    
  // print time
  long long elapsed = (end.tv_sec - start.tv_sec) * 1000000LL + end.tv_usec - start.tv_usec;   
  printf("elapsed time: %lld us\n", elapsed);

  // cleanup
  delete []data_points;
  delete []labels;
  delete []param_vector;

  return EXIT_SUCCESS;
}


/**** CHECK_RESULTS.CPP ****/

// data structure only used in this file
typedef struct DataSet_s 
{
  DataType*    data_points;
  LabelType*   labels;
  FeatureType* parameter_vector;
  size_t num_data_points;
  size_t num_features;
} DataSet;


// sub-functions for result checking
// dot product
float dotProduct(FeatureType* param_vector, DataType* data_point_i, const size_t num_features)
{
  FeatureType result = 0.0;

  for (int i = 0; i < num_features; i ++ ) {
    result += param_vector[i] * data_point_i[i];
  }
  return result;
}

// predict
LabelType getPrediction(FeatureType* parameter_vector, DataType* data_point_i, size_t num_features, const double treshold = 0) 
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
    LabelType prediction = getPrediction(data_set.parameter_vector, &data_set.data_points[i * data_set.num_features], data_set.num_features);
    if (prediction != data_set.labels[i])
    {
      if (prediction == 1){
        false_positives++;
      } else {
        false_negatives++;
      }
    } 
    else 
    {
      if (prediction == 1) {
        true_positives++;
      } else {
        true_negatives++;
      }
    }
  }

  double error_rate = (double)(false_positives + false_negatives) / data_set.num_data_points;

  cumulative_true_positive_rate += (double)true_positives / (true_positives + false_negatives);
  cumulative_false_positive_rate += (double)false_positives / (false_positives + true_negatives);
  cumulative_error += error_rate;

  return error_rate;
}

// check results
void check_results(FeatureType* param_vector, DataType* data_points, LabelType* labels)
{
  std::ofstream ofile;
  ofile.open("outputs.txt");
  if (ofile.is_open())
  {
    ofile << "\nmain parameter vector: \n";
    for(int i = 0; i < 30; i ++ ) {
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

/**** UTILS.CPP ****/

void print_usage(char* filename)
{
    printf("usage: %s <options>\n", filename);
    printf("  -f [kernel file]\n");
    printf("  -p [path to data]\n");
}

void parse_sdaccel_command_line_args(
    int argc,
    char** argv,
    std::string& kernelFile,
    std::string& path_to_data) 
{

  int c = 0;

  while ((c = getopt(argc, argv, "f:p:")) != -1) 
  {
    switch (c) 
    {
      case 'f':
        kernelFile = optarg;
        break;
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

/**** SGD_SW.CPP ****/

// Function to compute the dot product of data (feature) vector and parameter vector
FeatureType dotProduct(FeatureType param[NUM_FEATURES],
                       DataType    feature[NUM_FEATURES])
{
  FeatureType result = 0;
  DOT: for (int i = 0; i < NUM_FEATURES; i++) {
    result += param[i] * feature[i];
  }
  return result;
}

FeatureType Sigmoid(FeatureType exponent) 
{
  return 1.0 / (1.0 + exp(-exponent));
}

// Compute the gradient of the cost function
void computeGradient(
    FeatureType grad[NUM_FEATURES],
    DataType    feature[NUM_FEATURES],
    FeatureType scale)
{
  GRAD: for (int i = 0; i < NUM_FEATURES; i++){
    grad[i] = scale * feature[i];
  }
}

// Update the parameter vector
void updateParameter(
    FeatureType param[NUM_FEATURES],
    FeatureType grad[NUM_FEATURES],
    FeatureType scale)
{
  UPDATE: for (int i = 0; i < NUM_FEATURES; i++){
    param[i] += scale * grad[i];
  }
}

// top-level function 
void SgdLR_sw( DataType    data[NUM_FEATURES * NUM_TRAINING],
               LabelType   label[NUM_TRAINING],
               FeatureType theta[NUM_FEATURES])
{
  // intermediate variable for storing gradient
  FeatureType gradient[NUM_FEATURES];

  // main loop
  // runs for multiple epochs
  EPOCH: for (int epoch = 0; epoch < NUM_EPOCHS; epoch ++) 
  {
    #pragma artisan-hls parallel { "is_parallel" : "True" }
    #pragma artisan-hls vars { "data": "R", "label": "R", "theta": "W"}
    // in each epoch, go through each trainingf instance in sequence
    TRAINING_INST: for( int training_id = 0; training_id < NUM_TRAINING; training_id ++ )
    { 
      // dot product between parameter vector and data sample 
      FeatureType dot = dotProduct(theta, &data[NUM_FEATURES * training_id]);
      // sigmoid
      FeatureType prob = Sigmoid(dot);
      // compute gradient
      computeGradient(gradient, &data[NUM_FEATURES * training_id], (prob - label[training_id]));
      // update parameter vector
      updateParameter(theta, gradient, -STEP_SIZE);
    }
  }
}
