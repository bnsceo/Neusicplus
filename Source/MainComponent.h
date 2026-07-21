#pragma once

#include <JuceHeader.h>
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
        void paint(juce::Graphics& g) override;
    };

    void timerCallback() override;
    void toggleRecording();
    void updateStatus();

    AudioEngine audioEngine;

    juce::Label brandLabel;
    juce::Label statusLabel;
    juce::TextButton rewindButton { "|<" };
    juce::TextButton playButton { "PLAY" };
    juce::TextButton recordButton { "REC" };
    juce::ToggleButton monitorButton { "MONITOR" };

    TimelineView timeline;

    juce::TextButton arrangeTab { "ARRANGE" };
    juce::TextButton recordTab { "RECORD" };
    juce::TextButton soundsTab { "SOUNDS" };
    juce::TextButton mixerTab { "MIXER" };
    juce::TextButton exportTab { "EXPORT" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
