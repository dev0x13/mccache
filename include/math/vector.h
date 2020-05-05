#pragma once

#include <ostream>
#include <cassert>

#include "linalg_common.h"

// Simple vector class
template <typename FloatT>
class Vector {
public:
    explicit Vector(size_t size, FillType fillType = FillType::UNITIALIZED)
        : size(size), ownsData(true)
    {
        assert(size);
        assert(fillType == FillType::ZEROS || fillType == FillType::UNITIALIZED);

        switch (fillType) {
            case FillType::ZEROS:
                data = new FloatT[size]();
                break;
            case FillType::UNITIALIZED:
                data = new FloatT[size];
                break;
        }

        assert(data);
    }

    Vector(const Vector<FloatT>& other) : size(other.size), ownsData(true) {
        data = new FloatT[size];

        assert(data);

        std::copy(other.data, other.data + size, data);
    }

    Vector() = default;

    Vector<FloatT>& operator=(Vector<FloatT> other) {
        if (other.ownsData) {
            std::swap(data, other.data);
            std::swap(size, other.size);
            std::swap(ownsData, other.ownsData);
        } else {
            if (ownsData) {
                delete[] data;
            }

            ownsData = true;
            size = other.size;
            data = new FloatT[size];
            std::copy(other.data, other.data + size, data);
        }

        return *this;
    }

    Vector(FloatT* data, size_t size)
            : data(data), size(size), ownsData(false) {}

    FloatT operator()(size_t i) const {
        assert(i < size);
        return data[i];
    }

    FloatT& operator()(size_t i) {
        assert(i < size);
        return data[i];
    }

    // Returns the sum of vector elements
    FloatT sum() const;

    void mulElements(const Vector<FloatT>& other);

    void addElements(const Vector<FloatT>& other);

    void copyFromVector(const Vector<FloatT>& other) {
        assert(size == other.size);

        std::copy(other.data, other.data + size, data);
    }

    // Multiplies each elements by alpha
    void scale(FloatT alpha);

    const FloatT* getData() const {
        return data;
    }

    FloatT* getData(){
        return data;
    }

    const size_t& getSize() const {
        return size;
    }

    ~Vector() {
        if (ownsData) {
            delete[] data;
        }
    }

    friend std::ostream &operator<<(std::ostream& os, const Vector<FloatT>& obj) {
        os << "[";

        for (size_t i = 0; i < obj.size; ++i) {
            os << " " << obj(i);
        }

        os << " ]";

        return os;
    }
private:
    FloatT* data{ nullptr };
    size_t size = 0;
    bool ownsData = true;
};
