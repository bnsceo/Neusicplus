#include "AudioEngine.h"

AudioEngine::AudioEngine()
{
    formatManager.registerBasicFormats();
    for (int i = 0; i < trackCount; ++i)
        tracks[(size_t) i].state.name = "Track " + juce::String(i + 1);
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
    stop();
    deviceManager.removeAudioCallback(this);
    deviceManager.closeAudioDevice();
    releaseTracks();
}

bool AudioEngine::validTrack(int index) const noexcept
{
    return index >= 0 && index < trackCount;
}

bool AudioEngine::importAudio(int index, const juce::File& file)
{
    if (! validTrack(index) || ! file.existsAsFile())
        return false;

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (reader == nullptr)
        return false;

    const juce::ScopedLock lock(trackLock);
    auto& track = tracks[(size_t) index];
    track.transport.stop();
    track.transport.setSource(nullptr);
    track.readerSource = std::make_unique<juce::AudioFormatReaderSource>(reader.release(), true);

    auto* sourceReader = track.readerSource->getAudioFormatReader();
    if (sourceReader == nullptr)
        return false;

    track.transport.setSource(track.readerSource.get(), 0, nullptr, sourceReader->sampleRate);
    if (sampleRate.load() > 0.0)
        track.transport.prepareToPlay(juce::jmax(256, bufferSize.load()), sampleRate.load());

    track.state.file = file;
    track.state.name = file.getFileNameWithoutExtension();
    track.state.lengthSeconds = static_cast<double>(sourceReader->lengthInSamples) / sourceReader->sampleRate;
    return true;
}

void AudioEngine::clearTrack(int index)
{
    if (! validTrack(index))
        return;

    const juce::ScopedLock lock(trackLock);
    auto& track = tracks[(size_t) index];
    track.transport.stop();
    track.transport.setSource(nullptr);
    track.readerSource.reset();
    track.state = {};
    track.state.name = "Track " + juce::String(index + 1);
}

AudioEngine::TrackState AudioEngine::getTrackState(int index) const
{
    if (! validTrack(index))
        return {};

    const juce::ScopedLock lock(trackLock);
    return tracks[(size_t) index].state;
}

void AudioEngine::setTrackGain(int index, float value)
{
    if (! validTrack(index)) return;
    const juce::ScopedLock lock(trackLock);
    tracks[(size_t) index].state.gain = juce::jlimit(0.0f, 1.5f, value);
}

void AudioEngine::setTrackPan(int index, float value)
{
    if (! validTrack(index)) return;
    const juce::ScopedLock lock(trackLock);
    tracks[(size_t) index].state.pan = juce::jlimit(-1.0f, 1.0f, value);
}

void AudioEngine::setTrackMuted(int index, bool value)
{
    if (! validTrack(index)) return;
    const juce::ScopedLock lock(trackLock);
    tracks[(size_t) index].state.muted = value;
}

void AudioEngine::setTrackSolo(int index, bool value)
{
    if (! validTrack(index)) return;
    const juce::ScopedLock lock(trackLock);
    tracks[(size_t) index].state.solo = value;
}

bool AudioEngine::shouldPlayTrack(int index) const
{
    bool anySolo = false;
    for (const auto& track : tracks)
        anySolo = anySolo || track.state.solo;

    const auto& state = tracks[(size_t) index].state;
    return ! state.muted && (! anySolo || state.solo);
}

void AudioEngine::play()
{
    const juce::ScopedLock lock(trackLock);
    for (auto& track : tracks)
        if (track.readerSource != nullptr)
            track.transport.start();
    playing.store(true);
}

void AudioEngine::pause()
{
    const juce::ScopedLock lock(trackLock);
    for (auto& track : tracks)
        track.transport.stop();
    playing.store(false);
}

void AudioEngine::stop()
{
    pause();
    rewind();
}

void AudioEngine::rewind()
{
    setPosition(0.0);
}

void AudioEngine::setPosition(double seconds)
{
    const juce::ScopedLock lock(trackLock);
    for (auto& track : tracks)
        track.transport.setPosition(juce::jmax(0.0, seconds));
}

bool AudioEngine::isPlaying() const noexcept
{
    return playing.load();
}

double AudioEngine::getPosition() const noexcept
{
    const juce::ScopedLock lock(trackLock);
    for (const auto& track : tracks)
        if (track.readerSource != nullptr)
            return track.transport.getCurrentPosition();
    return 0.0;
}

double AudioEngine::getProjectLength() const noexcept
{
    double length = 0.0;
    const juce::ScopedLock lock(trackLock);
    for (const auto& track : tracks)
        length = juce::jmax(length, track.state.lengthSeconds);
    return length;
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

    lastRecording = directory.getChildFile("Neusic_" + juce::Time::getCurrentTime().formatted("%Y-%m-%d_%H-%M-%S") + ".wav");
    lastRecording.deleteFile();
    auto output = lastRecording.createOutputStream();
    if (output == nullptr)
        return false;

    juce::WavAudioFormat wav;
    std::unique_ptr<juce::AudioFormatWriter> writer(
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

bool AudioEngine::isRecording() const noexcept { return activeWriter.load() != nullptr; }
juce::File AudioEngine::getLastRecording() const { return lastRecording; }

bool AudioEngine::saveProject(const juce::File& destination) const
{
    auto root = std::make_unique<juce::DynamicObject>();
    root->setProperty("version", 1);

    juce::Array<juce::var> items;
    for (int i = 0; i < trackCount; ++i)
    {
        const auto state = getTrackState(i);
        auto object = std::make_unique<juce::DynamicObject>();
        object->setProperty("name", state.name);
        object->setProperty("file", state.file.getFullPathName());
        object->setProperty("gain", state.gain);
        object->setProperty("pan", state.pan);
        object->setProperty("muted", state.muted);
        object->setProperty("solo", state.solo);
        items.add(juce::var(object.release()));
    }

    root->setProperty("tracks", items);
    const juce::var project(root.release());
    return destination.replaceWithText(juce::JSON::toString(project, true));
}

bool AudioEngine::loadProject(const juce::File& source)
{
    if (! source.existsAsFile())
        return false;

    const auto json = juce::JSON::parse(source.loadFileAsString());
    auto* root = json.getDynamicObject();
    if (root == nullptr)
        return false;

    auto* array = root->getProperty("tracks").getArray();
    if (array == nullptr)
        return false;

    for (int i = 0; i < juce::jmin(trackCount, array->size()); ++i)
    {
        auto* object = array->getReference(i).getDynamicObject();
        if (object == nullptr)
            continue;

        const juce::File file(object->getProperty("file").toString());
        if (file.existsAsFile())
            importAudio(i, file);
        else
            clearTrack(i);

        setTrackGain(i, static_cast<float>(object->getProperty("gain")));
        setTrackPan(i, static_cast<float>(object->getProperty("pan")));
        setTrackMuted(i, static_cast<bool>(object->getProperty("muted")));
        setTrackSolo(i, static_cast<bool>(object->getProperty("solo")));
    }

    return true;
}

bool AudioEngine::exportMix(const juce::File& destination)
{
    const auto rate = sampleRate.load() > 0.0 ? sampleRate.load() : 44100.0;
    const auto lengthSamples = static_cast<juce::int64>(std::ceil(getProjectLength() * rate));
    if (lengthSamples <= 0)
        return false;

    destination.deleteFile();
    auto stream = destination.createOutputStream();
    if (stream == nullptr)
        return false;

    juce::WavAudioFormat wav;
    std::unique_ptr<juce::AudioFormatWriter> writer(
        wav.createWriterFor(stream.release(), rate, 2, 24, {}, 0));
    if (writer == nullptr)
        return false;

    constexpr int blockSize = 4096;
    juce::AudioBuffer<float> mix(2, blockSize);
    juce::AudioBuffer<float> temp(2, blockSize);

    for (juce::int64 position = 0; position < lengthSamples; position += blockSize)
    {
        const int count = static_cast<int>(juce::jmin<juce::int64>(blockSize, lengthSamples - position));
        mix.clear();

        for (int i = 0; i < trackCount; ++i)
        {
            const auto state = getTrackState(i);
            if (! state.file.existsAsFile())
                continue;

            bool playTrack = true;
            {
                const juce::ScopedLock lock(trackLock);
                playTrack = shouldPlayTrack(i);
            }
            if (! playTrack)
                continue;

            std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(state.file));
            if (reader == nullptr)
                continue;

            temp.clear();
            const auto sourcePosition = static_cast<juce::int64>(
                std::llround(static_cast<double>(position) * reader->sampleRate / rate));
            reader->read(&temp, 0, count, sourcePosition, true, true);

            const float left = state.gain * (state.pan <= 0.0f ? 1.0f : 1.0f - state.pan);
            const float right = state.gain * (state.pan >= 0.0f ? 1.0f : 1.0f + state.pan);
            mix.addFrom(0, 0, temp, 0, 0, count, left);
            mix.addFrom(1, 0, temp, reader->numChannels > 1 ? 1 : 0, 0, count, right);
        }

        writer->writeFromAudioSampleBuffer(mix, 0, count);
    }

    return true;
}

void AudioEngine::setMonitoringEnabled(bool enabled) noexcept { monitoringEnabled.store(enabled); }
bool AudioEngine::isMonitoringEnabled() const noexcept { return monitoringEnabled.load(); }
double AudioEngine::getCurrentSampleRate() const noexcept { return sampleRate.load(); }
int AudioEngine::getCurrentBufferSize() const noexcept { return bufferSize.load(); }

void AudioEngine::prepareTracks(double rate, int blockSize)
{
    const juce::ScopedLock lock(trackLock);
    for (auto& track : tracks)
        track.transport.prepareToPlay(blockSize, rate);
}

void AudioEngine::releaseTracks()
{
    const juce::ScopedLock lock(trackLock);
    for (auto& track : tracks)
        track.transport.releaseResources();
}

void AudioEngine::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    const auto newBufferSize = device != nullptr ? device->getCurrentBufferSizeSamples() : 0;
    const auto newSampleRate = device != nullptr ? device->getCurrentSampleRate() : 0.0;
    sampleRate.store(newSampleRate);
    bufferSize.store(newBufferSize);
    recordingBuffer.setSize(2, juce::jmax(2048, newBufferSize), false, true, true);
    trackBuffer.setSize(2, juce::jmax(2048, newBufferSize), false, true, true);
    prepareTracks(newSampleRate, newBufferSize);
}

void AudioEngine::audioDeviceStopped()
{
    pause();
    stopRecording();
    releaseTracks();
    sampleRate.store(0.0);
    bufferSize.store(0);
}

void AudioEngine::audioDeviceIOCallbackWithContext(
    const float* const* inputs,
    int numInputs,
    float* const* outputs,
    int numOutputs,
    int numSamples,
    const juce::AudioIODeviceCallbackContext&)
{
    juce::ScopedNoDenormals noDenormals;

    for (int channel = 0; channel < numOutputs; ++channel)
        if (outputs[channel] != nullptr)
            juce::FloatVectorOperations::clear(outputs[channel], numSamples);

    {
        const juce::ScopedLock lock(trackLock);
        for (int i = 0; i < trackCount; ++i)
        {
            auto& track = tracks[(size_t) i];
            if (track.readerSource == nullptr || ! shouldPlayTrack(i))
                continue;

            trackBuffer.clear();
            juce::AudioSourceChannelInfo info(&trackBuffer, 0, numSamples);
            track.transport.getNextAudioBlock(info);

            const float left = track.state.gain * (track.state.pan <= 0.0f ? 1.0f : 1.0f - track.state.pan);
            const float right = track.state.gain * (track.state.pan >= 0.0f ? 1.0f : 1.0f + track.state.pan);
            if (numOutputs > 0 && outputs[0] != nullptr)
                juce::FloatVectorOperations::addWithMultiply(outputs[0], trackBuffer.getReadPointer(0), left, numSamples);
            if (numOutputs > 1 && outputs[1] != nullptr)
                juce::FloatVectorOperations::addWithMultiply(outputs[1], trackBuffer.getReadPointer(1), right, numSamples);
        }
    }

    if (monitoringEnabled.load())
        for (int channel = 0; channel < juce::jmin(numInputs, numOutputs); ++channel)
            if (inputs[channel] != nullptr && outputs[channel] != nullptr)
                juce::FloatVectorOperations::add(outputs[channel], inputs[channel], numSamples);

    if (auto* writer = activeWriter.load())
    {
        recordingBuffer.clear();
        for (int channel = 0; channel < 2; ++channel)
        {
            const int sourceChannel = numInputs > 1 ? channel : 0;
            if (numInputs > 0 && inputs[sourceChannel] != nullptr)
                recordingBuffer.copyFrom(channel, 0, inputs[sourceChannel], numSamples);
        }
        writer->write(recordingBuffer.getArrayOfReadPointers(), numSamples);
    }
}
