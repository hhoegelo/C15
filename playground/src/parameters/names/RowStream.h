#pragma once

#include <playground.h>
#include <fstream>
#include <functional>

class RowStream
{
 public:
  RowStream(const std::string &fileName);
  virtual ~RowStream();

  bool eatRow();
  void forEach(std::function<void(const std::string &)> cb);

 private:
  bool pop(std::string &line);
  bool iterateQuotes(const std::string &buffer, bool insideQuote) const;

  std::ifstream m_stream;
};
