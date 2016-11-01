/*
 * CPOSIXMPI.h
 *
 *  Created on: Oct 6, 2016
 *      Author: wfg
 */

#ifndef CPOSIX_H_
#define CPOSIX_H_


#include "core/CTransport.h"


namespace adios
{


class CPOSIX : public CTransport
{

public:

    CPOSIX( const unsigned int priority, const unsigned int iteration, MPI_Comm mpiComm, const bool debugMode  );

    ~CPOSIX( );

    void Open( const std::string fileName, const std::string accessMode );

    void Write( const CVariableBase& variable );

    void Close( );

};


} //end namespace






#endif /* CPOSIX_H_ */
