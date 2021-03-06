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

#include "alsfvm/equation/CUDACellComputer.hpp"
#include "alsfvm/equation/CPUCellComputer.hpp"
#include "alsfvm/equation/euler/Euler.hpp"
#include "alsfvm/equation/equation_list.hpp"
#include "alsfvm/cuda/cuda_utils.hpp"
#include <thrust/device_vector.h>
#include <thrust/reduce.h>

namespace alsfvm {
namespace equation {

namespace {
///
/// Computes the extra variables for each cell.
///
template<class Equation>
__global__ void computeExtraVariablesDevice(Equation eq,
    typename Equation::ConstViews conservedIn,
    typename Equation::ViewsExtra extraOut,
    size_t size) {

    size_t index = blockIdx.x * blockDim.x + threadIdx.x;

    if (index >= size) {
        return;
    }

    typename Equation::ConservedVariables conservedStruct =
        eq.fetchConservedVariables(conservedIn, index);
    typename Equation::ExtraVariables extraStruct = eq.computeExtra(
            conservedStruct);
    eq.setExtraViewAt(extraOut, index, extraStruct);
}


///
/// Computes the wave speed for each cell in the given direction.
/// To get the maximum wavespeed one needs to do a reduction.
///
template<class Equation, size_t direction>
__global__ void computeWaveSpeedDevice(Equation eq,
    typename Equation::ConstViews conserved,
    size_t size, real* outputPointer) {

    const size_t index = blockIdx.x * blockDim.x + threadIdx.x;

    if (index >= size) {
        return;
    }

    auto conservedStruct = eq.fetchConservedVariables(conserved, index);
    auto extraStruct = eq.computeExtra(conservedStruct);
    outputPointer[index] = eq.template computeWaveSpeed < direction >
        (conservedStruct, extraStruct);
}

///
/// Computes the wave speed for the given direction, then reduces the output (using thrust)
/// \note We might want to change the thrust implementation to something faster eventually, but we
/// will do more profiling first.
///
template<class Equation, size_t direction>
real computeWaveSpeedAndReduce(const Equation& eq,
    const volume::Volume& conservedVariables,
    thrust::device_vector<real>& deviceVector) {
    const size_t size = conservedVariables.getScalarMemoryArea(0)->getSize();
    const size_t blockSize = 1024;

    computeWaveSpeedDevice<Equation, direction> << < (size + blockSize - 1) /
        blockSize, blockSize >> > (eq,
            typename Equation::ConstViews(conservedVariables),
             size,
            thrust::raw_pointer_cast(&deviceVector[0]));
    CUDA_SAFE_CALL(cudaStreamSynchronize(0));
    // For now we simply use thrust to reduce.
    return thrust::reduce(deviceVector.begin(), deviceVector.end(), 0.0,
            thrust::maximum<real>());
}


///
/// Checks for each cell if we obey the constraints given.
/// One should do a reduction over the results to get if all cells obeys or not.
///
template<class Equation>
__global__ void checkObeysConstraintsDevice(Equation eq,
    typename Equation::ConstViews conserved,
    size_t size, size_t* outputPointer) {
    size_t index = blockIdx.x * blockDim.x + threadIdx.x;

    if (index >= size) {
        return;
    }


    auto conservedStruct = eq.fetchConservedVariables(conserved, index);
    auto extraStruct = eq.computeExtra(conservedStruct);
    outputPointer[index] = eq.obeysConstraints(conservedStruct, extraStruct);
}


///
/// Checks if each cells obeys the constraints, then reduces the output using thrust.
///
template<class Equation>
bool checkObeysConstraintsAndReduce(const Equation& equation,
    const volume::Volume& conservedVariables,
    thrust::device_vector<size_t>& deviceVector) {

    const size_t size = conservedVariables.getScalarMemoryArea(0)->getSize();
    const size_t blockSize = 1024;


    checkObeysConstraintsDevice<Equation> << < (size + blockSize - 1) / blockSize,
                                blockSize >> > (equation, typename Equation::ConstViews(conservedVariables),

                                    size, thrust::raw_pointer_cast(&deviceVector[0]));
    CUDA_SAFE_CALL(cudaStreamSynchronize(0));
    // For now we simply use thrust to reduce.
    return bool(thrust::reduce(deviceVector.begin(), deviceVector.end(), 0,
                thrust::maximum<size_t>()));
}


}

template<class Equation>
void CUDACellComputer<Equation>::computeExtraVariables(const volume::Volume&
    conservedVariables,
    volume::Volume& extraVariables) {
    const size_t size = conservedVariables.getScalarMemoryArea(0)->getSize();
    const size_t blockSize = 1024;


    computeExtraVariablesDevice<Equation> << < (size + blockSize - 1) / blockSize,
                                blockSize >> > (equation, typename Equation::ConstViews(conservedVariables),
                                    typename Equation::ViewsExtra(extraVariables), size);
}


template<class Equation>
real CUDACellComputer<Equation>::computeMaxWaveSpeed(const volume::Volume&
    conservedVariables, size_t direction) {

    // We declare this static to avoid having to reallocate it every time,
    // and to avoid having to expose it in the class interface.
    static thrust::device_vector<real> deviceVector;
    deviceVector.resize(conservedVariables.getScalarMemoryArea(0)->getSize(), 0.0);
    assert(direction < 3);

    if (direction == 0) {
        return computeWaveSpeedAndReduce<Equation, 0>(equation, conservedVariables,
                deviceVector);
    }

    if (direction == 1) {
        return computeWaveSpeedAndReduce<Equation, 1>(equation, conservedVariables,
                deviceVector);
    }

    if (direction == 2) {
        return computeWaveSpeedAndReduce<Equation, 2>(equation, conservedVariables,
                deviceVector);
    }

    THROW("Unknown direction: " << direction);

}

///
/// Checks if all the constraints for the equation are met
/// \param conservedVariables the conserved variables (density, momentum, Energy for Euler)
/// \param extraVariables the extra variables (pressure and velocity for Euler)
/// \return true if it obeys the constraints, false otherwise
/// \todo Tidy up the way we check for nan and inf
///
template<class Equation>
bool CUDACellComputer<Equation>::obeysConstraints(const volume::Volume&
    conservedVariables) {
    static thrust::device_vector<size_t> deviceVector;
    deviceVector.resize(conservedVariables.getScalarMemoryArea(0)->getSize(), 0);

    return checkObeysConstraintsAndReduce<Equation>(equation, conservedVariables,
             deviceVector);
}

template<class Equation>
void CUDACellComputer<Equation>::computeFromPrimitive(const volume::Volume&
    primitiveVariables,
    volume::Volume& conservedVariables) {
    THROW("Unsupported operation. We do not support calculating primitive variables on the GPU, this should be done on the CPU for now.")
}

template<class Equation> CUDACellComputer<Equation>::CUDACellComputer(
    simulator::SimulatorParameters& simulatorParameters)
    : equation(static_cast<typename Equation::Parameters&>
          (simulatorParameters.getEquationParameters())) {

}

ALSFVM_EQUATION_INSTANTIATE(CUDACellComputer)
}
}


