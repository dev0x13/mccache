#pragma once

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

    Matrix(size_t numRows, size_t numCols, FillType fillType = FillType::UNITIALIZED)
        : numRows(numRows), numCols(numCols)
    {
        assert(numRows);
        assert(numCols);
        assert(fillType == FillType::ZEROS || fillType == FillType::UNITIALIZED);

        switch (fillType) {
            case FillType::ZEROS:
                data = new FloatT[numCols * numRows]();
                break;
            case FillType::UNITIALIZED:
                data = new FloatT[numCols * numRows];
                break;
        }

        assert(data);
    }

    Matrix() = default;

    FloatT operator()(size_t row, size_t col) const {
        assert(row < numRows);
        assert(col < numCols);

        return data[row * numCols + col];
    }

    FloatT& operator()(size_t row, size_t col) {
        assert(row < numRows);
        assert(col < numCols);

        return data[row * numCols + col];
    }

    void resize(size_t newNumRows, size_t newNumCols, ResizeType resizeType) {
        assert(newNumRows);
        assert(newNumCols);
        assert(resizeType == ResizeType::ZEROS || resizeType == ResizeType::UNITIALIZED || resizeType == ResizeType::COPY);

        if (newNumCols == numCols && newNumRows == numRows) {
            return;
        }

        FloatT *newData = nullptr;

        switch (resizeType) {
            case ResizeType::ZEROS:
                newData = new FloatT[newNumRows * newNumCols]();
                break;
            case ResizeType::UNITIALIZED:
                newData = new FloatT[newNumRows * newNumCols];
                break;
            case ResizeType::COPY:
                newData = new FloatT[newNumRows * newNumCols]();

                if (newNumRows > numRows && newNumCols > numCols) {
                    for (size_t i = 0; i < numRows; ++i) {
                        std::copy(data + i * numCols, data + (i + 1) * numCols, newData + i * newNumCols);
                    }
                }

                break;
        }

        assert(newData);
        delete[] data;
        data = newData;
        numRows = newNumRows;
        numCols = newNumCols;
    }

    Vector<FloatT> row(size_t row) {
        assert(row < numRows);

        return { data + row * numCols, numCols };
    }

    Vector<FloatT> row(size_t row) const {
        assert(row < numRows);

        return { data + row * numCols, numCols };
    }

    const FloatT* getData() const {
        return data;
    }

    FloatT* getData() {
        return data;
    }

    // Returns matrix-vector multiplication result
    void matMulVec(const Vector<FloatT>& vec, Vector<FloatT>* output) const {
        assert(output);
        assert(vec.getSize() == numRows);
        assert(vec.getSize() == output->getSize());

        for (size_t i = 0; i < numRows; ++i) {
            for (size_t j = 0; j < numCols; ++j) {
                (*output)(i) += vec(j) * (*this)(i, j);
            }
        }
    }

    const size_t& getNumRows() const {
        return numRows;
    }

    const size_t& getNumCols() const {
        return numCols;
    }

    friend std::ostream &operator<<(std::ostream& os, const Matrix<FloatT>& obj) {
        for (size_t i = 0; i < obj.numRows; ++i) {
            os << obj.row(i) << "\n";
        }

        return os;
    }

    ~Matrix() {
        delete[] data;
    }

private:
    size_t numRows = 0;
    size_t numCols = 0;

    FloatT* data{ nullptr };
};