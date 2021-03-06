/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * it->first ==ompanying file Copyright.txt for details.
 *
 * CompressSZ.cpp
 *
 *  Created on: Jul 24, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "CompressSZ.h"

#include <cmath> //std::ceil
#include <ios>   //std::ios_base::failure
#include <iostream>
#include <stdexcept> //std::invalid_argument
/// \endcond

#include "adios2/helper/adiosFunctions.h"

namespace adios2
{
namespace core
{
namespace compress
{

CompressSZ::CompressSZ(const Params &parameters, const bool debugMode)
: Operator("SZ", parameters, debugMode)
{
}

size_t CompressSZ::BufferMaxSize(const size_t sizeIn) const
{
    return static_cast<size_t>(std::ceil(1.1 * sizeIn) + 600);
}

size_t CompressSZ::Compress(const void *dataIn, const Dims &dimensions,
                            const size_t elementSize, const std::string varType,
                            void *bufferOut, const Params &parameters) const
{
    int ndims = dimensions.size();
    if (ndims > 5)
    {
        throw std::invalid_argument("No more than 5 dimension is supported.\n");
    }

    sz_params sz;
    memset(&sz, 0, sizeof(sz_params));
    sz.max_quant_intervals = 65536;
    sz.quantization_intervals = 0;
    sz.dataEndianType = LITTLE_ENDIAN_DATA;
    sz.sysEndianType = LITTLE_ENDIAN_DATA;
    sz.sol_ID = SZ;
    sz.layers = 1;
    sz.sampleDistance = 100;
    sz.predThreshold = 0.99;
    sz.offset = 0;
    sz.szMode = SZ_BEST_COMPRESSION; // SZ_BEST_SPEED; //SZ_BEST_COMPRESSION;
    sz.gzipMode = 1;
    sz.errorBoundMode = ABS;
    sz.absErrBound = 1E-4;
    sz.relBoundRatio = 1E-3;
    sz.psnr = 80.0;
    sz.pw_relBoundRatio = 1E-5;
    sz.segment_size = (int)pow(5, (double)ndims);
    sz.pwr_type = SZ_PWR_MIN_TYPE;

    size_t outsize;
    size_t r[5] = {0, 0, 0, 0, 0};

    /* SZ parameters */
    int use_configfile = 0;
    int use_zchecker = 0;
    std::string sz_configfile = "sz.config";
    // std::string zc_configfile = "zc.config";

    std::cout << "debugMode:" << this->m_DebugMode << std::endl;

    Params::const_iterator it;
    for (it = parameters.begin(); it != parameters.end(); it++)
    {
        std::cout << it->first << " => " << it->second << '\n';
        if (it->first == "init")
        {
            use_configfile = 1;
            sz_configfile = std::string(it->second);
        }
        else if (it->first == "max_quant_intervals")
        {
            sz.max_quant_intervals = std::stoi(it->second);
        }
        else if (it->first == "quantization_intervals")
        {
            sz.quantization_intervals = std::stoi(it->second);
        }
        else if (it->first == "dataEndianType")
        {
            sz.dataEndianType = std::stoi(it->second);
        }
        else if (it->first == "sysEndianType")
        {
            sz.sysEndianType = std::stoi(it->second);
        }
        else if (it->first == "sol_ID")
        {
            sz.sol_ID = std::stoi(it->second);
        }
        else if (it->first == "layers")
        {
            sz.layers = std::stoi(it->second);
        }
        else if (it->first == "sampleDistance")
        {
            sz.sampleDistance = std::stoi(it->second);
        }
        else if (it->first == "predThreshold")
        {
            sz.predThreshold = std::stof(it->second);
        }
        else if (it->first == "offset")
        {
            sz.offset = std::stoi(it->second);
        }
        else if (it->first == "szMode")
        {
            int szMode = SZ_BEST_SPEED;
            if (it->second == "SZ_BEST_SPEED")
            {
                szMode = SZ_BEST_SPEED;
            }
            else if (it->second == "SZ_BEST_COMPRESSION")
            {
                szMode = SZ_BEST_COMPRESSION;
            }
            else if (it->second == "SZ_DEFAULT_COMPRESSION")
            {
                szMode = SZ_DEFAULT_COMPRESSION;
            }
            else
            {
                std::cout << "[WARN] An unknown szMode: " << it->second
                          << std::endl;
            }
            sz.szMode = szMode;
        }
        else if (it->first == "gzipMode")
        {
            sz.gzipMode = std::stoi(it->second);
        }
        else if (it->first == "errorBoundMode")
        {
            int errorBoundMode = ABS;
            if (it->second == "ABS")
            {
                errorBoundMode = ABS;
            }
            else if (it->second == "REL")
            {
                errorBoundMode = REL;
            }
            else if (it->second == "ABS_AND_REL")
            {
                errorBoundMode = ABS_AND_REL;
            }
            else if (it->second == "ABS_OR_REL")
            {
                errorBoundMode = ABS_OR_REL;
            }
            else if (it->second == "PW_REL")
            {
                errorBoundMode = PW_REL;
            }
            else
            {
                std::cout << "[WARN] An unknown errorBoundMode: " << it->second
                          << std::endl;
            }
            sz.errorBoundMode = errorBoundMode;
        }
        else if (it->first == "absErrBound")
        {
            sz.absErrBound = std::stof(it->second);
        }
        else if (it->first == "relBoundRatio")
        {
            sz.relBoundRatio = std::stof(it->second);
        }
        else if (it->first == "pw_relBoundRatio")
        {
            sz.pw_relBoundRatio = std::stof(it->second);
        }
        else if (it->first == "segment_size")
        {
            sz.segment_size = std::stoi(it->second);
        }
        else if (it->first == "pwr_type")
        {
            int pwr_type = SZ_PWR_MIN_TYPE;
            if ((it->first == "MIN") || (it->first == "SZ_PWR_MIN_TYPE"))
            {
                pwr_type = SZ_PWR_MIN_TYPE;
            }
            else if ((it->first == "AVG") || (it->first == "SZ_PWR_AVG_TYPE"))
            {
                pwr_type = SZ_PWR_AVG_TYPE;
            }
            else if ((it->first == "MAX") || (it->first == "SZ_PWR_MAX_TYPE"))
            {
                pwr_type = SZ_PWR_MAX_TYPE;
            }
            else
            {
                std::cout << "[WARN] An unknown pwr_type: " << it->second
                          << std::endl;
            }
            sz.pwr_type = pwr_type;
        }
        else if ((it->first == "abs") || (it->first == "absolute") ||
                 (it->first == "accuracy"))
        {
            sz.errorBoundMode = ABS;
            sz.absErrBound = std::stof(it->second);
        }
        else if ((it->first == "rel") || (it->first == "relative"))
        {
            sz.errorBoundMode = REL;
            sz.relBoundRatio = std::stof(it->second);
        }
        else if ((it->first == "pw") || (it->first == "pwr") ||
                 (it->first == "pwrel") || (it->first == "pwrelative"))
        {
            sz.errorBoundMode = PW_REL;
            sz.pw_relBoundRatio = std::stof(it->second);
        }
        else if ((it->first == "zchecker") || (it->first == "zcheck") ||
                 (it->first == "z-checker") || (it->first == "z-check"))
        {
            use_zchecker = (it->second == "") ? 1 : std::stof(it->second);
        }
        else
        {
            std::cout << "[WARN] An unknown SZ parameter: " << it->first
                      << std::endl;
        }
    }

    if (use_configfile)
    {
        std::cout << "SZ config:" << sz_configfile << std::endl;
        SZ_Init((char *)sz_configfile.c_str());
    }
    else
    {
        if (m_DebugMode)
        {
            std::cout << "sz.max_quant_intervals: " << sz.max_quant_intervals
                      << std::endl;
            std::cout << "sz.quantization_intervals: "
                      << sz.quantization_intervals << std::endl;
            std::cout << "sz.dataEndianType: " << sz.dataEndianType
                      << std::endl;
            std::cout << "sz.sysEndianType: " << sz.sysEndianType << std::endl;
            std::cout << "sz.sol_ID: " << sz.sol_ID << std::endl;
            std::cout << "sz.layers: " << sz.layers << std::endl;
            std::cout << "sz.sampleDistance: " << sz.sampleDistance
                      << std::endl;
            std::cout << "sz.predThreshold: " << sz.predThreshold << std::endl;
            std::cout << "sz.offset: " << sz.offset << std::endl;
            std::cout << "sz.szMode: " << sz.szMode << std::endl;
            std::cout << "sz.gzipMode: " << sz.gzipMode << std::endl;
            std::cout << "sz.errorBoundMode: " << sz.errorBoundMode
                      << std::endl;
            std::cout << "sz.absErrBound: " << sz.absErrBound << std::endl;
            std::cout << "sz.relBoundRatio: " << sz.relBoundRatio << std::endl;
            std::cout << "sz.psnr: " << sz.psnr << std::endl;
            std::cout << "sz.pw_relBoundRatio: " << sz.pw_relBoundRatio
                      << std::endl;
            std::cout << "sz.segment_size: " << sz.segment_size << std::endl;
            std::cout << "sz.pwr_type: " << sz.pwr_type << std::endl;
        }
        SZ_Init_Params(&sz);
    }

    // Get type info
    int dtype;
    if (varType == "double")
    {
        dtype = SZ_DOUBLE;
    }
    else if (varType == "float")
    {
        dtype = SZ_FLOAT;
    }
    else
    {
        throw std::invalid_argument("No supported data type\n");
    }

    // r[0] is the fastest changing dimension and r[4] is the lowest changing
    // dimension
    // In C, r[0] is the last dimension. In Fortran, r[0] is the first dimension
    for (int i = 0; i < ndims; i++)
    {
        uint dsize = dimensions[i];
        r[ndims - i - 1] = dsize;
        /*
        if (fd->group->adios_host_language_fortran == adios_flag_yes)
            r[i] = dsize;
        else
            r[ndims-i-1] = dsize;
        d = d->next;
         */
    }

    unsigned char *bytes;
    bytes = SZ_compress(dtype, (void *)dataIn, &outsize, r[4], r[3], r[2], r[1],
                        r[0]);
    memcpy(bufferOut, bytes, outsize);
    return static_cast<size_t>(outsize);
}

size_t CompressSZ::Decompress(const void *bufferIn, const size_t sizeIn,
                              void *dataOut, const Dims &dimensions,
                              const std::string varType,
                              const Params &parameters) const
{
    int ndims = dimensions.size();
    if (ndims > 5)
    {
        throw std::invalid_argument("No more than 5 dimension is supported.\n");
    }

    // Get type info
    int dtype;
    size_t typeSizeBytes;
    if (varType == "double")
    {
        dtype = SZ_DOUBLE;
        typeSizeBytes = 8;
    }
    else if (varType == "float")
    {
        dtype = SZ_FLOAT;
        typeSizeBytes = 4;
    }
    else
    {
        throw std::invalid_argument("No supported data type\n");
    }

    // r[0] is the fastest changing dimension and r[4] is the lowest changing
    // dimension
    // In C, r[0] is the last dimension. In Fortran, r[0] is the first dimension
    size_t r[5] = {0, 0, 0, 0, 0};
    for (int i = 0; i < ndims; i++)
    {
        uint dsize = dimensions[i];
        r[ndims - i - 1] = dsize;
        /*
         if (fd->group->adios_host_language_fortran == adios_flag_yes)
         r[i] = dsize;
         else
         r[ndims-i-1] = dsize;
         d = d->next;
         */
    }

    size_t dataSizeBytes = GetTotalSize(dimensions) * typeSizeBytes;

    void *orig_buff;
    orig_buff = SZ_decompress(dtype, (unsigned char *)bufferIn, sizeIn, r[4],
                              r[3], r[2], r[1], r[0]);
    memcpy(dataOut, orig_buff, dataSizeBytes);

    return static_cast<size_t>(dataSizeBytes);
}

} // end namespace compress
} // end namespace core
} // end namespace adios2
