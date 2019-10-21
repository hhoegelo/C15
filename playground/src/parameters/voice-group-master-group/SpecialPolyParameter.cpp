#include <proxies/hwui/panel-unit/boled/parameter-screens/DualSpecialParameterScreen.h>
#include "SpecialPolyParameter.h"

SpecialPolyParameter::SpecialPolyParameter(ParameterGroup *group, uint16_t id, const ScaleConverter *scaling,
                                               tControlPositionValue def, tControlPositionValue coarseDenominator,
                                               tControlPositionValue fineDenominator, const std::string &shortName,
                                               const std::string &longName, VoiceGroup vg)
    : Parameter(group, id, scaling, def, coarseDenominator, fineDenominator)
    , m_longname{ longName }
    , m_shortname{ shortName }
    , m_vg{vg}
{
}

ustring SpecialPolyParameter::getLongName() const
{
  return m_longname;
}

ustring SpecialPolyParameter::getShortName() const
{
  return m_shortname;
}

ustring SpecialPolyParameter::getGroupAndParameterName() const
{
  return getParentGroup()->getShortName() + getLongName();
}

DFBLayout *SpecialPolyParameter::createLayout(FocusAndMode focusAndMode) const
{
  return new DualSpecialParameterScreen();
}