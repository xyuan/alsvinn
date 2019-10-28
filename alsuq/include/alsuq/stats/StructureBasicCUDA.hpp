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

#include "alsuq/stats/StatisticsHelper.hpp"
#include "alsuq/types.hpp"
#include <thrust/device_vector.h>

namespace alsuq {
namespace stats {

//! Computes the sturcture function given a direction
//!
//! Ie in parameters it gets an in direction that corresponds to the unit
//! direction vectors \f$e\f$
//!
//! It then computes the structure function as
//!
//! \f[\sum_{i,j,k} |u_{(i,j,k) + e}-u_{(i,j,k)}|^p\f]
class StructureBasicCUDA : public StatisticsHelper {
public:
    StructureBasicCUDA(const StatisticsParameters& parameters);


    //! Returns a list of ['structure_basic']
    virtual std::vector<std::string> getStatisticsNames() const override;




    virtual void computeStatistics(const alsfvm::volume::Volume& conservedVariables,
        const alsfvm::grid::Grid& grid,
        const alsfvm::simulator::TimestepInformation& timestepInformation) override;

    virtual void finalizeStatistics() override;



private:
    template<alsfvm::boundary::Type BoundaryType>
    void computeStructure(alsfvm::volume::Volume& outputVolume,
        const alsfvm::volume::Volume& input);

    const size_t direction;
    const real p;
    const ivec3 directionVector;
    const size_t numberOfH;
    const std::string statisticsName;

    // For now we use thurst's version of reduce, and to make everything
    // play nice, we use thrust device vector to hold the temporary results.
    thrust::device_vector<real> structureOutput;

};
} // namespace stats
} // namespace alsuq
