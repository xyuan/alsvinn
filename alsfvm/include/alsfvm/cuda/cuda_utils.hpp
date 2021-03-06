/* Copyright (c) 2018 ETH Zurich, Kjetil Olsen Lye
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once
#include "cuda.h"
#include "cuda_runtime.h"
#include <iostream>
#include <exception>
#include "alsutils/types.hpp"
#include "alsutils/error/Exception.hpp"
#include "alsutils/cuda/cuda_safe_call.hpp"

#ifdef NDEBUG
#define CUDA_CHECK_IF_DEBUG
#else
//! If in debug mode, checks if there has been any error
//! with cuda.
#define CUDA_CHECK_IF_DEBUG { \
    CUDA_SAFE_CALL(cudaGetLastError()); \
CUDA_SAFE_CALL(cudaDeviceSynchronize()); \
CUDA_SAFE_CALL(cudaGetLastError()); \
}
#endif
namespace alsfvm {
namespace cuda {
inline dim3 calculateBlockDimensions(size_t numberOfXCells,
    size_t numberOfYCells, size_t numberOfZCells) {
    const size_t blockSize = 1024;
    return dim3(blockSize, numberOfYCells > 1 ? blockSize : 1,
            numberOfZCells > 1 ? blockSize : 1);
}

inline dim3 calculateGridDimensions(size_t numberOfXCells,
    size_t numberOfYCells, size_t numberOfZCells, dim3 blockDimensions) {
    return dim3((numberOfXCells + blockDimensions.x - 1) / blockDimensions.x,
            (numberOfYCells + blockDimensions.y - 1) / blockDimensions.y,
            (numberOfZCells + blockDimensions.z - 1) / blockDimensions.z);
}


//! Gets the spatial (in grid cell integer coordinates) coordinates
//!
//! This is typically useful when launching a CUDA kernel that needs
//! neighbouring cells
inline ivec3 __device__ getCoordinates(dim3 threadIdx, dim3 blockIdx,
    dim3 blockDim,
    size_t numberOfXCells, size_t numberOfYCells,
    size_t numberOfZCells, ivec3 directionVector) {
    const int index = threadIdx.x + blockIdx.x * blockDim.x;

    const size_t xInternalFormat = index % numberOfXCells;
    const size_t yInternalFormat = (index / numberOfXCells) % numberOfYCells;
    const size_t zInternalFormat = (index) / (numberOfXCells * numberOfYCells);

    if (xInternalFormat >= numberOfXCells || yInternalFormat >= numberOfYCells
        || zInternalFormat >= numberOfZCells) {
        return {-1, -1, -1};
    }

    const int x = xInternalFormat + directionVector[0];
    const int y = yInternalFormat + directionVector[1];
    const int z = zInternalFormat + directionVector[2];

    return {x, y, z};
}


//! Gets teh kernel launch paramemters.
inline std::tuple<int, ivec3> makeKernelLaunchParameters(ivec3 start, ivec3 end,
    size_t blockSize) {
    const ivec3 numberOfCellsPerDimension = end - start;

    const size_t totalNumberOfCells = size_t(numberOfCellsPerDimension.x) *
        size_t(numberOfCellsPerDimension.y) *
        size_t(numberOfCellsPerDimension.z);

    const int gridSize = (totalNumberOfCells + blockSize - 1 ) / blockSize;

    return std::make_tuple(gridSize, numberOfCellsPerDimension);
}
}
}
