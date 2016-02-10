#include "alsfvm/memory/HostMemory.hpp"
#include "alsfvm/memory/memory_utils.hpp"
#include <cassert>
#include <algorithm>
#include "alsfvm/error/Exception.hpp"

namespace alsfvm {
namespace memory {

template<class T>
HostMemory<T>::HostMemory(size_t nx, size_t ny, size_t nz)
    : Memory<T>(nx, ny, nz), data(nx*ny*nz, 42)
{

}

template<class T>
bool HostMemory<T>::isOnHost() const
{
    return true;
}

template<class T>
T *HostMemory<T>::getPointer()
{
    return data.data();
}

template<class T>
const T *HostMemory<T>::getPointer() const
{
	return data.data();
}

template<class T>
void HostMemory<T>::copyToHost(T *bufferPointer, size_t bufferLength) const
{
    assert(bufferLength >= Memory<T>::getSize());
    std::copy(data.begin(), data.end(), bufferPointer);
}

template<class T>
void HostMemory<T>::copyFromHost(const T* bufferPointer, size_t bufferLength)
{
    const size_t sizeToCopy = std::min(bufferLength, Memory<T>::getSize());
    std::copy(bufferPointer, bufferPointer+sizeToCopy, data.begin());
}



///
/// Adds the other memory area to this one
/// \param other the memory area to add from
///
template <class T>
void HostMemory<T>::operator+=(const Memory<T>& other) {
	if (!other.isOnHost()) {
		THROW("Memory not on host");
	}

	auto pointer = other.getPointer();
    #pragma omp parallel for
    for (int i = 0; i < int(data.size()); ++i) {
		data[i] += pointer[i];
	}
}

///
/// Mutliplies the other memory area to this one
/// \param other the memory area to multiply from
///
template <class T>
void HostMemory<T>::operator*=(const Memory<T>& other) {
	if (!other.isOnHost()) {
		THROW("Memory not on host");
	}

	if (other.getSize() != this->getSize()) {
		THROW("Memory size not the same");
	}

	auto pointer = other.getPointer();
    #pragma omp parallel for
    for (int i = 0; i < int(data.size()); ++i) {
		data[i] *= pointer[i];
	}
}

///
/// Subtracts the other memory area to this one
/// \param other the memory area to subtract from
///
template <class T>
void HostMemory<T>::operator-=(const Memory<T>& other) {
	if (!other.isOnHost()) {
		THROW("Memory not on host");
	}
	if (other.getSize() != this->getSize()) {
		THROW("Memory size not the same");
	}

	auto pointer = other.getPointer();
    #pragma omp parallel for
    for (int i = 0; i < int(data.size()); ++i) {
		data[i] -= pointer[i];
	}
}

///
/// Divides the other memory area to this one
/// \param other the memory area to divide from
///
template <class T>
void HostMemory<T>::operator/=(const Memory<T>& other) {
	if (!other.isOnHost()) {
		THROW("Memory not on host");
	}
	if (other.getSize() != this->getSize()) {
		THROW("Memory size not the same");
	}

	auto pointer = other.getPointer();
    #pragma omp parallel for
    for (int i = 0; i < int(data.size()); ++i) {
		data[i] /= pointer[i];
	}
}

///
/// Adds the scalar to each component
/// \param scalar the scalar to add
///
template <class T>
void HostMemory<T>::operator+=(real scalar) {

#pragma omp parallel for
    for (int i = 0; i < int(data.size()); ++i) {
		data[i] += scalar;
	}
}

///
/// Multiplies the scalar to each component
/// \param scalar the scalar to multiply
///
template <class T>
void HostMemory<T>::operator*=(real scalar) {
#pragma omp parallel for
    for (int i = 0; i < int(data.size()); ++i) {
		data[i] *= scalar;
	}
}

///
/// Subtracts the scalar from each component
/// \param scalar the scalar to subtract
///
template <class T>
void HostMemory<T>::operator-=(real scalar) {
#pragma omp parallel for
    for (int i = 0; i < int(data.size()); ++i) {
		data[i] -= scalar;
	}
}

///
/// Divides the each component by the scalar
/// \param scalar the scalar to divide
///
template <class T>
void HostMemory<T>::operator/=(real scalar) {
    #pragma omp parallel for
    for (int i = 0; i < int(data.size()); ++i) {
		data[i] /= scalar;
    }
}

template <class T>
void HostMemory<T>::makeZero()
{
    #pragma omp parallel for
    for (int i = 0; i < int(data.size()); ++i) {
        data[i] = 0;
    }
}


template <class T>
void HostMemory<T>::copyInternalCells(size_t startX, size_t endX, size_t startY, size_t endY, size_t startZ, size_t endZ, T *output, size_t outputSize)
{
    const size_t nx = this->nx;
    const size_t ny = this->ny;
    const size_t numberOfY = endY-startY;
    const size_t numberOfX = endX-startX;
    for(size_t z = startZ; z < endZ; z++) {
        for(size_t y = startY; y < endY; y++) {
            for(size_t x = startX; x < endX; x++) {
                size_t indexIn = z * nx * ny + y * nx + x;
                size_t indexOut = (z-startZ) * numberOfX * numberOfY
                      + (y - startY) * numberOfY + (x - startX);
                output[indexOut] = data[indexIn];
             }
        }
    }
}

INSTANTIATE_MEMORY(HostMemory)
} // namespace memory
} // namespace alsfvm

