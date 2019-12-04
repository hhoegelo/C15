#pragma once

#include <giomm/datainputstream.h>
#include "InStream.h"

class FileInStream : public InStream
{
 public:
  FileInStream(const Glib::ustring &fileName, bool tryZip);
  virtual ~FileInStream();

  Glib::ustring read() override;
  std::vector<uint8_t> readAll() override;
  bool eof() const;

 private:
  GDataInputStream *m_stream = nullptr;
  bool m_eof = false;
};
