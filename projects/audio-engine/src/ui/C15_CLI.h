#pragma once

#include "CommandLineInterface.h"
#include <unordered_map>
#include <functional>

class C15Synth;

class C15_CLI : public CommandLineInterface
{
 public:
  C15_CLI(C15Synth *synth);

 protected:
  void processByte(char c) override;

 private:
  std::unordered_map<char, std::function<void()>> m_commands;
};
