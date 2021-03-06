/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Variable.cpp :
 *
 *  Created on: Jun 4, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */
#include "Variable.h"

#include "adios2/ADIOSMacros.h"
#include "adios2/core/Variable.h"
#include "adios2/helper/adiosFunctions.h" //CheckNullptr

namespace adios2
{

#define declare_type(T)                                                        \
                                                                               \
    template <>                                                                \
    Variable<T>::Variable(core::Variable<T> *variable) : m_Variable(variable)  \
    {                                                                          \
    }                                                                          \
                                                                               \
    template <>                                                                \
    Variable<T>::operator bool() const noexcept                                \
    {                                                                          \
        return (m_Variable == nullptr) ? false : true;                         \
    }                                                                          \
                                                                               \
    template <>                                                                \
    void Variable<T>::SetSelection(const Box<Dims> &selection)                 \
    {                                                                          \
        helper::CheckForNullptr(m_Variable,                                    \
                                "in call to Variable<T>::SetSelection");       \
        m_Variable->SetSelection(selection);                                   \
    }                                                                          \
                                                                               \
    template <>                                                                \
    void Variable<T>::SetStepSelection(const Box<size_t> &stepSelection)       \
    {                                                                          \
        helper::CheckForNullptr(m_Variable,                                    \
                                "in call to Variable<T>::SetStepSelection");   \
        m_Variable->SetStepSelection(stepSelection);                           \
    }                                                                          \
                                                                               \
    template <>                                                                \
    size_t Variable<T>::SelectionSize() const                                  \
    {                                                                          \
        helper::CheckForNullptr(m_Variable,                                    \
                                "in call to Variable<T>::SelectionSize");      \
        return m_Variable->SelectionSize();                                    \
    }                                                                          \
                                                                               \
    template <>                                                                \
    const std::string &Variable<T>::Name() const                               \
    {                                                                          \
        helper::CheckForNullptr(m_Variable, "in call to Variable<T>::Name");   \
        return m_Variable->m_Name;                                             \
    }                                                                          \
                                                                               \
    template <>                                                                \
    std::string Variable<T>::Type() const                                      \
    {                                                                          \
        helper::CheckForNullptr(m_Variable, "in call to Variable<T>::Type");   \
        return m_Variable->m_Type;                                             \
    }                                                                          \
    template <>                                                                \
    size_t Variable<T>::Sizeof() const                                         \
    {                                                                          \
        helper::CheckForNullptr(m_Variable, "in call to Variable<T>::Sizeof"); \
        return m_Variable->m_ElementSize;                                      \
    }                                                                          \
                                                                               \
    template <>                                                                \
    adios2::ShapeID Variable<T>::ShapeID() const                               \
    {                                                                          \
        helper::CheckForNullptr(m_Variable,                                    \
                                "in call to Variable<T>::ShapeID");            \
        return m_Variable->m_ShapeID;                                          \
    }                                                                          \
                                                                               \
    template <>                                                                \
    const Dims &Variable<T>::Shape() const                                     \
    {                                                                          \
        helper::CheckForNullptr(m_Variable, "in call to Variable<T>::Shape");  \
        return m_Variable->m_Shape;                                            \
    }                                                                          \
                                                                               \
    template <>                                                                \
    const Dims &Variable<T>::Start() const                                     \
    {                                                                          \
        helper::CheckForNullptr(m_Variable, "in call to Variable<T>::Start");  \
        return m_Variable->m_Start;                                            \
    }                                                                          \
                                                                               \
    template <>                                                                \
    const Dims &Variable<T>::Count() const                                     \
    {                                                                          \
        helper::CheckForNullptr(m_Variable, "in call to Variable<T>::Count");  \
        return m_Variable->m_Count;                                            \
    }                                                                          \
                                                                               \
    template <>                                                                \
    size_t Variable<T>::Steps() const                                          \
    {                                                                          \
        helper::CheckForNullptr(m_Variable, "in call to Variable<T>::Steps");  \
        return m_Variable->m_AvailableStepsCount;                              \
    }                                                                          \
                                                                               \
    template <>                                                                \
    size_t Variable<T>::StepsStart() const                                     \
    {                                                                          \
        helper::CheckForNullptr(m_Variable,                                    \
                                "in call to Variable<T>::StepsStart");         \
        return m_Variable->m_AvailableStepsStart;                              \
    }                                                                          \
                                                                               \
    template <>                                                                \
    unsigned int Variable<T>::AddOperator(Operator &op,                        \
                                          const Params &parameters)            \
    {                                                                          \
        helper::CheckForNullptr(m_Variable,                                    \
                                "in call to Variable<T>::AddOperator");        \
        if (!op)                                                               \
        {                                                                      \
            throw std::invalid_argument("ERROR: invalid operator, in call to " \
                                        "Variable<T>::AddOperator");           \
        }                                                                      \
        return m_Variable->AddOperator(*op.m_Operator, parameters);            \
    }                                                                          \
                                                                               \
    template <>                                                                \
    std::vector<typename Variable<T>::OperatorInfo>                            \
    Variable<T>::OperatorsInfo() const noexcept                                \
    {                                                                          \
        helper::CheckForNullptr(m_Variable,                                    \
                                "in call to Variable<T>::OperatorsInfo");      \
        std::vector<OperatorInfo> operatorsInfo;                               \
        operatorsInfo.reserve(m_Variable->m_OperatorsInfo.size());             \
                                                                               \
        for (auto &opInfo : m_Variable->m_OperatorsInfo)                       \
        {                                                                      \
            operatorsInfo.push_back(                                           \
                OperatorInfo{Operator(&opInfo.Op), opInfo.Parameters});        \
        }                                                                      \
        return operatorsInfo;                                                  \
    }

ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

} // end namespace adios2
