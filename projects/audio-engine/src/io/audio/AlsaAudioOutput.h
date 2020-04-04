#pragma once

#include "Types.h"
#include "AudioOutput.h"
#include <functional>
#include <thread>
#include <memory>
#include <vector>
#include <tuple>
#include <future>
#include <alsa/asoundlib.h>

class AudioWriterBase;
class HighPriorityTask;
class AudioEngineOptions;

class AlsaAudioOutput : public AudioOutput
{
 public:
  using Callback = std::function<void(SampleFrame *target, size_t numFrames)>;

  AlsaAudioOutput(const AudioEngineOptions *options, const std::string &deviceName, Callback cb);
  ~AlsaAudioOutput();

  void start() override;
  void stop() override;

 private:
  void open(const std::string &deviceName);

  void close();
  void doBackgroundWork();
  void handleWriteError(snd_pcm_sframes_t result);
  void playback(SampleFrame *frames, size_t numFrames);
  template <typename T> snd_pcm_sframes_t playbackIntLE(const SampleFrame *frames, size_t numFrames);
  snd_pcm_sframes_t playbackF32(SampleFrame *frames, size_t numFrames);
  snd_pcm_sframes_t playbackInt24LE(const SampleFrame *frames, size_t numFrames);
  void writeTimestamps();

  Callback m_cb;
  snd_pcm_t *m_handle = nullptr;
  std::unique_ptr<HighPriorityTask> m_task;
  bool m_run = true;

  snd_pcm_uframes_t m_framesProcessed = 0;
  snd_pcm_uframes_t m_numFramesPerPeriod = 0;

  std::unique_ptr<AudioWriterBase> m_writer;
  const AudioEngineOptions *m_options;

  std::vector<std::tuple<int64_t, int64_t, int64_t>> m_timestamps;
  uint64_t m_timestampWriteHead = 0;
  uint64_t m_timestampReadHead = 0;
  std::future<void> m_timestampWriter;
  bool m_close = false;
};
