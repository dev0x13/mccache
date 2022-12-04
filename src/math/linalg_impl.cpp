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

template <>
void Matrix<float>::TransMatMulVec(const Vector<float>& vec,
                                   Vector<float>* output) const {
  assert(output);
  assert(vec.GetSize() == num_rows_);
  assert(vec.GetSize() == output->GetSize());

  for (size_t i = 0; i < num_cols_; ++i) {
    (*output)(i) = 0;
    for (size_t j = 0; j < num_rows_; ++j) {
      (*output)(i) += vec(j) * (*this)(j, i);
    }
  }
}

template <>
float Vector<float>::Sum() const {
  float sum = 0;

  for (size_t i = 0; i < size_; ++i) {
    sum += data_[i];
  }

  return sum;
}

template <>
void Vector<float>::Scale(float alpha) {
  for (size_t i = 0; i < size_; ++i) {
    data_[i] *= alpha;
  }
}

template <>
void Vector<float>::AddElements(const Vector<float>& other) {
  assert(size_ == other.size_);

  for (size_t i = 0; i < size_; ++i) {
    data_[i] += other(i);
  }
}

template <>
void Vector<float>::MulElements(const Vector<float>& other) {
  assert(size_ == other.size_);

  for (size_t i = 0; i < size_; ++i) {
    data_[i] *= other(i);
  }
}

#endif
