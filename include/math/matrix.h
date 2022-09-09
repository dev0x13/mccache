#pragma once

#include <algorithm>
#include <cassert>
#include <cstring>
#include <ostream>

#include "linalg_common.h"
#include "vector.h"

// Simple row-major matrix class
template <typename FloatT>
class Matrix {
 public:
  Matrix<FloatT>& operator=(const Matrix<FloatT>& other) = delete;

  Matrix(size_t num_rows, size_t num_cols,
         FillType fill_type = FillType::kUninitialized)
      : num_rows_(num_rows), num_cols_(num_cols) {
    assert(num_rows);
    assert(num_cols);
    assert(fill_type == FillType::kZeros || fill_type == FillType::kUninitialized);

    switch (fill_type) {
      case FillType::kZeros:
        data_ = new FloatT[num_cols_ * num_rows_]();
        break;
      case FillType::kUninitialized:
        data_ = new FloatT[num_cols_ * num_rows_];
        break;
    }

    assert(data_);
  }

  Matrix() = default;

  FloatT operator()(size_t row, size_t col) const {
    assert(row < num_rows_);
    assert(col < num_cols_);

    return data_[row * num_cols_ + col];
  }

  FloatT& operator()(size_t row, size_t col) {
    assert(row < num_rows_);
    assert(col < num_cols_);

    return data_[row * num_cols_ + col];
  }

  void Resize(size_t newNumRows, size_t newNumCols, ResizeType resizeType) {
    assert(newNumRows);
    assert(newNumCols);
    assert(resizeType == ResizeType::kZeros ||
           resizeType == ResizeType::kUninitialized ||
           resizeType == ResizeType::kCopy);

    if (newNumCols == num_cols_ && newNumRows == num_rows_) {
      return;
    }

    FloatT* newData = nullptr;

    switch (resizeType) {
      case ResizeType::kZeros:
        newData = new FloatT[newNumRows * newNumCols]();
        break;
      case ResizeType::kUninitialized:
        newData = new FloatT[newNumRows * newNumCols];
        break;
      case ResizeType::kCopy:
        newData = new FloatT[newNumRows * newNumCols]();

        if (newNumRows > num_rows_ && newNumCols > num_cols_) {
          for (size_t i = 0; i < num_rows_; ++i) {
            std::copy(data_ + i * num_cols_, data_ + (i + 1) * num_cols_,
                      newData + i * newNumCols);
          }
        }

        break;
    }

    assert(newData);
    delete[] data_;
    data_ = newData;
    num_rows_ = newNumRows;
    num_cols_ = newNumCols;
  }

  Vector<FloatT> Row(size_t row) {
    assert(row < num_rows_);

    return {data_ + row * num_cols_, num_cols_};
  }

  Vector<FloatT> Row(size_t row) const {
    assert(row < num_rows_);

    return {data_ + row * num_cols_, num_cols_};
  }

  const FloatT* GetData() const { return data_; }

  FloatT* GetData() { return data_; }

  // Returns transposed matrix-vector multiplication result
  void TransMatMulVec(const Vector<FloatT>& vec, Vector<FloatT>* output) const;

  const size_t& GetNumRows() const { return num_rows_; }

  const size_t& GetNumCols() const { return num_cols_; }

  friend std::ostream& operator<<(std::ostream& os, const Matrix<FloatT>& obj) {
    for (size_t i = 0; i < obj.numRows; ++i) {
      os << obj.row(i) << "\n";
    }

    return os;
  }

  ~Matrix() { delete[] data_; }

 private:
  size_t num_rows_ = 0;
  size_t num_cols_ = 0;

  FloatT* data_{nullptr};
};
