#include <iomanip>
#include <iostream>
#include <sstream>

#include <cuda.h>
#include <cuda_runtime_api.h>
#include "sys/time.h"

#include "backend/backends.h"
#include "util/blitz_cpu_function.h"

using namespace blitz;

void left_transpose(int left_dim, int common_dim, int right_dim, const string& kernel) {
  std::cout << "left transpose start" << std::endl;
  Shape left_shape(2);
  left_shape[0] = common_dim;
  left_shape[1] = left_dim;

  CPUTensor<float> left_cpu(left_shape);
  GPUTensor<float> left_gpu(left_shape);
  Backend<GPUTensor, float>::NormalDistributionFunc(0, 1, &left_gpu);
  cudaMemcpy(left_cpu.data(), left_gpu.data(), left_cpu.size() * sizeof(float),
    cudaMemcpyDeviceToHost);

  Shape right_shape(2);
  right_shape[0] = common_dim;
  right_shape[1] = right_dim;
  CPUTensor<float> right_cpu(right_shape);
  GPUTensor<float> right_gpu(right_shape);
  Backend<GPUTensor, float>::NormalDistributionFunc(0, 1, &right_gpu);
  cudaMemcpy(right_cpu.data(), right_gpu.data(), right_cpu.size() * sizeof(float),
    cudaMemcpyDeviceToHost);
  cudaDeviceSynchronize();

  Shape output_shape(2);
  output_shape[0] = left_dim;
  output_shape[1] = right_dim;
  CPUTensor<float> output_cpu(output_shape);
  GPUTensor<float> output_gpu(output_shape);
  CPUTensor<float> output_copy(output_shape);

  cudaEvent_t start, stop;
  cudaEventCreate(&start);
  cudaEventCreate(&stop);

  cudaEventRecord(start);
  Backend<GPUTensor, float>::MatrixDotFunc(&left_gpu, &right_gpu,
    true, false, 1, 0, &output_gpu, kernel);
  cudaEventRecord(stop);
  cudaEventSynchronize(stop);

  float elapsed_time = 0;
  cudaEventElapsedTime(&elapsed_time, start, stop);
  elapsed_time /= 1000.0;
  std::cout << "GPU running time: " << elapsed_time << std::endl;

  cudaMemcpy(output_copy.data(), output_gpu.data(), output_cpu.size() * sizeof(float),
    cudaMemcpyDeviceToHost);
  cudaDeviceSynchronize();
  timeval t1, t2;
  gettimeofday(&t1, NULL);
  
  Backend<CPUTensor, float>::MatrixDotFunc(&left_cpu, &right_cpu,
    true, false, 1, 0, &output_cpu);

  gettimeofday(&t2, NULL);
  elapsed_time = (t2.tv_sec - t1.tv_sec) * 1000.0;
  elapsed_time += (t2.tv_usec - t1.tv_usec) / 1000.0;
  elapsed_time /= 1000.0;
  std::cout << "CPU running time: " << elapsed_time << std::endl;

  for (size_t i = 0; i < output_cpu.size(); ++i) {
    if (!(output_copy[i] <= output_cpu[i] + 1e-3 && output_copy[i] >= output_cpu[i] - 1e-3)) {
      std::cout << "index: " << i << " gpu: " <<
        output_copy[i] << " cpu: " << output_cpu[i] << std::endl;
    }
  }
}

void right_transpose(int left_dim, int common_dim, int right_dim, const string& kernel) {
  std::cout << "right transpose start" << std::endl;
  Shape left_shape(2);
  left_shape[0] = left_dim;
  left_shape[1] = common_dim;

  CPUTensor<float> left_cpu(left_shape);
  GPUTensor<float> left_gpu(left_shape);
  Backend<GPUTensor, float>::NormalDistributionFunc(0, 1, &left_gpu);
  cudaMemcpy(left_cpu.data(), left_gpu.data(), left_cpu.size() * sizeof(float),
    cudaMemcpyDeviceToHost);

  Shape right_shape(2);
  right_shape[0] = right_dim;
  right_shape[1] = common_dim;
  CPUTensor<float> right_cpu(right_shape);
  GPUTensor<float> right_gpu(right_shape);
  Backend<GPUTensor, float>::NormalDistributionFunc(0, 1, &right_gpu);
  cudaMemcpy(right_cpu.data(), right_gpu.data(), right_cpu.size() * sizeof(float),
    cudaMemcpyDeviceToHost);

  Shape output_shape(2);
  output_shape[0] = left_dim;
  output_shape[1] = right_dim;
  CPUTensor<float> output_cpu(output_shape);
  GPUTensor<float> output_gpu(output_shape);
  CPUTensor<float> output_copy(output_shape);
  time_point<system_clock> start, end;
  duration<double> time = duration<double>::zero();
  start = system_clock::now();
  Backend<GPUTensor, float>::MatrixDotFunc(&left_gpu, &right_gpu,
    false, true, 1, 0, &output_gpu, kernel);
  cudaDeviceSynchronize();
  end = system_clock::now();
  time = end - start;
  std::cout << "GPU running time: " << time.count() << std::endl;

  cudaMemcpy(output_copy.data(), output_gpu.data(), output_cpu.size() * sizeof(float),
    cudaMemcpyDeviceToHost);

  start = system_clock::now();
  Backend<CPUTensor, float>::MatrixDotFunc(&left_cpu, &right_cpu,
    false, true, 1, 0, &output_cpu);
  end = system_clock::now();
  time = end - start;
  std::cout << "CPU running time: " << time.count() << std::endl;

  for (size_t i = 0; i < output_cpu.size(); ++i) {
    if (!(output_copy[i] <= output_cpu[i] + 1e-3 && output_copy[i] >= output_cpu[i] - 1e-3)) {
      std::cout << "index: " << i << " gpu: " <<
        output_copy[i] << " cpu: " << output_cpu[i] << std::endl;
    }
  }
}

void both_transpose() {
  std::cout << "both transpose start" << std::endl;
  Shape left_shape(2);
  left_shape[0] = 1024;
  left_shape[1] = 1024;

  CPUTensor<float> left(left_shape);
  Backend<CPUTensor, float>::NormalDistributionFunc(0, 1, &left);

  std::cout << "left: " << std::endl;
  size_t shape1 = left.shape()[0];
  size_t shape2 = left.shape()[1];
  for (size_t i = 0; i < shape1; ++i) {
    for (size_t j = 0; j < shape2; ++j) {
      std::cout << left[i * shape2 + j] << " ";
    }
    std::cout << std::endl;
  }

  Shape right_shape(2);
  right_shape[0] = 1024;
  right_shape[1] = 1024;
  CPUTensor<float> right(right_shape);
  Backend<CPUTensor, float>::NormalDistributionFunc(0, 1, &right);
  std::cout << "right size " << right.shape().size() << std::endl;

  std::cout << "right: " << std::endl;
  shape1 = right.shape()[0];
  shape2 = right.shape()[1];
  for (size_t i = 0; i < shape1; ++i) {
    for (size_t j = 0; j < shape2; ++j) {
      std::cout << right[i * shape2 + j] << " ";
    }
    std::cout << std::endl;
  }

  Shape output_shape(2);
  output_shape[0] = 1024;
  output_shape[1] = 1024;
  CPUTensor<float> output(output_shape);
  Backend<CPUTensor, float>::MatrixDotFunc(&left, &right, true, true, 1, 0, &output);

  std::cout << "output: " << std::endl;
  shape1 = output.shape()[0];
  shape2 = output.shape()[1];
  for (size_t i = 0; i < shape1; ++i) {
    for (size_t j = 0; j < shape2; ++j) {
      std::cout << output[i * shape2 + j] << " ";
    }
    std::cout << std::endl;
  }
  std::cout << "both transpose end" << std::endl;
}

void no_transpose(int left_dim, int common_dim, int right_dim, const string& kernel) {
  std::cout << "no transpose start" << std::endl;
  Shape left_shape(2);
  left_shape[0] = left_dim;
  left_shape[1] = common_dim;

  CPUTensor<float> left_cpu(left_shape);
  GPUTensor<float> left_gpu(left_shape);
  Backend<GPUTensor, float>::NormalDistributionFunc(0, 1, &left_gpu);
  cudaMemcpy(left_cpu.data(), left_gpu.data(), left_cpu.size() * sizeof(float),
    cudaMemcpyDeviceToHost);

  Shape right_shape(2);
  right_shape[0] = common_dim;
  right_shape[1] = right_dim;
  CPUTensor<float> right_cpu(right_shape);
  GPUTensor<float> right_gpu(right_shape);
  Backend<GPUTensor, float>::NormalDistributionFunc(0, 1, &right_gpu);
  cudaMemcpy(right_cpu.data(), right_gpu.data(), right_cpu.size() * sizeof(float),
    cudaMemcpyDeviceToHost);

  Shape output_shape(2);
  output_shape[0] = left_dim;
  output_shape[1] = right_dim;
  CPUTensor<float> output_cpu(output_shape);
  GPUTensor<float> output_gpu(output_shape);
  CPUTensor<float> output_copy(output_shape);
  time_point<system_clock> start, end;
  duration<double> time = duration<double>::zero();
  start = system_clock::now();
  Backend<GPUTensor, float>::MatrixDotFunc(&left_gpu, &right_gpu,
    false, false, 1, 0, &output_gpu, kernel);
  cudaDeviceSynchronize();
  end = system_clock::now();
  time = end - start;
  std::cout << "GPU running time: " << time.count() << std::endl;

  cudaMemcpy(output_copy.data(), output_gpu.data(), output_cpu.size() * sizeof(float),
    cudaMemcpyDeviceToHost);

  start = system_clock::now();
  Backend<CPUTensor, float>::MatrixDotFunc(&left_cpu, &right_cpu,
    false, false, 1, 0, &output_cpu);
  end = system_clock::now();
  time = end - start;
  std::cout << "CPU running time: " << time.count() << std::endl;

  for (size_t i = 0; i < output_cpu.size(); ++i) {
    if (!(output_copy[i] <= output_cpu[i] + 1e-3 && output_copy[i] >= output_cpu[i] - 1e-3)) {
      std::cout << "index: " << i << " gpu: " <<
        output_copy[i] << " cpu: " << output_cpu[i] << std::endl;
    }
  }
}

void performance() {
  // no transpose
  //for (int i = 1; i <= 40; ++i) {
  //  int current_dim = i * 128;
  //  no_transpose(current_dim, current_dim, current_dim, "asm");
  //}
  //for (int i = 1; i <= 40; ++i) {
  //  int current_dim = i * 128;
  //  no_transpose(current_dim, current_dim, current_dim, "blas");
  //}
  // left transpose
  //for (int i = 1; i <= 40; ++i) {
  //  int current_dim = i * 128;
  //  left_transpose(current_dim, current_dim, current_dim, "asm");
  //}
  //for (int i = 1; i <= 40; ++i) {
  //  int current_dim = i * 128;
  //  left_transpose(current_dim, current_dim, current_dim, "blas");
  //}
  // right transpose
  //for (int i = 1; i <= 40; ++i) {
  //  int current_dim = i * 128;
  //  right_transpose(current_dim, current_dim, current_dim, "asm");
  //}
  for (int i = 1; i <= 40; ++i) {
    int current_dim = i * 128;
    right_transpose(current_dim, current_dim, current_dim, "blas");
  }
}

void correct() {
  //no_transpose(512, 128, 500, "asm");
  //no_transpose(1024, 1024, 1024, "asm");
  //no_transpose(1024, 1025, 1024, "asm");
  //no_transpose(1024, 1026, 1024, "asm");
  //no_transpose(1024, 1027, 1024, "asm");
  //no_transpose(1024, 1028, 1024, "asm");
  //no_transpose(1024, 1029, 1024, "asm");
  //no_transpose(1024, 1031, 1024, "asm");
  //no_transpose(1024, 1032, 1024, "asm");
  //no_transpose(1024, 1033, 1024, "asm");
  //no_transpose(1024, 1034, 1024, "asm");
  //no_transpose(1024, 1035, 1024, "asm");
  //no_transpose(1024, 1036, 1024, "asm");
  //no_transpose(1024, 1037, 1024, "asm");
  //no_transpose(1024, 1038, 1024, "asm");
  //no_transpose(1024, 1039, 1024, "asm");

  //for (int i = 0; i < 20; ++i) {
  //  no_transpose(512, 128, 500, "asm");
  //  left_transpose(512, 128, 500, "asm");
  //  right_transpose(512, 128, 500, "asm");
  //}
  //left_transpose(1024, 1024, 1024, "asm");
  //left_transpose(1024, 1024, 1024, "blas");
  //std::cout << "conv5" << std::endl;
  //no_transpose(256, 172, 2304, "asm");
  //no_transpose(256, 169, 2304, "blas");
  //std::cout << "conv4" << std::endl;
  //no_transpose(256, 172, 3456, "asm");
  //no_transpose(256, 169, 3456, "blas");
  //std::cout << "conv3" << std::endl;
  //no_transpose(384, 172, 1728, "asm");
  //no_transpose(384, 169, 1728, "blas");
  //std::cout << "conv2" << std::endl;
  //no_transpose(192, 732, 1600, "asm");
  //no_transpose(192, 729, 1600, "blas");
  //std::cout << "conv1" << std::endl;
  //no_transpose(64, 3028, 363, "asm");
  //no_transpose(64, 3025, 363, "blas");

  //std::cout << "conv5" << std::endl;
  //left_transpose(2304, 256, 172, "asm");
  //left_transpose(2304, 256, 169, "blas");
  //std::cout << "conv4" << std::endl;
  //left_transpose(3456, 256, 172, "asm");
  //left_transpose(3456, 256, 169, "blas");
  //std::cout << "conv3" << std::endl;
  //left_transpose(1728, 384, 172, "asm");
  //left_transpose(1728, 384, 169, "blas");
  //std::cout << "conv2" << std::endl;
  //left_transpose(1024, 1024, 1024, "asm");
  //left_transpose(1024, 1024, 1024, "blas");
  //std::cout << "affine1" << std::endl;
  //right_transpose(128, 9216, 4096, "asm");
  //right_transpose(128, 9216, 4096, "blas");
  //std::cout << "affine2" << std::endl;
  //right_transpose(128, 4096, 4096, "asm");
  //right_transpose(128, 4096, 4096, "blas");
  //std::cout << "affine3" << std::endl;
  //right_transpose(128, 4096, 1000, "asm");
  //right_transpose(128, 4096, 1000, "blas");

  //std::cout << "affine1" << std::endl;
  //no_transpose(9216, 128, 4096, "asm");
  //no_transpose(9216, 128, 4096, "blas");
  //std::cout << "affine2" << std::endl;
  //no_transpose(4096, 128, 4096, "asm");
  //no_transpose(4096, 128, 4096, "blas");
  //std::cout << "affine3" << std::endl;
  //no_transpose(4096, 128, 1000, "asm");
  //no_transpose(4096, 128, 1000, "blas");

  //std::cout << "affine1" << std::endl;
  //left_transpose(128, 4096, 9216, "asm");
  //left_transpose(128, 4096, 9216, "blas");
  //std::cout << "affine2" << std::endl;
  //left_transpose(128, 4096, 4096, "asm");
  //left_transpose(128, 4096, 4096, "blas");
  //std::cout << "affine3" << std::endl;
  //left_transpose(128, 1000, 4096, "asm");
  //left_transpose(128, 1000, 4096, "blas");
  left_transpose(4096, 4096, 4096, "asm");
  left_transpose(4096, 4096, 4096, "blas");
  left_transpose(4096, 4096, 4096, "asm");
  left_transpose(4096, 4096, 4096, "blas");
  right_transpose(4096, 4096, 4096, "asm");
  right_transpose(4096, 4096, 4096, "blas");
  right_transpose(4096, 4096, 4096, "asm");
  right_transpose(4096, 4096, 4096, "blas");
  no_transpose(4096, 4096, 4096, "asm");
  no_transpose(4096, 4096, 4096, "blas");
  no_transpose(4096, 4096, 4096, "asm");
  no_transpose(4096, 4096, 4096, "blas");
  //left_transpose(1024, 1025, 1024, "asm");
  //left_transpose(1024, 1026, 1024, "asm");
  //left_transpose(1024, 1027, 1024, "asm");
  //left_transpose(1024, 1028, 1024, "asm");
  //left_transpose(1024, 1029, 1024, "asm");
  //left_transpose(1024, 1031, 1024, "asm");
  //left_transpose(1024, 1032, 1024, "asm");
  //left_transpose(1024, 1033, 1024, "asm");
  //left_transpose(1024, 1034, 1024, "asm");
  //left_transpose(1024, 1035, 1024, "asm");
  //left_transpose(1024, 1036, 1024, "asm");
  //left_transpose(1024, 1037, 1024, "asm");
  //left_transpose(1024, 1038, 1024, "asm");
  //left_transpose(1024, 1039, 1024, "asm");

  //right_transpose(1024, 1024, 1024, "asm");
  //right_transpose(1024, 1025, 1024, "asm");
  //right_transpose(1024, 1026, 1024, "asm");
  //right_transpose(1024, 1027, 1024, "asm");
  //right_transpose(1024, 1028, 1024, "asm");
  //right_transpose(1024, 1029, 1024, "asm");
  //right_transpose(1024, 1031, 1024, "asm");
  //right_transpose(1024, 1032, 1024, "asm");
  //right_transpose(1024, 1033, 1024, "asm");
  //right_transpose(1024, 1034, 1024, "asm");
  //right_transpose(1024, 1035, 1024, "asm");
  //right_transpose(1024, 1036, 1024, "asm");
  //right_transpose(1024, 1037, 1024, "asm");
  //right_transpose(1024, 1038, 1024, "asm");
  //right_transpose(1024, 1039, 1024, "asm");
}

int main() {
  std::cout << "start" << std::endl;

  cudaFree(0);
  //performance();
  correct();
  cudaFree(0);

  std::cout << "end" << std::endl;
  return 0;
}