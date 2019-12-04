#include <device-settings/DeviceName.h>
#include <device-settings/Settings.h>
#include <device-settings/SSID.h>

SSID::SSID(Settings &parent)
    : super(parent, "ssid=")
{
  loadFromFile();

  parent.getSetting<DeviceName>()->onChange([=](const Setting *s) {
    static const std::string dict = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789+-_";

    std::string ssid = "NL-C15-" + s->save();
    auto l = ssid.length();

    for(char &it : ssid)
      if(dict.find(it) == std::string::npos)
        it = '_';

    set(ssid);
  });
}

SSID::~SSID() = default;

Glib::ustring SSID::getDisplayString() const
{
  return get();
}
