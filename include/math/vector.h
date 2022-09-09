#pragma once

#include <algorithm>
#include <cassert>
#include <ostream>
#include <utility>

#include "linalg_common.h"

// Simple vector class
template <typename FloatT>
class Vector {
 public:
  explicit Vector(size_t size, FillType fill_type = FillType::kUninitialized)
      : size_(size), owns_data_(true) {
    assert(size);
    assert(fill_type == FillType::kZeros ||
           fill_type == FillType::kUninitialized);

    switch (fill_type) {
      case FillType::kZeros:
        data_ = new FloatT[size_]();
        break;
      case FillType::kUninitialized:
        data_ = new FloatT[size_];
        break;
    }

    assert(data_);
  }

  Vector(const Vector<FloatT>& other) : size_(other.size_), owns_data_(true) {
    data_ = new FloatT[size_];

    assert(data_);

    std::copy(other.data_, other.data_ + size_, data_);
  }

  Vector() = default;

  Vector<FloatT>& operator=(Vector<FloatT> other) {
    if (other.owns_data_) {
      std::swap(data_, other.data_);
      std::swap(size_, other.size_);
      std::swap(owns_data_, other.owns_data_);
    } else {
      if (owns_data_) {
        delete[] data_;
      }

      owns_data_ = true;
      size_ = other.size_;
      data_ = new FloatT[size_];
      std::copy(other.data_, other.data_ + size_, data_);
    }

    return *this;
  }

  Vector(FloatT* data, size_t size)
      : data_(data), size_(size), owns_data_(false) {}

  FloatT operator()(size_t i) const {
    assert(i < size_);
    return data_[i];
  }

  FloatT& operator()(size_t i) {
    assert(i < size_);
    return data_[i];
  }

  // Returns the sum of vector elements
  FloatT Sum() const;

  void MulElements(const Vector<FloatT>& other);

  void AddElements(const Vector<FloatT>& other);

  void CopyFromVector(const Vector<FloatT>& other) {
    assert(size_ == other.size_);

    std::copy(other.data_, other.data_ + size_, data_);
  }

  // Multiplies each element by alpha
  void Scale(FloatT alpha);

  const FloatT* GetData() const { return data_; }

  FloatT* GetData() { return data_; }

  const size_t& GetSize() const { return size_; }

  ~Vector() {
    if (owns_data_) {
      delete[] data_;
    }
  }

  friend std::ostream& operator<<(std::ostream& os, const Vector<FloatT>& obj) {
    os << "[";

    for (size_t i = 0; i < obj.size_; ++i) {
      os << " " << obj(i);
    }

    os << " ]";

    return os;
  }

 private:
  FloatT* data_{nullptr};
  size_t size_ = 0;
  bool owns_data_ = true;
};
