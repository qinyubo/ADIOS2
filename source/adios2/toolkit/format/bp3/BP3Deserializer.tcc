/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP3Deserializer.tcc
 *
 *  Created on: Sep 7, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_FORMAT_BP1_BP3DESERIALIZER_TCC_
#define ADIOS2_TOOLKIT_FORMAT_BP1_BP3DESERIALIZER_TCC_

#include "BP3Deserializer.h"

#include <algorithm> //std::reverse
#include <unordered_set>

#include "adios2/helper/adiosFunctions.h"

namespace adios2
{
namespace format
{

template <class T>
std::map<std::string, helper::SubFileInfoMap>
BP3Deserializer::GetSyncVariableSubFileInfo(
    const core::Variable<T> &variable) const
{
    std::map<std::string, helper::SubFileInfoMap> variableSubFileInfo;
    variableSubFileInfo[variable.m_Name] = GetSubFileInfo(variable);
    return variableSubFileInfo;
}

template <class T>
void BP3Deserializer::GetSyncVariableDataFromStream(core::Variable<T> &variable,
                                                    BufferSTL &bufferSTL) const
{
    auto itStep =
        variable.m_IndexStepBlockStarts.find(variable.m_StepsStart + 1);

    if (itStep == variable.m_IndexStepBlockStarts.end())
    {
        variable.m_Data = nullptr;
        return;
    }

    auto &buffer = bufferSTL.m_Buffer;
    size_t position = itStep->second.front();

    const Characteristics<T> characteristics =
        ReadElementIndexCharacteristics<T>(
            buffer, position, static_cast<DataTypes>(GetDataType<T>()), false);

    const size_t payloadOffset = characteristics.Statistics.PayloadOffset;
    variable.m_Data = reinterpret_cast<T *>(&buffer[payloadOffset]);
}

template <class T>
void BP3Deserializer::GetDeferredVariable(core::Variable<T> &variable, T *data)
{
    variable.m_Data = data;
    m_DeferredVariables[variable.m_Name] = helper::SubFileInfoMap();
}

helper::SubFileInfoMap &
BP3Deserializer::GetSubFileInfoMap(const std::string &variableName)
{
    return m_DeferredVariables[variableName];
}

template <class T>
void BP3Deserializer::GetValueFromMetadata(core::Variable<T> &variable) const
{
    GetValueFromMetadataCommon(variable);
}

// PRIVATE
template <>
inline void BP3Deserializer::DefineVariableInIO<std::string>(
    const ElementIndexHeader &header, core::IO &io,
    const std::vector<char> &buffer, size_t position) const
{
    const size_t initialPosition = position;

    const Characteristics<std::string> characteristics =
        ReadElementIndexCharacteristics<std::string>(
            buffer, position, static_cast<DataTypes>(header.DataType));

    std::string variableName(header.Name);
    if (!header.Path.empty())
    {
        variableName = header.Path + PathSeparator + header.Name;
    }

    core::Variable<std::string> *variable = nullptr;
    if (characteristics.Statistics.IsValue)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        variable = &io.DefineVariable<std::string>(variableName);
        variable->m_Value =
            characteristics.Statistics.Value; // assigning first step
    }
    else
    {
        throw std::runtime_error("ERROR: variable " + variableName +
                                 " of type string can't be an array, when "
                                 "parsing metadata in call to Open");
    }

    // going back to get variable index position
    variable->m_IndexStart =
        initialPosition - (header.Name.size() + header.GroupName.size() +
                           header.Path.size() + 23);

    const size_t endPosition =
        variable->m_IndexStart + static_cast<size_t>(header.Length) + 4;

    position = initialPosition;

    size_t currentStep = 0; // Starts at 1 in bp file
    std::unordered_set<uint32_t> stepsFound;
    variable->m_AvailableStepsCount = 0;
    while (position < endPosition)
    {
        const size_t subsetPosition = position;

        // read until step is found
        const Characteristics<std::string> subsetCharacteristics =
            ReadElementIndexCharacteristics<std::string>(
                buffer, position, static_cast<DataTypes>(header.DataType),
                false);

        if (subsetCharacteristics.Statistics.Step > currentStep)
        {
            currentStep = subsetCharacteristics.Statistics.Step;
        }
        if (stepsFound.insert(subsetCharacteristics.Statistics.Step).second)
        {
            ++variable->m_AvailableStepsCount;
        }
        variable->m_IndexStepBlockStarts[currentStep].push_back(subsetPosition);
        position = subsetPosition + subsetCharacteristics.EntryLength + 5;
    }
}

template <class T>
void BP3Deserializer::DefineVariableInIO(const ElementIndexHeader &header,
                                         core::IO &io,
                                         const std::vector<char> &buffer,
                                         size_t position) const
{
    const size_t initialPosition = position;

    Characteristics<T> characteristics = ReadElementIndexCharacteristics<T>(
        buffer, position, static_cast<DataTypes>(header.DataType));

    std::string variableName(header.Name);
    if (!header.Path.empty())
    {
        variableName = header.Path + PathSeparator + header.Name;
    }

    core::Variable<T> *variable = nullptr;
    if (characteristics.Statistics.IsValue)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        variable = &io.DefineVariable<T>(variableName);
        variable->m_Value = characteristics.Statistics.Value;
        variable->m_Min = characteristics.Statistics.Value;
        variable->m_Max = characteristics.Statistics.Value;
    }
    else
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        if (m_ReverseDimensions)
        {
            std::reverse(characteristics.Shape.begin(),
                         characteristics.Shape.end());
        }

        variable = &io.DefineVariable<T>(variableName, characteristics.Shape,
                                         Dims(characteristics.Shape.size(), 0),
                                         characteristics.Shape);

        variable->m_Min = characteristics.Statistics.Min;
        variable->m_Max = characteristics.Statistics.Max;
    }

    // going back to get variable index position
    variable->m_IndexStart =
        initialPosition - (header.Name.size() + header.GroupName.size() +
                           header.Path.size() + 23);

    const size_t endPosition =
        variable->m_IndexStart + static_cast<size_t>(header.Length) + 4;

    position = initialPosition;

    size_t currentStep = 0; // Starts at 1 in bp file
    std::unordered_set<uint32_t> stepsFound;
    variable->m_AvailableStepsCount = 0;
    while (position < endPosition)
    {
        const size_t subsetPosition = position;

        // read until step is found
        const Characteristics<T> subsetCharacteristics =
            ReadElementIndexCharacteristics<T>(
                buffer, position, static_cast<DataTypes>(header.DataType),
                false);

        if (helper::LessThan(subsetCharacteristics.Statistics.Min,
                             variable->m_Min))
        {
            variable->m_Min = subsetCharacteristics.Statistics.Min;
        }

        if (helper::GreaterThan(subsetCharacteristics.Statistics.Max,
                                variable->m_Max))
        {
            variable->m_Max = subsetCharacteristics.Statistics.Max;
        }

        if (subsetCharacteristics.Statistics.Step > currentStep)
        {
            currentStep = subsetCharacteristics.Statistics.Step;
        }
        if (stepsFound.insert(subsetCharacteristics.Statistics.Step).second)
        {
            ++variable->m_AvailableStepsCount;
        }
        variable->m_IndexStepBlockStarts[currentStep].push_back(subsetPosition);
        position = subsetPosition + subsetCharacteristics.EntryLength + 5;
    }
}

template <class T>
void BP3Deserializer::DefineAttributeInIO(const ElementIndexHeader &header,
                                          core::IO &io,
                                          const std::vector<char> &buffer,
                                          size_t position) const
{
    const Characteristics<T> characteristics =
        ReadElementIndexCharacteristics<T>(
            buffer, position, static_cast<DataTypes>(header.DataType));

    std::string attributeName(header.Name);
    if (!header.Path.empty())
    {
        attributeName = header.Path + PathSeparator + header.Name;
    }

    if (characteristics.Statistics.IsValue)
    {
        io.DefineAttribute<T>(attributeName, characteristics.Statistics.Value);
    }
    else
    {
        io.DefineAttribute<T>(attributeName,
                              characteristics.Statistics.Values.data(),
                              characteristics.Statistics.Values.size());
    }
}

template <class T>
helper::SubFileInfoMap
BP3Deserializer::GetSubFileInfo(const core::Variable<T> &variable) const
{
    helper::SubFileInfoMap infoMap;

    const auto &buffer = m_Metadata.m_Buffer;

    const size_t stepStart = variable.m_StepsStart + 1;
    const size_t stepEnd = stepStart + variable.m_StepsCount; // exclusive

    const Box<Dims> selectionBox = helper::StartEndBox(
        variable.m_Start, variable.m_Count, m_ReverseDimensions);

    for (size_t step = stepStart; step < stepEnd; ++step)
    {
        auto itBlockStarts = variable.m_IndexStepBlockStarts.find(step);
        if (itBlockStarts == variable.m_IndexStepBlockStarts.end())
        {
            continue;
        }

        const std::vector<size_t> &blockStarts = itBlockStarts->second;

        // blockPosition gets updated by Read, can't be const
        for (size_t blockPosition : blockStarts)
        {
            const Characteristics<T> blockCharacteristics =
                ReadElementIndexCharacteristics<T>(
                    buffer, blockPosition,
                    static_cast<DataTypes>(GetDataType<T>()));

            // check if they intersect
            helper::SubFileInfo info;
            info.BlockBox = helper::StartEndBox(blockCharacteristics.Start,
                                                blockCharacteristics.Count);
            info.IntersectionBox =
                helper::IntersectionBox(selectionBox, info.BlockBox);

            if (info.IntersectionBox.first.empty() ||
                info.IntersectionBox.second.empty())
            {
                continue;
            }
            // if they intersect get info Seeks (first: start, second:
            // count)
            info.Seeks.first =
                blockCharacteristics.Statistics.PayloadOffset +
                helper::LinearIndex(info.BlockBox, info.IntersectionBox.first,
                                    m_IsRowMajor) *
                    sizeof(T);

            info.Seeks.second =
                blockCharacteristics.Statistics.PayloadOffset +
                (helper::LinearIndex(info.BlockBox, info.IntersectionBox.second,
                                     m_IsRowMajor) +
                 1) *
                    sizeof(T);

            const size_t fileIndex =
                static_cast<size_t>(blockCharacteristics.Statistics.FileIndex);

            infoMap[fileIndex][step].push_back(std::move(info));
        }
    }

    return infoMap;
}

template <class T>
void BP3Deserializer::ClipContiguousMemoryCommon(
    core::Variable<T> &variable, const std::vector<char> &contiguousMemory,
    const Box<Dims> &blockBox, const Box<Dims> &intersectionBox) const
{
    const Dims &start = intersectionBox.first;
    if (start.size() == 1) // 1D copy memory
    {
        // normalize intersection start with variable.m_Start
        const size_t normalizedStart =
            (start[0] - variable.m_Start[0]) * sizeof(T);
        char *rawVariableData = reinterpret_cast<char *>(variable.m_Data);

        std::copy(contiguousMemory.begin(), contiguousMemory.end(),
                  &rawVariableData[normalizedStart]);

        return;
    }

    if (m_IsRowMajor) // stored with C, C++, Python
    {
        ClipContiguousMemoryCommonRow(variable, contiguousMemory, blockBox,
                                      intersectionBox);
    }
    else // stored with Fortran, R
    {
        ClipContiguousMemoryCommonColumn(variable, contiguousMemory, blockBox,
                                         intersectionBox);
    }
}

template <class T>
void BP3Deserializer::ClipContiguousMemoryCommonRow(
    core::Variable<T> &variable, const std::vector<char> &contiguousMemory,
    const Box<Dims> &blockBox, const Box<Dims> &intersectionBox) const
{
    const Dims &start = intersectionBox.first;
    const Dims &end = intersectionBox.second;
    const size_t stride = (end.back() - start.back() + 1) * sizeof(T);

    Dims currentPoint(start); // current point for memory copy

    const Box<Dims> selectionBox = helper::StartEndBox(
        variable.m_Start, variable.m_Count, m_ReverseDimensions);

    const size_t dimensions = start.size();
    bool run = true;

    const size_t intersectionStart =
        helper::LinearIndex(blockBox, intersectionBox.first, true) * sizeof(T);

    while (run)
    {
        // here copy current linear memory between currentPoint and end
        const size_t contiguousStart =
            helper::LinearIndex(blockBox, currentPoint, true) * sizeof(T) -
            intersectionStart;

        const size_t variableStart =
            helper::LinearIndex(selectionBox, currentPoint, true) * sizeof(T);

        char *rawVariableData = reinterpret_cast<char *>(variable.m_Data);

        std::copy(contiguousMemory.begin() + contiguousStart,
                  contiguousMemory.begin() + contiguousStart + stride,
                  rawVariableData + variableStart);

        // here update each index recursively, always starting from the 2nd
        // fastest changing index, since fastest changing index is the
        // continuous part in the previous std::copy
        size_t p = dimensions - 2;
        while (true)
        {
            ++currentPoint[p];
            if (currentPoint[p] > end[p])
            {
                if (p == 0)
                {
                    run = false; // we are done
                    break;
                }
                else
                {
                    currentPoint[p] = start[p];
                    --p;
                }
            }
            else
            {
                break; // break inner p loop
            }
        } // dimension index update
    }     // run
}

template <class T>
void BP3Deserializer::ClipContiguousMemoryCommonColumn(
    core::Variable<T> &variable, const std::vector<char> &contiguousMemory,
    const Box<Dims> &blockBox, const Box<Dims> &intersectionBox) const
{
    const Dims &start = intersectionBox.first;
    const Dims &end = intersectionBox.second;
    const size_t stride = (end.front() - start.front() + 1) * sizeof(T);

    Dims currentPoint(start); // current point for memory copy

    const Box<Dims> selectionBox = helper::StartEndBox(
        variable.m_Start, variable.m_Count, m_ReverseDimensions);

    const size_t dimensions = start.size();
    bool run = true;

    const size_t intersectionStart =
        helper::LinearIndex(blockBox, intersectionBox.first, false) * sizeof(T);

    while (run)
    {

        // here copy current linear memory between currentPoint and end
        const size_t contiguousStart =
            helper::LinearIndex(blockBox, currentPoint, false) * sizeof(T) -
            intersectionStart;

        const size_t variableStart =
            helper::LinearIndex(selectionBox, currentPoint, false) * sizeof(T);

        char *rawVariableData = reinterpret_cast<char *>(variable.m_Data);

        std::copy(contiguousMemory.begin() + contiguousStart,
                  contiguousMemory.begin() + contiguousStart + stride,
                  rawVariableData + variableStart);

        // here update each index recursively, always starting from the 2nd
        // fastest changing index, since fastest changing index is the
        // continuous part in the previous std::copy
        size_t p = 1;
        while (true)
        {
            ++currentPoint[p];
            if (currentPoint[p] > end[p])
            {
                if (p == dimensions - 1)
                {
                    run = false; // we are done
                    break;
                }
                currentPoint[p] = start[p];
                ++p;
            }
            else
            {
                break; // break inner p loop
            }
        } // dimension index update
    }
}

template <class T>
void BP3Deserializer::SetVariableNextStepDataCommon(
    core::Variable<T> &variable) const
{
    variable.m_Data += helper::GetTotalSize(variable.m_Count);
}

template <>
inline void BP3Deserializer::GetValueFromMetadataCommon<std::string>(
    core::Variable<std::string> &variable) const
{
    std::string *data = variable.m_Data;
    const auto &buffer = m_Metadata.m_Buffer;

    for (size_t i = 0; i < variable.m_StepsCount; ++i)
    {
        *(data + i) = "";
        const size_t step = variable.m_StepsStart + i + 1;
        auto itStep = variable.m_IndexStepBlockStarts.find(step);

        if (itStep == variable.m_IndexStepBlockStarts.end())
        {
            continue;
        }

        for (const size_t position : itStep->second)
        {
            size_t localPosition = position;
            const Characteristics<std::string> characteristics =
                ReadElementIndexCharacteristics<std::string>(
                    buffer, localPosition, type_string, false);

            *(data + i) = characteristics.Statistics.Value;
        }

        variable.m_Value = *(data + i);
    }
}

template <class T>
inline void
BP3Deserializer::GetValueFromMetadataCommon(core::Variable<T> &variable) const
{
    T *data = variable.m_Data;
    const auto &buffer = m_Metadata.m_Buffer;

    const size_t step = variable.m_StepsStart + 1;
    auto itStep = variable.m_IndexStepBlockStarts.find(step);

    if (itStep == variable.m_IndexStepBlockStarts.end())
    {
        data = nullptr;
        return;
    }

    for (const size_t position : itStep->second)
    {
        size_t localPosition = position;

        const Characteristics<T> characteristics =
            ReadElementIndexCharacteristics<T>(
                buffer, localPosition, static_cast<DataTypes>(GetDataType<T>()),
                false);

        *data = characteristics.Statistics.Value;
    }
}

} // end namespace format
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_FORMAT_BP1_BP3DESERIALIZER_TCC_ */
