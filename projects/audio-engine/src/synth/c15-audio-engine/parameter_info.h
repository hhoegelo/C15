#pragma once

/******************************************************************************/
/** @file       parameter_info.h
    @date
    @version    1.7-0
    @author     M. Seeber
    @brief      combining all necessary parameter-related details
    @todo
*******************************************************************************/

#include "parameter_declarations.h"

namespace C15
{

namespace Properties
{

enum class LayerId
{
    I, II,
    _LENGTH_
};

enum class LayerMode
{
    Single, Split, Layer
};

// naming every HW Source return behavior
enum class HW_Return_Behavior
{
    Stay, Zero, Center
};

} // namespace C15::Properties

} // namespace C15
