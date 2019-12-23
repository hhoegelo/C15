#include "testing/parameter/TestGroupSet.h"
#include "testing/parameter/TestGroup.h"
#include "testing/TestRootDocument.h"
#include "parameters/SplitPointParameter.h"
#include "testing/TestHelper.h"
#include "third-party/include/catch.hpp"

TEST_CASE("Split Point Display Value")
{
  TestRootDocument root;
  TestGroupSet set{ &root };
  TestGroup group(&set, VoiceGroup::I);
  group.addParameter(new SplitPointParameter(&group, ParameterId{ 1, VoiceGroup::Global }));

  auto parameter = dynamic_cast<SplitPointParameter *>(group.findParameterByID({ 1, VoiceGroup::Global }));
  g_assert(parameter != nullptr);

  auto transScope = UNDO::Scope::startTrashTransaction();
  auto transaction = transScope->getTransaction();

  parameter->setCPFromHwui(transaction, 0);
  REQUIRE(parameter->getDisplayValue(VoiceGroup::I) == "C1");
  REQUIRE(parameter->getDisplayValue(VoiceGroup::II) == "C#1");

  parameter->setCPFromHwui(transaction, 0.5);
  REQUIRE(parameter->getDisplayValue(VoiceGroup::I) == "F#3");
  REQUIRE(parameter->getDisplayValue(VoiceGroup::II) == "G3");

  parameter->setCPFromHwui(transaction, 1);
  REQUIRE(parameter->getDisplayValue(VoiceGroup::I) == "B5");
  REQUIRE(parameter->getDisplayValue(VoiceGroup::II) == "C6");
}