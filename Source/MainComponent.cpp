#include "MainComponent.h"

namespace
{
constexpr auto background = 0xff0b0f12;
constexpr auto panel = 0xff151b1f;
constexpr auto line = 0xff2b353b;
constexpr auto text = 0xffedf2f3;
constexpr auto muted = 0xff87939a;
constexpr auto amber = 0xffe2a84b;
constexpr auto red = 0xffff5c66;
}

MainComponent::MainComponent()
{
    setOpaque(true);
    setSize(430, 820);

    brandLabel.setText("NEUSICPLUS", juce::dontSendNotification);
    brandLabel.setColour(juce::Label::textColourId, juce::Colour(text));
    brandLabel.setFont(juce::FontOptions(18.0f, juce::Font::bold));
    addAndMakeVisible(brandLabel);

    statusLabel.setColour(juce::Label::textColourId, juce::Colour(muted));
    statusLabel.setJustificationType(juce::Justification::centredRight);
    statusLabel.setFont(juce::FontOptions(11.0f));
    addAndMakeVisible(statusLabel);

    auto prepareButton = [this](juce::Button& button)
    {
        button.setColour(juce::TextButton::buttonColourId, juce::Colour(panel));
        button.setColour(juce::TextButton::buttonOnColourId, juce::Colour(amber));
        button.setColour(juce::TextButton::textColourOffId, juce::Colour(text));
        button.setColour(juce::TextButton::textColourOnId, juce::Colour(background));
        addAndMakeVisible(button);
    };

    for (auto* button : { &rewindButton, &playButton, &recordButton, &arrangeTab, &recordTab, &soundsTab, &mixerTab, &exportTab })
        prepareButton(*button);

    monitorButton.setColour(juce::ToggleButton::textColourId, juce::Colour(text));
    monitorButton.setToggleState(true, juce::dontSendNotification);
    monitorButton.onClick = [this] { audioEngine.setMonitoringEnabled(monitorButton.getToggleState()); };
    addAndMakeVisible(monitorButton);

    recordButton.onClick = [this] { toggleRecording(); };
    playButton.onClick = [this]
    {
        playButton.setToggleState(! playButton.getToggleState(), juce::dontSendNotification);
        playButton.setButtonText(playButton.getToggleState() ? "PAUSE" : "PLAY");
    };

    arrangeTab.setToggleState(true, juce::dontSendNotification);
    addAndMakeVisible(timeline);

    const bool ready = audioEngine.initialise();
    statusLabel.setText(ready ? "AUDIO READY" : "AUDIO ERROR", juce::dontSendNotification);
    startTimerHz(10);
}

MainComponent::~MainComponent()
{
    stopTimer();
    audioEngine.shutdown();
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(background));
    const auto bounds = getLocalBounds().toFloat();
    g.setColour(juce::Colour(line));
    g.drawLine(0.0f, 58.0f, bounds.getWidth(), 58.0f, 1.0f);
    g.drawLine(0.0f, 128.0f, bounds.getWidth(), 128.0f, 1.0f);
    g.drawLine(0.0f, bounds.getHeight() - 70.0f, bounds.getWidth(), bounds.getHeight() - 70.0f, 1.0f);
}

void MainComponent::resized()
{
    auto area = getLocalBounds();
    const int padding = juce::jlimit(10, 18, getWidth() / 28);

    auto header = area.removeFromTop(58).reduced(padding, 8);
    brandLabel.setBounds(header.removeFromLeft(190));
    statusLabel.setBounds(header);

    auto transport = area.removeFromTop(70).reduced(padding, 10);
    const int gap = 8;
    const int buttonWidth = juce::jmax(58, (transport.getWidth() - gap * 3) / 4);
    rewindButton.setBounds(transport.removeFromLeft(buttonWidth));
    transport.removeFromLeft(gap);
    playButton.setBounds(transport.removeFromLeft(buttonWidth));
    transport.removeFromLeft(gap);
    recordButton.setBounds(transport.removeFromLeft(buttonWidth));
    transport.removeFromLeft(gap);
    monitorButton.setBounds(transport);

    auto tabs = area.removeFromBottom(70).reduced(padding, 9);
    const int tabGap = 5;
    const int tabWidth = (tabs.getWidth() - tabGap * 4) / 5;
    for (auto* button : { &arrangeTab, &recordTab, &soundsTab, &mixerTab, &exportTab })
    {
        button->setBounds(tabs.removeFromLeft(tabWidth));
        tabs.removeFromLeft(tabGap);
    }

    timeline.setBounds(area.reduced(padding, 10));
}

void MainComponent::timerCallback() { updateStatus(); }

void MainComponent::toggleRecording()
{
    if (audioEngine.isRecording())
    {
        audioEngine.stopRecording();
        recordButton.setButtonText("REC");
        recordButton.setColour(juce::TextButton::buttonColourId, juce::Colour(panel));
    }
    else if (audioEngine.startRecording())
    {
        recordButton.setButtonText("STOP");
        recordButton.setColour(juce::TextButton::buttonColourId, juce::Colour(red));
    }
    repaint();
}

void MainComponent::updateStatus()
{
    if (audioEngine.isRecording())
    {
        statusLabel.setText("RECORDING", juce::dontSendNotification);
        return;
    }

    const auto rate = static_cast<int>(audioEngine.getCurrentSampleRate());
    const auto buffer = audioEngine.getCurrentBufferSize();
    statusLabel.setText(rate > 0 ? juce::String(rate) + " HZ / " + juce::String(buffer) : "AUDIO OFFLINE",
                        juce::dontSendNotification);
}

void MainComponent::TimelineView::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(panel));
    g.setColour(juce::Colour(line));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 8.0f, 1.0f);

    const auto bounds = getLocalBounds().reduced(12);
    const int rulerHeight = 28;
    const int trackHeight = juce::jmax(58, (bounds.getHeight() - rulerHeight - 30) / 4);

    g.setColour(juce::Colour(muted));
    g.setFont(10.0f);
    for (int beat = 0; beat < 9; ++beat)
    {
        const auto x = bounds.getX() + beat * bounds.getWidth() / 8;
        g.drawVerticalLine(x, static_cast<float>(bounds.getY()), static_cast<float>(bounds.getBottom()));
        if (beat < 8)
            g.drawText(juce::String(beat + 1), x + 4, bounds.getY(), 22, rulerHeight, juce::Justification::centredLeft);
    }

    for (int track = 0; track < 4; ++track)
    {
        const int y = bounds.getY() + rulerHeight + track * trackHeight;
        auto trackArea = juce::Rectangle<int>(bounds.getX(), y, bounds.getWidth(), trackHeight - 6);
        g.setColour(juce::Colour(track % 2 == 0 ? 0xff10161a : 0xff0e1316));
        g.fillRoundedRectangle(trackArea.toFloat(), 5.0f);
        g.setColour(juce::Colour(line));
        g.drawRoundedRectangle(trackArea.toFloat(), 5.0f, 1.0f);
        g.setColour(juce::Colour(text));
        g.drawText("TRACK " + juce::String(track + 1), trackArea.removeFromLeft(72), juce::Justification::centred);

        if (track < 2)
        {
            auto clip = trackArea.reduced(8, 10);
            clip.setWidth(juce::jmax(80, clip.getWidth() * (track == 0 ? 2 : 1) / 3));
            g.setColour(juce::Colour(track == 0 ? amber : 0xff32e7ff).withAlpha(0.24f));
            g.fillRoundedRectangle(clip.toFloat(), 4.0f);
            g.setColour(juce::Colour(track == 0 ? amber : 0xff32e7ff));
            g.drawRoundedRectangle(clip.toFloat(), 4.0f, 1.0f);
        }
    }
}
