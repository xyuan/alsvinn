#pragma once
#include "alsfvm/numflux/NumericalFlux.hpp"
#include "alsfvm/grid/Grid.hpp"
#include "alsfvm/reconstruction/Reconstruction.hpp"
#include "alsfvm/simulator/SimulatorParameters.hpp"
#include "alsfvm/equation/euler/Euler.hpp"

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
                         alsfvm::shared_ptr<reconstruction::Reconstruction>& reconstruction,
                         const alsfvm::shared_ptr<simulator::SimulatorParameters>& simulatorParameters,
                         alsfvm::shared_ptr<DeviceConfiguration>& deviceConfiguration
                         );

		/// 
		/// Computes the numerical flux at each cell. 
		/// This will compute the net flux in the cell, ie.
		/// \f[
		/// \mathrm{output}_{i,j,k}=\frac{\Delta t}{\Delta x}\left(F(u_{i+1,j,k}, u_{i,j,k})-F(u_{i,j,k}, u_{i-1,j,k})\right)+
		///                         \frac{\Delta t}{\Delta y}\left(F(u_{i,j+1,k}, u_{i,j,k})-F(u_{i,j,k}, u_{i,j-1,k})\right)+
		///                         \frac{\Delta t}{\Delta z}\left((F(u_{i,j,k+1}, u_{i,j,k})-F(u_{i,j,k}, u_{i,j,k-1})\right)
		/// \f]
		/// \param[in] conservedVariables the conservedVariables to read from (eg. for Euler: \f$\rho,\; \vec{m},\; E\f$)
		/// \param[out] waveSpeed the maximum wave speed in each direction
		/// \param[in] computeWaveSpeed should we compute the wave speeds?
		/// \param[out] output the output to write to
		///
		/// \note this will calculate the extra variables on the fly.
		/// 
		virtual void computeFlux(const volume::Volume& conservedVariables,
			rvec3& waveSpeed, bool computeWaveSpeed,
			volume::Volume& output
			);


		/// 
		/// \returns the number of ghost cells this specific flux requires
		///
		virtual size_t getNumberOfGhostCells();

    private:

        alsfvm::shared_ptr<reconstruction::Reconstruction> reconstruction;
        alsfvm::shared_ptr<volume::Volume> left;
        alsfvm::shared_ptr<volume::Volume> right;

        // This is for storing F(U_l, U_r) at each cell
        alsfvm::shared_ptr<volume::Volume> temporaryVolume;

        equation::euler::Euler::Parameters parameters;
    };

} // namespace alsfvm
} // namespace numflux
} // namespace euler

