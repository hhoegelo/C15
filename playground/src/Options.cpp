#include "Options.h"
#include <glibmm/optiongroup.h>
#include "device-settings/DebugLevel.h"

Options::Options(int &argc, char **&argv)
    : m_selfPath(argv[0])
{
  setDefaults();

  OptionGroup mainGroup("common", "common options");
  OptionContext ctx;

  OptionEntry pmPath;
  pmPath.set_flags(OptionEntry::FLAG_FILENAME);
  pmPath.set_long_name("pm-path");
  pmPath.set_short_name('p');
  pmPath.set_description("Name of the folder that stores preset-managers banks as XML files");
  mainGroup.add_entry_filename(pmPath, sigc::mem_fun(this, &Options::setPMPathName));

  OptionEntry bbbb;
  bbbb.set_long_name("bbbb-host");
  bbbb.set_short_name('b');
  bbbb.set_description("Where to find the bbbb");
  mainGroup.add_entry(bbbb, m_bbbb);

  OptionEntry ae;
  ae.set_long_name("audio-engine-host");
  ae.set_short_name('a');
  ae.set_description("Where to find the audio-engine");
  mainGroup.add_entry(ae, m_audioEngineHost);

  ctx.set_main_group(mainGroup);
  ctx.set_help_enabled(true);

  ctx.parse(argc, argv);
}

void Options::setDefaults()
{
  ustring prefered = "/internalstorage/preset-manager/";

  auto file = Gio::File::create_for_path(prefered);

  if(file->query_exists() || makePresetManagerDirectory(file))
  {
    m_pmPath = prefered;
  }
  else
  {
    m_pmPath = "./preset-manager/";
  }

  m_settingsFile = "./settings.xml";
  m_kioskModeFile = "./kiosk-mode.stamp";
}

bool Options::makePresetManagerDirectory(Glib::RefPtr<Gio::File> file)
{
  try
  {
    return file->make_directory();
  }
  catch(...)
  {
  }
  return false;
}

bool Options::setPMPathName(const Glib::ustring &optionName, const Glib::ustring &path, bool hasValue)
{
  if(hasValue)
    m_pmPath = path;

  return true;
}

Glib::ustring Options::getPresetManagerPath() const
{
  return m_pmPath;
}

Glib::ustring Options::getBBBB() const
{
  return m_bbbb;
}

ustring Options::getAudioEngineHost() const
{
  return m_audioEngineHost;
}

Glib::ustring Options::getSelfPath() const
{
  return m_selfPath;
}

Glib::ustring Options::getSettingsFile() const
{
  return m_settingsFile;
}

Glib::ustring Options::getKioskModeFile() const
{
  return m_kioskModeFile;
}

Glib::ustring Options::getHardwareTestsFolder() const
{
  const char *folder = "/home/hhoegelo/hw_tests-binaries";
  //const char *folder = "/nonlinear/hw_tests-binaries";
  return folder;
}
