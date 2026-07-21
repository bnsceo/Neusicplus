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

bool AudioEngine::validTrack(int index) const noexcept { return index >= 0 && index < trackCount; }

bool AudioEngine::importAudio(int index, const juce::File& file)
{
    if (! validTrack(index) || ! file.existsAsFile()) return false;
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (reader == nullptr) return false;
    const juce::ScopedLock lock(trackLock);
    auto& track = tracks[(size_t) index];
    track.transport.stop();
    track.transport.setSource(nullptr);
    track.readerSource = std::make_unique<juce::AudioFormatReaderSource>(reader.release(), true);
    auto* sourceReader = track.readerSource->getAudioFormatReader();
    track.transport.setSource(track.readerSource.get(), 0, nullptr, sourceReader->sampleRate);
    if (sampleRate.load() > 0.0)
        track.transport.prepareToPlay(juce::jmax(256, bufferSize.load()), sampleRate.load());
    track.state.file = file;
    track.state.name = file.getFileNameWithoutExtension();
    track.state.lengthSeconds = sourceReader->lengthInSamples / sourceReader->sampleRate;
    return true;
}

void AudioEngine::clearTrack(int index)
{
    if (! validTrack(index)) return;
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
    if (! validTrack(index)) return {};
    const juce::ScopedLock lock(trackLock);
    return tracks[(size_t) index].state;
}

void AudioEngine::setTrackGain(int i, float v) { if (validTrack(i)) { const juce::ScopedLock l(trackLock); tracks[(size_t)i].state.gain = juce::jlimit(0.0f, 1.5f, v); } }
void AudioEngine::setTrackPan(int i, float v)  { if (validTrack(i)) { const juce::ScopedLock l(trackLock); tracks[(size_t)i].state.pan = juce::jlimit(-1.0f, 1.0f, v); } }
void AudioEngine::setTrackMuted(int i, bool v){ if (validTrack(i)) { const juce::ScopedLock l(trackLock); tracks[(size_t)i].state.muted = v; } }
void AudioEngine::setTrackSolo(int i, bool v) { if (validTrack(i)) { const juce::ScopedLock l(trackLock); tracks[(size_t)i].state.solo = v; } }

bool AudioEngine::shouldPlayTrack(int index) const
{
    bool anySolo = false;
    for (const auto& t : tracks) anySolo = anySolo || t.state.solo;
    const auto& s = tracks[(size_t) index].state;
    return ! s.muted && (! anySolo || s.solo);
}

void AudioEngine::play() { const juce::ScopedLock lock(trackLock); for (auto& t : tracks) if (t.readerSource) t.transport.start(); playing.store(true); }
void AudioEngine::pause() { const juce::ScopedLock lock(trackLock); for (auto& t : tracks) t.transport.stop(); playing.store(false); }
void AudioEngine::stop() { pause(); rewind(); }
void AudioEngine::rewind() { setPosition(0.0); }
void AudioEngine::setPosition(double s) { const juce::ScopedLock l(trackLock); for (auto& t : tracks) t.transport.setPosition(juce::jmax(0.0, s)); }
bool AudioEngine::isPlaying() const noexcept { return playing.load(); }
double AudioEngine::getPosition() const noexcept { const juce::ScopedLock l(trackLock); for (const auto& t : tracks) if (t.readerSource) return t.transport.getCurrentPosition(); return 0.0; }
double AudioEngine::getProjectLength() const noexcept { double length = 0.0; const juce::ScopedLock l(trackLock); for (const auto& t : tracks) length = juce::jmax(length, t.state.lengthSeconds); return length; }

bool AudioEngine::startRecording()
{
    const juce::ScopedLock lock(writerLock);
    if (activeWriter.load() != nullptr || sampleRate.load() <= 0.0) return false;
    auto dir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("Neusicplus").getChildFile("Recordings");
    if (! dir.createDirectory()) return false;
    lastRecording = dir.getChildFile("Neusic_" + juce::Time::getCurrentTime().formatted("%Y-%m-%d_%H-%M-%S") + ".wav");
    auto output = lastRecording.createOutputStream();
    if (output == nullptr) return false;
    juce::WavAudioFormat wav;
    auto writer = std::unique_ptr<juce::AudioFormatWriter>(wav.createWriterFor(output.release(), sampleRate.load(), 2, 24, {}, 0));
    if (writer == nullptr) return false;
    threadedWriter = std::make_unique<juce::AudioFormatWriter::ThreadedWriter>(writer.release(), writerThread, 32768);
    activeWriter.store(threadedWriter.get());
    return true;
}

void AudioEngine::stopRecording() { const juce::ScopedLock lock(writerLock); activeWriter.store(nullptr); threadedWriter.reset(); }
bool AudioEngine::isRecording() const noexcept { return activeWriter.load() != nullptr; }
juce::File AudioEngine::getLastRecording() const { return lastRecording; }

bool AudioEngine::saveProject(const juce::File& destination) const
{
    juce::DynamicObject::Ptr root = new juce::DynamicObject();
    root->setProperty("version", 1);
    juce::Array<juce::var> items;
    for (int i = 0; i < trackCount; ++i)
    {
        const auto s = getTrackState(i);
        juce::DynamicObject::Ptr o = new juce::DynamicObject();
        o->setProperty("name", s.name); o->setProperty("file", s.file.getFullPathName());
        o->setProperty("gain", s.gain); o->setProperty("pan", s.pan);
        o->setProperty("muted", s.muted); o->setProperty("solo", s.solo);
        items.add(juce::var(o.get()));
    }
    root->setProperty("tracks", items);
    return destination.replaceWithText(juce::JSON::toString(juce::var(root.get()), true));
}

bool AudioEngine::loadProject(const juce::File& source)
{
    const auto json = juce::JSON::parse(source);
    auto* root = json.getDynamicObject();
    if (root == nullptr) return false;
    auto* array = root->getProperty("tracks").getArray();
    if (array == nullptr) return false;
    for (int i = 0; i < juce::jmin(trackCount, array->size()); ++i)
    {
        auto* o = array->getReference(i).getDynamicObject(); if (o == nullptr) continue;
        const juce::File file(o->getProperty("file").toString());
        if (file.existsAsFile()) importAudio(i, file);
        setTrackGain(i, (float) o->getProperty("gain")); setTrackPan(i, (float) o->getProperty("pan"));
        setTrackMuted(i, (bool) o->getProperty("muted")); setTrackSolo(i, (bool) o->getProperty("solo"));
    }
    return true;
}

bool AudioEngine::exportMix(const juce::File& destination)
{
    const auto rate = sampleRate.load() > 0.0 ? sampleRate.load() : 44100.0;
    const auto lengthSamples = (juce::int64) std::ceil(getProjectLength() * rate);
    if (lengthSamples <= 0) return false;
    auto stream = destination.createOutputStream(); if (stream == nullptr) return false;
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::AudioFormatWriter> writer(wav.createWriterFor(stream.release(), rate, 2, 24, {}, 0));
    if (! writer) return false;
    constexpr int block = 4096; juce::AudioBuffer<float> mix(2, block), temp(2, block);
    for (juce::int64 pos = 0; pos < lengthSamples; pos += block)
    {
        const int count = (int) juce::jmin<juce::int64>(block, lengthSamples - pos); mix.clear();
        for (int i = 0; i < trackCount; ++i)
        {
            const auto state = getTrackState(i); if (! state.file.existsAsFile() || ! shouldPlayTrack(i)) continue;
            std::unique_ptr<juce::AudioFormatReader> r(formatManager.createReaderFor(state.file)); if (! r) continue;
            temp.clear(); const auto sourcePos = (juce::int64) std::llround((double) pos * r->sampleRate / rate);
            r->read(&temp, 0, count, sourcePos, true, true);
            const float left = state.gain * (state.pan <= 0.0f ? 1.0f : 1.0f - state.pan);
            const float right = state.gain * (state.pan >= 0.0f ? 1.0f : 1.0f + state.pan);
            mix.addFrom(0, 0, temp, 0, 0, count, left); mix.addFrom(1, 0, temp, 1, 0, count, right);
        }
        writer->writeFromAudioSampleBuffer(mix, 0, count);
    }
    return true;
}

void AudioEngine::setMonitoringEnabled(bool e) noexcept { monitoringEnabled.store(e); }
bool AudioEngine::isMonitoringEnabled() const noexcept { return monitoringEnabled.load(); }
double AudioEngine::getCurrentSampleRate() const noexcept { return sampleRate.load(); }
int AudioEngine::getCurrentBufferSize() const noexcept { return bufferSize.load(); }
void AudioEngine::prepareTracks(double rate, int blockSize) { const juce::ScopedLock l(trackLock); for (auto& t : tracks) t.transport.prepareToPlay(blockSize, rate); }
void AudioEngine::releaseTracks() { const juce::ScopedLock l(trackLock); for (auto& t : tracks) t.transport.releaseResources(); }
void AudioEngine::audioDeviceAboutToStart(juce::AudioIODevice* d) { const auto b = d ? d->getCurrentBufferSizeSamples() : 0; const auto r = d ? d->getCurrentSampleRate() : 0.0; sampleRate.store(r); bufferSize.store(b); recordingBuffer.setSize(2, juce::jmax(2048,b), false,true,true); trackBuffer.setSize(2,juce::jmax(2048,b),false,true,true); prepareTracks(r,b); }
void AudioEngine::audioDeviceStopped() { pause(); stopRecording(); releaseTracks(); sampleRate.store(0.0); bufferSize.store(0); }

void AudioEngine::audioDeviceIOCallbackWithContext(const float* const* inputs, int numInputs, float* const* outputs, int numOutputs, int numSamples, const juce::AudioIODeviceCallbackContext&)
{
    for (int c=0;c<numOutputs;++c) if (outputs[c]) juce::FloatVectorOperations::clear(outputs[c],numSamples);
    const juce::ScopedLock lock(trackLock);
    for (int i=0;i<trackCount;++i)
    {
        auto& t=tracks[(size_t)i]; if (! t.readerSource || ! shouldPlayTrack(i)) continue;
        trackBuffer.clear(); juce::AudioSourceChannelInfo info(&trackBuffer,0,numSamples); t.transport.getNextAudioBlock(info);
        const float l=t.state.gain*(t.state.pan<=0?1.0f:1.0f-t.state.pan), r=t.state.gain*(t.state.pan>=0?1.0f:1.0f+t.state.pan);
        if (numOutputs>0 && outputs[0]) juce::FloatVectorOperations::addWithMultiply(outputs[0],trackBuffer.getReadPointer(0),l,numSamples);
        if (numOutputs>1 && outputs[1]) juce::FloatVectorOperations::addWithMultiply(outputs[1],trackBuffer.getReadPointer(1),r,numSamples);
    }
    if (monitoringEnabled.load()) for(int c=0;c<juce::jmin(numInputs,numOutputs);++c) if(inputs[c]&&outputs[c]) juce::FloatVectorOperations::add(outputs[c],inputs[c],numSamples);
    if (auto* w=activeWriter.load()) { recordingBuffer.clear(); for(int c=0;c<2;++c){const int src=numInputs>1?c:0;if(numInputs>0&&inputs[src]) recordingBuffer.copyFrom(c,0,inputs[src],numSamples);} w->write(recordingBuffer.getArrayOfReadPointers(),numSamples); }
}
