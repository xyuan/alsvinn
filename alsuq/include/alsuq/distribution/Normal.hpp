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
#include "alsuq/distribution/Distribution.hpp"
#include "alsuq/distribution/Parameters.hpp"
namespace alsuq {
namespace distribution {

class Normal : public Distribution {
public:
    Normal(const Parameters& parameters);

    real generate(generator::Generator& generator, size_t component,
        size_t sample) override;

private:
    real scale(real x);
    const real mean;
    const real standardDeviation;

    real buffer{42};
    bool hasBuffer{false};

};
} // namespace distribution
} // namespace alsuq
