#include "third-party/include/catch.hpp"
#include "tools/TextSplitter.h"
#include <glib.h>
#include <sstream>

TEST_CASE("Text Splitter concat utf8")
{
  Glib::ustring in("\u24b6\u24b7\u24b8\u24b9");
  REQUIRE(in.length() == 4);
  std::stringstream str;
  str << in;
  Glib::ustring out = str.str();
  REQUIRE(out.length() == 4);
}

TEST_CASE("Text Splitter iterate utf8")
{
  Glib::ustring in("\u24b6\u24b7\u24b8\u24b9");
  REQUIRE(in.length() == 4);
  std::stringstream str;

  for(auto it = in.begin(); it != in.end(); it++)
  {
    gchar txt[5];
    gint numBytes = g_unichar_to_utf8(*it, txt);
    str.write(txt, numBytes);
  }

  Glib::ustring out = str.str();
  REQUIRE(out.length() == 4);
}
