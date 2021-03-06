/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * cxx98ADIOS.h : C++98 bindings based on C API, eliminates all C++11
 * features in public headers
 *
 *  Created on: Apr 4, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_BINDINGS_CXX98_CXX98_CXX98ADIOS_H_
#define ADIOS2_BINDINGS_CXX98_CXX98_CXX98ADIOS_H_

#include <string>

#ifdef ADIOS2_HAVE_MPI
#include <mpi.h>
#endif

#include "cxx98IO.h"

#include "adios2/ADIOSConfig.h"

struct adios2_adios;

namespace adios2
{
namespace cxx98
{

class ADIOS
{

public:
#ifdef ADIOS2_HAVE_MPI
    ADIOS(const std::string &configFile, MPI_Comm mpiComm,
          const bool debugMode = true);

    ADIOS(MPI_Comm mpiComm, const bool debugMode = true);

    ADIOS(const std::string &configFile, const bool debugMode = true);

    ADIOS(const bool debugMode = true);
#else
    ADIOS(const std::string &configFile, const bool debugMode = true);

    ADIOS(const bool debugMode = true);
#endif

    ADIOS(const ADIOS &adios);

    ~ADIOS();

    operator bool() const;

    /**
     * Declares a new IO class object and returns a reference to that
     * object.
     * @param ioName must be unique
     * @return reference to newly created IO object inside current ADIOS
     * object
     * @exception std::invalid_argument if IO with unique name is already
     * declared, in debug mode only
     */
    IO DeclareIO(const std::string &name);

    /**
     * Retrieve a reference to an existing IO object created with DeclareIO.
     * @param name of IO to look for
     * @return if IO exists returns a reference to existing IO object inside
     * ADIOS
     * @exception std::invalid_argument if IO was not created with
     * DeclareIO, in debug mode only
     */
    IO AtIO(const std::string &name);

    /**
     * Flushes all engines in all IOs created with the current ADIOS object
     * using DeclareIO and IO.Open.
     * If no IO or engine is created it does nothing.
     * @exception std::runtime_error if any engine Flush fails
     */
    void FlushAll();

private:
    /** pointer to C adios2_adios */
    adios2_adios *m_ADIOS;
};

} // end namespace cxx03
} // end namespace adios2

#endif /* ADIOS2_BINDINGS_CXX98_CXX98_CXX98ADIOS_H_ */
