
#include <vector>
#include <algorithm>
#include <libundo/undo/StringTools.h>
#include "StringTools.h"

namespace StringTools
{

  std::vector<Glib::ustring> splitStringOnAnyDelimiter(const Glib::ustring &s, char delimiter)
  {
    std::vector<Glib::ustring> strings;
    std::string token;
    std::istringstream tokenStream(s);
    while(std::getline(tokenStream, token, delimiter))
    {
      strings.emplace_back(token);
    }
    return strings;
  }

  std::vector<std::string> splitStringOnStringDelimiter(std::string s, std::string delimiter)
  {
    size_t pos = 0;
    std::vector<std::string> strings;
    std::string token;
    while((pos = s.find(delimiter)) != std::string::npos)
    {
      token = s.substr(0, pos);
      if(token.length() > 0)
        strings.push_back(token);
      s.erase(0, pos + delimiter.length());
    }
    if(s.length() > 0)
      strings.push_back(s);

    return strings;
  }

  Glib::ustring replaceAll(const Glib::ustring &in, const Glib::ustring &pattern, const Glib::ustring &replace)
  {
    Glib::ustring ret(in);
    size_t pos = 0;
    while((pos = ret.find(pattern, pos)) != Glib::ustring::npos)
    {
      ret.replace(pos, pattern.size(), replace);
      pos += replace.size();
    }
    return ret;
  }

  Glib::ustring removeSpaces(const Glib::ustring &in)
  {
    std::string ret(in);
    ret.erase(std::remove_if(ret.begin(), ret.end(), [&](char c) { return std::isspace(c); }), ret.end());
    return ret;
  }
  
  bool hasEnding(std::string const &in, std::string const &test)
  {
    if(in.length() >= test.length())
    {
      return (0 == in.compare(in.length() - test.length(), test.length(), test));
    }
    else
    {
      return false;
    }
  }
}
