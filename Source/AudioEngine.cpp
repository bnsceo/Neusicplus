#include "AudioEngine.h"

AudioEngine::AudioEngine()
{
    writerThread.startThread();
}

AudioEngine::~AudioEngine()
{
    shutdown();
    writerThread.stopThread(2000);
}

bool AudioEngine::initialise()
{
    const auto error = deviceManager.initialiseWithDefaultDevices(2, 2);
    if (error.isNotEmpty())
    {
        juce::Logger::writeToLog("Audio device error: " + error);
        return false;
    }

    deviceManager.addAudioCallback(this);
    return true;
}

void AudioEngine::shutdown()
{
    stopRecording();
    deviceManager.removeAudioCallback(this);
    deviceManager.closeAudioDevice();
}

bool AudioEngine::startRecording()
{
    const juce::ScopedLock lock(writerLock);

    if (activeWriter.load() != nullptr || sampleRate.load() <= 0.0)
        return false;

    auto directory = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                         .getChildFile("Neusicplus")
                         .getChildFile("Recordings");

    if (! directory.createDirectory())
        return false;

    const auto timestamp = juce::Time::getCurrentTime().formatted("%Y-%m-%d_%H-%M-%S");
    lastRecording = directory.getChildFile("Neusicplus_" + timestamp + ".wav");

    auto output = lastRecording.createOutputStream();
    if (output == nullptr)
        return false;

    juce::WavAudioFormat wav;
    auto writer = std::unique_ptr<juce::AudioFormatWriter>(
        wav.createWriterFor(output.release(), sampleRate.load(), 2, 24, {}, 0));

    if (writer == nullptr)
        return false;

    threadedWriter = std::make_unique<juce::AudioFormatWriter::ThreadedWriter>(
        writer.release(), writerThread, 32768);
    activeWriter.store(threadedWriter.get());
    return true;
}

void AudioEngine::stopRecording()
{
    const juce::ScopedLock lock(writerLock);
    activeWriter.store(nullptr);
    threadedWriter.reset();
}

bool AudioEngine::isRecording() const noexcept
{
    return activeWriter.load() != nullptr;
}

juce::File AudioEngine::getLastRecording() const
{
    return lastRecording;
}

void AudioEngine::setMonitoringEnabled(bool enabled) noexcept
{
    monitoringEnabled.store(enabled);
}

bool AudioEngine::isMonitoringEnabled() const noexcept
{
    return monitoringEnabled.load();
}

double AudioEngine::getCurrentSampleRate() const noexcept
{
    return sampleRate.load();
}

int AudioEngine::getCurrentBufferSize() const noexcept
{
    return bufferSize.load();
}

void AudioEngine::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    const auto newBufferSize = device != nullptr ? device->getCurrentBufferSizeSamples() : 0;
    sampleRate.store(device != nullptr ? device->getCurrentSampleRate() : 0.0);
    bufferSize.store(newBufferSize);
    recordingBuffer.setSize(2, juce::jmax(2048, newBufferSize), false, true, true);
}

void AudioEngine::audioDeviceStopped()
{
    stopRecording();
    sampleRate.store(0.0);
    bufferSize.store(0);
}

void AudioEngine::audioDeviceIOCallbackWithContext(
    const float* const* inputChannelData,
    int numInputChannels,
    float* const* outputChannelData,
    int numOutputChannels,
    int numSamples,
    const juce::AudioIODeviceCallbackContext&)
{
    for (int channel = 0; channel < numOutputChannels; ++channel)
    {
        auto* output = outputChannelData[channel];
        if (output == nullptr)
            continue;

        const auto* input = channel < numInputChannels ? inputChannelData[channel] : nullptr;
        if (monitoringEnabled.load() && input != nullptr)
            juce::FloatVectorOperations::copy(output, input, numSamples);
        else
            juce::FloatVectorOperations::clear(output, numSamples);
    }

    if (auto* writer = activeWriter.load())
    {
        recordingBuffer.clear();
        for (int channel = 0; channel < 2; ++channel)
        {
            const int sourceChannel = numInputChannels > 1 ? channel : 0;
            if (numInputChannels > 0 && inputChannelData[sourceChannel] != nullptr)
                recordingBuffer.copyFrom(channel, 0, inputChannelData[sourceChannel], numSamples);
        }

        writer->write(recordingBuffer.getArrayOfReadPointers(), numSamples);
    }
}
