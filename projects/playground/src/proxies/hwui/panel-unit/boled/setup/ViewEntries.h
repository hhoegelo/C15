#pragma once

#include <proxies/hwui/controls/ControlWithChildren.h>

class ViewEntries : public ControlWithChildren
{
 public:
  ViewEntries(const Rect &pos);
  ~ViewEntries();

  void addEntry(Control *child, bool focussed);
  void finish();

 private:
  typedef std::pair<Control *, bool> tEntry;
  typedef std::list<tEntry> tEntries;

  void assignDownwards(tEntries::iterator entryIt, int idx);

  tEntries m_entries;
};
