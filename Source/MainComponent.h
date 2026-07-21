#pragma once

#include <JuceHeader.h>
#include <array>
#include "AudioEngine.h"

class MainComponent final : public juce::Component,
                            private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    class TimelineView final : public juce::Component
    {
    public:
        explicit TimelineView(AudioEngine& engineToUse) : engine(engineToUse) {}
        void paint(juce::Graphics& g) override;
    private:
        AudioEngine& engine;
    };

    void timerCallback() override;
    void toggleRecording();
    void togglePlayback();
    void chooseAudioForTrack(int trackIndex);
    void saveProject();
    void openProject();
    void exportMix();
    void updateStatus();
    void refreshTrackLabels();

    AudioEngine audioEngine;

    juce::Label brandLabel;
    juce::Label statusLabel;
    juce::Label clockLabel;
    juce::TextButton rewindButton { "|<" };
    juce::TextButton playButton { "PLAY" };
    juce::TextButton recordButton { "REC" };
    juce::ToggleButton monitorButton { "MON" };
    juce::TextButton saveButton { "SAVE" };
    juce::TextButton openButton { "OPEN" };
    juce::TextButton exportButton { "EXPORT" };

    TimelineView timeline { audioEngine };

    std::array<juce::Label, AudioEngine::trackCount> trackLabels;
    std::array<juce::TextButton, AudioEngine::trackCount> importButtons;
    std::array<juce::Slider, AudioEngine::trackCount> gainSliders;
    std::array<juce::Slider, AudioEngine::trackCount> panSliders;
    std::array<juce::ToggleButton, AudioEngine::trackCount> muteButtons;
    std::array<juce::ToggleButton, AudioEngine::trackCount> soloButtons;

    std::unique_ptr<juce::FileChooser> chooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};