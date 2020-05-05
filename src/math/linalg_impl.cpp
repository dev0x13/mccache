#include <math/matrix.h>
#include <math/vector.h>

#ifdef USE_MKL

#include <mkl.h>

template<> void Matrix<float>::transMatMulVec(const Vector<float>& vec, Vector<float>* output) const {
    assert(output);
    assert(vec.getSize() == numRows);
    assert(vec.getSize() == output->getSize());

    cblas_sgemv(
        CblasRowMajor,
        CblasTrans,
        numRows,
        numCols,
        1,
        data,
        numRows,
        vec.getData(),
        1,
        0,
        output->getData(),
        1
    );
}

template<> float Vector<float>::sum() const {
    return cblas_sasum(size, data, 1);
}

template<> void Vector<float>::scale(float alpha) {
    cblas_sscal(size, alpha, data, 1);
}

template<> void Vector<float>::addElements(const Vector<float>& other) {
    assert(size == other.size);

    vsAdd(size, other.data, data, data);
}

template<> void Vector<float>::mulElements(const Vector<float>& other) {
    assert(size == other.size);

    vsMul(size, other.data, data, data);
}

#else

template<> void Matrix<float>::transMatMulVec(const Vector<float>& vec, Vector<float>* output) const {
    assert(output);
    assert(vec.getSize() == numRows);
    assert(vec.getSize() == output->getSize());

    for (size_t i = 0; i < numCols; ++i) {
        (*output)(i) = 0;
        for (size_t j = 0; j < numRows; ++j) {
            (*output)(i) += vec(j) * (*this)(j, i);
        }
    }
}

template<> float Vector<float>::sum() const {
    float sum = 0;

    for (size_t i = 0; i < size; ++i) {
        sum += data[i];
    }

    return sum;
}

template<> void Vector<float>::scale(float alpha) {
    for (size_t i = 0; i < size; ++i) {
        data[i] *= alpha;
    }
}

template<> void Vector<float>::addElements(const Vector<float>& other) {
    assert(size == other.size);

    for (size_t i = 0; i < size; ++i) {
        data[i] += other(i);
    }
}

template<> void Vector<float>::mulElements(const Vector<float>& other) {
    assert(size == other.size);

    for (size_t i = 0; i < size; ++i) {
        data[i] *= other(i);
    }
}

#endif