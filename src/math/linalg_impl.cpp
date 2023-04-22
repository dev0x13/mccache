#include <math/matrix.h>
#include <math/vector.h>

#ifdef USE_MKL

#include <mkl/mkl.h>

template <>
void Matrix<float>::TransMatMulVec(const Vector<float>& vec,
                                   Vector<float>* output) const {
  assert(output);
  assert(vec.GetSize() == num_rows_);
  assert(vec.GetSize() == output->GetSize());

  cblas_sgemv(CblasRowMajor, CblasTrans, num_rows_, num_cols_, 1, data_,
              num_rows_, vec.GetData(), 1, 0, output->GetData(), 1);
}

template <>
float Vector<float>::Sum() const {
  return cblas_sasum(size_, data_, 1);
}

template <>
void Vector<float>::Scale(float alpha) {
  cblas_sscal(size_, alpha, data_, 1);
}

template <>
void Vector<float>::AddElements(const Vector<float>& other) {
  assert(size_ == other.size_);

  vsAdd(size_, other.data_, data_, data_);
}

template <>
void Vector<float>::MulElements(const Vector<float>& other) {
  assert(size_ == other.size_);

  vsMul(size_, other.data_, data_, data_);
}

#else

#include <cublas_v2.h>
#include <cuda_runtime.h>
#include <mkl/mkl.h>

struct CuBlas {
  CuBlas() {
    if (cublasCreate_v2(&handle) != CUBLAS_STATUS_SUCCESS) {
      throw std::runtime_error("cublasCreate_v2 failed");
    }
  }

  ~CuBlas() {
    cublasDestroy_v2(handle);
  }

  cublasHandle_t handle = nullptr;
};

CuBlas cb;

//template <>
//void Matrix<float>::TransMatMulVec(const Vector<float>& vec,
//                                   Vector<float>* output) const {
//  assert(output);
//  assert(vec.GetSize() == num_rows_);
//  assert(vec.GetSize() == output->GetSize());
//
//  for (size_t i = 0; i < num_cols_; ++i) {
//    (*output)(i) = 0;
//    for (size_t j = 0; j < num_rows_; ++j) {
//      (*output)(i) += vec(j) * (*this)(j, i);
//    }
//  }
//}
//
//template <>
//float Vector<float>::Sum() const {
//  float sum = 0;
//
//  for (size_t i = 0; i < size_; ++i) {
//    sum += data_[i];
//  }
//
//  return sum;
//}
//
//template <>
//void Vector<float>::Scale(float alpha) {
//  for (size_t i = 0; i < size_; ++i) {
//    data_[i] *= alpha;
//  }
//}
//
//template <>
//void Vector<float>::AddElements(const Vector<float>& other) {
//  assert(size_ == other.size_);
//
//  for (size_t i = 0; i < size_; ++i) {
//    data_[i] += other(i);
//  }
//}
//
//template <>
//void Vector<float>::MulElements(const Vector<float>& other) {
//  assert(size_ == other.size_);
//
//  for (size_t i = 0; i < size_; ++i) {
//    data_[i] *= other(i);
//  }
//}

template <>
void Matrix<float>::TransMatMulVec(const Vector<float>& vec,
                                   Vector<float>* output) const {
  assert(output);
  assert(vec.GetSize() == num_rows_);
  assert(vec.GetSize() == output->GetSize());

  float* cuda_vec;
  float* cuda_mat;
  float* cuda_output;
  cudaMalloc(&cuda_vec, vec.GetSize() * sizeof(float));
  cudaMalloc(&cuda_mat, GetNumCols() * GetNumRows() * sizeof(float));
  cudaMalloc(&cuda_output, output->GetSize() * sizeof(float));

  cudaMemcpy(cuda_vec, vec.GetData(), vec.GetSize() * sizeof(float),
             cudaMemcpyHostToDevice);
  cudaMemcpy(cuda_mat, GetData(), GetNumCols() * GetNumRows() * sizeof(float),
             cudaMemcpyHostToDevice);

  static float alpha = 1;
  cublasSgemv_v2(cb.handle, CUBLAS_OP_T, num_rows_, num_cols_, &alpha, cuda_mat,
                 num_rows_, cuda_vec, 1, 0, cuda_output, 1);
  cudaMemcpy(output->GetData(), cuda_output, output->GetSize() * sizeof(float),
             cudaMemcpyDeviceToHost);
  cudaFree(cuda_vec);
  cudaFree(cuda_mat);
  cudaFree(cuda_output);
}

template <>
float Vector<float>::Sum() const {
  float* cuda_vec;
  cudaMalloc(&cuda_vec, GetSize() * sizeof(float));
  float res;
  cublasSasum_v2(cb.handle, size_, cuda_vec, 1, &res);
  cudaFree(cuda_vec);
  return res;
}

template <>
void Vector<float>::Scale(float alpha) {
  float* cuda_vec;
  cudaMalloc(&cuda_vec, GetSize() * sizeof(float));
  cublasSscal_v2(cb.handle, size_, &alpha, data_, 1);
  cudaFree(cuda_vec);
}

template <>
void Vector<float>::AddElements(const Vector<float>& other) {
  assert(size_ == other.size_);

  vsAdd(size_, other.data_, data_, data_);
}

template <>
void Vector<float>::MulElements(const Vector<float>& other) {
  assert(size_ == other.size_);

  vsMul(size_, other.data_, data_, data_);
}

#endif
