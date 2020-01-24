#include <Application.h>
#include <device-settings/DebugLevel.h>
#include <parameters/descriptions/ParameterDescriptionDatabase.h>
#include <parameters/MacroControlParameter.h>
#include <presets/EditBuffer.h>
#include <presets/PresetManager.h>
#include <tools/Signal.h>
#include <giomm/file.h>
#include <CompileTimeOptions.h>

class ParameterDescriptionDatabase::Job
{
 public:
  Job(ParameterId paramID)
      : paramID(paramID)
  {
    load();
  }

  sigc::connection connect(sigc::slot<void, const Glib::ustring &> cb)
  {
    return sigLoaded.connectAndInit(cb, text);
  }

 private:
  void load()
  {
    auto param = Application::get().getPresetManager()->getEditBuffer()->findParameterByID(paramID);

    if(auto mc = dynamic_cast<MacroControlParameter *>(param))
    {
      loadFromParameter(mc);
    }
    else
    {
      loadFromFile();
    }
  }

  void loadFromParameter(MacroControlParameter *mc)
  {
    m_connection = mc->onParameterChanged(sigc::mem_fun(this, &Job::onParameterChanged));
  }

  void onParameterChanged(const Parameter *p)
  {
    if(auto mc = dynamic_cast<const MacroControlParameter *>(p))
    {
      const auto &info = mc->getInfo();
      if(std::exchange(text, info) != info)
      {
        notifyListeners();
      }
    }
  }

  void loadFromFile()
  {
    try
    {
      file = Gio::File::create_for_path(getResourcesDir() + "/parameter-descriptions/" + to_string(paramID.getNumber())
                                        + ".txt");
      if(file->query_exists())
      {
        file->read_async(mem_fun(this, &Job::onReadFinish));
      }
    }
    catch(...)
    {
      DebugLevel::error("Exception caught in", __PRETTY_FUNCTION__, __LINE__);
      notifyListeners();
    }
  }

  void notifyListeners()
  {
    sigLoaded.send(text);
  }

  void onReadFinish(Glib::RefPtr<Gio::AsyncResult> &result)
  {
    try
    {
      stream = file->read_finish(result);
      stream->read_bytes_async(1024, mem_fun(this, &Job::onBytesRead));
    }
    catch(...)
    {
      DebugLevel::error("Exception caught in", __PRETTY_FUNCTION__, __LINE__);
      notifyListeners();
    }
  }

  void onBytesRead(Glib::RefPtr<Gio::AsyncResult> &result)
  {
    try
    {
      finishBytesRead(result);
    }
    catch(...)
    {
      DebugLevel::error("Exception caught in", __PRETTY_FUNCTION__, __LINE__);
      notifyListeners();
    }
  }

  void finishBytesRead(Glib::RefPtr<Gio::AsyncResult> result)
  {
    gsize len = 0;
    auto bytes = stream->read_bytes_finish(result);
    const char *data = (const char *) ((bytes->get_data(len)));
    if(len > 0)
    {
      text.append(data, data + len);
      stream->read_bytes_async(1024, mem_fun(this, &Job::onBytesRead));
    }
    else
    {
      notifyListeners();
    }
  }

  ParameterId paramID;
  Glib::ustring text;
  Glib::RefPtr<Gio::File> file;
  Glib::RefPtr<Gio::FileInputStream> stream;
  Signal<void, const Glib::ustring &> sigLoaded;
  sigc::connection m_connection;
};

ParameterDescriptionDatabase &ParameterDescriptionDatabase::get()
{
  static ParameterDescriptionDatabase db;
  return db;
}

ParameterDescriptionDatabase::ParameterDescriptionDatabase()
{
}

sigc::connection ParameterDescriptionDatabase::load(ParameterId paramID, sigc::slot<void, const Glib::ustring &> cb)
{
  auto it = m_jobs.find(paramID);

  if(it != m_jobs.end())
    return it->second->connect(cb);

  auto job = std::make_shared<Job>(paramID);
  m_jobs[paramID] = job;
  return job->connect(cb);
}