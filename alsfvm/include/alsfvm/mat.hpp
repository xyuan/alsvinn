#pragma once
namespace alsfvm {

    template<class T, size_t NumberOfRows, size_t NumberOfColumns>
    class matrix {
    public:
        //! Creates a matrix initialized to 0.
        __device__ __host__  matrix() {

            for (size_t column = 0; column < NumberOfColumns; ++column) {
                for (size_t row = 0; row < NumberOfRows; ++row) {
                    data[column][row] = T(0);
                }
            }
        }

        //! Get the matrix element at the given row and column.
        __device__ __host__ const T& operator()(size_t row, size_t column) const {
            return data[column][row];
        }

        //! Get the matrix element at the given row and column.
        __device__ __host__ T& operator()(size_t row, size_t column) {
            return data[column][row];
        }

        
        //! Matrix-vector multiplication. We only support this for 
        //! quadratic matrices as of now.
        template<class VectorType>
        __device__ __host__ VectorType operator*(const VectorType& vector) const {
            static_assert(NumberOfColumns == NumberOfRows,
                "Matrix-Vector multiplication only supported for quadratic matrices.");
            static_assert(VectorType::size() == NumberOfColumns, 
                "Matrix vector multiplication given wrong dimensions");

            VectorType product;

            for (int column = 0; column < NumberOfColumns; ++column) {
                for (int row = 0; row < NumberOfRows; ++row) {
                    product[row] += (*this)(row, column) *  vector[column];
                }
            }

            return product;
        }

        __device__ __host__ matrix<T, NumberOfColumns, NumberOfRows> transposed() const {
            matrix<T, NumberOfColumns, NumberOfRows> transposedMatrix;
            for (int column = 0; column < NumberOfColumns; ++column) {
                for (int row = 0; row < NumberOfRows; ++row) {
                    transposedMatrix(row, column) = (*this)(column, row);
                }
            }

            return transposedMatrix;
        }

        __device__ __host__ matrix<T, NumberOfColumns, NumberOfRows> normalized() const {
            matrix<T, NumberOfColumns, NumberOfRows> newMatrix;
            for (int column = 0; column < NumberOfColumns; ++column) {
                T norm = 0;
                for (int row = 0; row < NumberOfRows; ++row) {
                    norm += (*this)(row, column)*(*this)(row, column);
                }
                norm = sqrt(norm);
                for (int row = 0; row < NumberOfRows; ++row) {
                    newMatrix(row, column) = (*this)(row, column) /norm;
                }
            }

            return newMatrix;
        }

        static __device__ __host__ matrix<T, NumberOfColumns, NumberOfRows> identity() {
            static_assert(NumberOfColumns == NumberOfRows,
                "Matrix-Vector multiplication only supported for quadratic matrices.");
            matrix<T, NumberOfColumns, NumberOfRows> identityMatrix;
            for (int i = 0; i < NumberOfColumns; ++i) {
                identityMatrix(i, i) = 1;
            }

            return identityMatrix;
        }
    private:
        T data[NumberOfColumns][NumberOfRows];
    };
}