#pragma once
#include "alsfvm/numflux/NumericalFlux.hpp"
#include "alsfvm/grid/Grid.hpp"

namespace alsfvm { namespace numflux { namespace euler { 

	///
	/// The class to compute numerical flux on the CPU
	/// The template argument Flux is used to choose the concrete flux
    /// The template argument dimension is to choose the correct dimension
    /// (1 up to and including 3 is supported).
	///
    template<class Flux, size_t dimension>
    class NumericalFluxCPU : public NumericalFlux {
    public:

        NumericalFluxCPU(const grid::Grid& grid,
                         const std::shared_ptr<DeviceConfiguration>& deviceConfiguration);

		virtual void computeFlux(const volume::Volume& conservedVariables,
			const volume::Volume& extraVariables,
			const rvec3& cellLengths,
			volume::Volume& output
			);

		/// 
		/// \returns the number of ghost cells this specific flux requires
		///
		virtual size_t getNumberOfGhostCells();

    };

} // namespace alsfvm
} // namespace numflux
} // namespace euler
