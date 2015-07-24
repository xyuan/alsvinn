#pragma once
#include <array>
#include "alsfvm/types.hpp"
namespace alsfvm {
	namespace grid {
		///
		/// Holds the information about the grid
		/// \note We only support regular, uniform cartesian grid
		///
		class Grid {
		public:

			/// 
			/// Constructs the Grid
			/// \param origin the origin point of the grid (the smallest point in lexicographical order)
			/// \param top the top right corner of the grid (maximum point in lexicographical order)
			/// \param dimensions the dimensions of the grid (in number of cells in each direction)
			///
			Grid(rvec3 origin, rvec3 top, ivec3 dimensions);

			///
			/// Gets the origin point
			/// \returns the origin point
			///
			rvec3 getOrigin() const;

			///
			/// Gets the top point
			/// \returns the top point
			///
			rvec3 getTop() const;

			///
			/// Gets the dimensions
			/// \returns the dimensions (number of cells in each direction)
			///
			ivec3 getDimensions() const;

            ///
            /// Gets the number of active dimensions (eg. 1D, 2D, 3D)
            /// \returns the number of active space dimensions
            ///
            size_t getActiveDimension() const;

		private:
			rvec3 origin;
			rvec3 top;
			ivec3 dimensions;
		};
	}
}