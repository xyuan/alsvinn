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
#include "alsutils/error/Exception.hpp"
#include <hdf5.h>
///
/// This file contains various utility functions for HDF5
///


#define HDF5_SAFE_CALL(x) {\
    if (x < 0) { \
        THROW("HDF5 error, call looked like: " << #x); \
    } \
}

//! Convience macro. Runs expression, test the return value
//! Throws an exception if return value is negative
#define HDF5_MAKE_RESOURCE(holder, expression, closer) { \
    auto hidValue = expression; \
    if (hidValue < 0) { \
        THROW("HDF5 error in running\n\t" << #expression \
              <<"\n\n" << __FILE__ << ": " << __LINE__); \
    } \
    else { \
        holder.reset(new HDF5Resource(hidValue, closer)); \
    } \
}

namespace alsfvm {
namespace io {



///
/// \brief The HDF5Resource class is a unique_ptr for hdf5 resources
///
class HDF5Resource {
public:
    typedef herr_t (*delete_function)(hid_t);

    ///
    /// \brief HDF5Resource constructs a new HDF5Resource
    ///
    /// Example usage
    /// \code{.cpp}
    /// HDF5Resource file(H5fopen("file.h5")), H5Fclose);
    /// // do something with file.hid()
    /// // deletes when file goes out of scope automatically
    /// \endcode
    ///
    /// \param hdf5Resource the hdf5 id to store (obtained from say H5Fopen
    /// \param deleter the deleter function (eg. H5Fclose)
    ///
    ///
    inline HDF5Resource(hid_t hdf5Resource, delete_function deleter)
        : hdf5Resource(hdf5Resource), deleter(deleter) {
        // empty
    }

    inline ~HDF5Resource() noexcept(false) {
        HDF5_SAFE_CALL(deleter(hdf5Resource));
    }

    inline hid_t hid() {
        return hdf5Resource;
    }



private:
    // We do not want to be able to copy this
    HDF5Resource(const HDF5Resource& other) : hdf5Resource(0), deleter(NULL) {}
    void operator=(const HDF5Resource& other) {}
    const hid_t hdf5Resource;
    const delete_function deleter;

};
}
}
