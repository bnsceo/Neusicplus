#pragma once

#include <JuceHeader.h>
#include <array>

class AudioEngine final : public juce::AudioIODeviceCallback
{
public:
    static constexpr int trackCount = 4;

    struct TrackState
    {
        juce::String name;
        juce::File file;
        float gain = 0.85f;
        float pan = 0.0f;
        bool muted = false;
        bool solo = false;
        double lengthSeconds = 0.0;
    };

    AudioEngine();
    ~AudioEngine() override;

    bool initialise();
    void shutdown();

    bool importAudio(int trackIndex, const juce::File& file);
    void clearTrack(int trackIndex);
    TrackState getTrackState(int trackIndex) const;
    void setTrackGain(int trackIndex, float gain);
    void setTrackPan(int trackIndex, float pan);
    void setTrackMuted(int trackIndex, bool muted);
    void setTrackSolo(int trackIndex, bool solo);

    void play();
    void pause();
    void stop();
    void rewind();
    void setPosition(double seconds);
    bool isPlaying() const noexcept;
    double getPosition() const noexcept;
    double getProjectLength() const noexcept;

    bool startRecording();
    void stopRecording();
    bool isRecording() const noexcept;
    juce::File getLastRecording() const;

    bool saveProject(const juce::File& destination) const;
    bool loadProject(const juce::File& source);
    bool exportMix(const juce::File& destination);

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
    struct Track
    {
        TrackState state;
        std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
        juce::AudioTransportSource transport;
    };

    bool validTrack(int index) const noexcept;
    bool shouldPlayTrack(int index) const;
    void prepareTracks(double rate, int blockSize);
    void releaseTracks();

    juce::AudioDeviceManager deviceManager;
    juce::AudioFormatManager formatManager;
    std::array<Track, trackCount> tracks;
    mutable juce::CriticalSection trackLock;

    juce::TimeSliceThread writerThread { "Neusicplus recording writer" };
    std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> threadedWriter;
    std::atomic<juce::AudioFormatWriter::ThreadedWriter*> activeWriter { nullptr };
    juce::CriticalSection writerLock;
    juce::AudioBuffer<float> recordingBuffer { 2, 2048 };
    juce::AudioBuffer<float> trackBuffer { 2, 2048 };

    juce::File lastRecording;
    std::atomic<bool> monitoringEnabled { false };
    std::atomic<bool> playing { false };
    std::atomic<double> sampleRate { 0.0 };
    std::atomic<int> bufferSize { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioEngine)
};