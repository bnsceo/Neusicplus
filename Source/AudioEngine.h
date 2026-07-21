#pragma once

#include <JuceHeader.h>

class AudioEngine final : public juce::AudioIODeviceCallback
{
public:
    AudioEngine();
    ~AudioEngine() override;

    bool initialise();
    void shutdown();

    bool startRecording();
    void stopRecording();
    bool isRecording() const noexcept;
    juce::File getLastRecording() const;

    void setMonitoringEnabled(bool enabled) noexcept;
    bool isMonitoringEnabled() const noexcept;

    double getCurrentSampleRate() const noexcept;
    int getCurrentBufferSize() const noexcept;

    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;
    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                          int numInputChannels,
                                          float* const* outputChannelData,
                                          int numOutputChannels,
                                          int numSamples,
                                          const juce::AudioIODeviceCallbackContext& context) override;

private:
    juce::AudioDeviceManager deviceManager;
    juce::TimeSliceThread writerThread { "Neusicplus recording writer" };

    std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> threadedWriter;
    std::atomic<juce::AudioFormatWriter::ThreadedWriter*> activeWriter { nullptr };
    juce::CriticalSection writerLock;
    juce::AudioBuffer<float> recordingBuffer { 2, 2048 };

    juce::File lastRecording;
    std::atomic<bool> monitoringEnabled { true };
    std::atomic<double> sampleRate { 0.0 };
    std::atomic<int> bufferSize { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioEngine)
};
