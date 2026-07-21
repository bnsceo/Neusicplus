#include "MainComponent.h"

namespace
{
constexpr auto background = 0xff090d10;
constexpr auto panel = 0xff151b1f;
constexpr auto panelAlt = 0xff10161a;
constexpr auto line = 0xff2b353b;
constexpr auto text = 0xffedf2f3;
constexpr auto muted = 0xff87939a;
constexpr auto amber = 0xffe2a84b;
constexpr auto cyan = 0xff32e7ff;
constexpr auto red = 0xffff5c66;
}

MainComponent::MainComponent()
{
    setOpaque(true);
    setSize(980, 720);

    auto prepButton = [this](juce::Button& button)
    {
        button.setColour(juce::TextButton::buttonColourId, juce::Colour(panel));
        button.setColour(juce::TextButton::buttonOnColourId, juce::Colour(amber));
        button.setColour(juce::TextButton::textColourOffId, juce::Colour(text));
        button.setColour(juce::TextButton::textColourOnId, juce::Colour(background));
        addAndMakeVisible(button);
    };

    brandLabel.setText("NEUSICPLUS", juce::dontSendNotification);
    brandLabel.setColour(juce::Label::textColourId, juce::Colour(text));
    brandLabel.setFont(juce::FontOptions(20.0f, juce::Font::bold));
    addAndMakeVisible(brandLabel);

    statusLabel.setColour(juce::Label::textColourId, juce::Colour(muted));
    statusLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(statusLabel);

    clockLabel.setText("00:00.0", juce::dontSendNotification);
    clockLabel.setColour(juce::Label::textColourId, juce::Colour(amber));
    clockLabel.setJustificationType(juce::Justification::centred);
    clockLabel.setFont(juce::FontOptions(17.0f, juce::Font::bold));
    addAndMakeVisible(clockLabel);

    for (auto* button : { &rewindButton, &playButton, &recordButton, &saveButton, &openButton, &exportButton })
        prepButton(*button);

    monitorButton.setColour(juce::ToggleButton::textColourId, juce::Colour(text));
    monitorButton.setToggleState(false, juce::dontSendNotification);
    monitorButton.onClick = [this] { audioEngine.setMonitoringEnabled(monitorButton.getToggleState()); };
    addAndMakeVisible(monitorButton);

    rewindButton.onClick = [this] { audioEngine.rewind(); };
    playButton.onClick = [this] { togglePlayback(); };
    recordButton.onClick = [this] { toggleRecording(); };
    saveButton.onClick = [this] { saveProject(); };
    openButton.onClick = [this] { openProject(); };
    exportButton.onClick = [this] { exportMix(); };

    for (int i = 0; i < AudioEngine::trackCount; ++i)
    {
        auto& label = trackLabels[(size_t) i];
        label.setColour(juce::Label::textColourId, juce::Colour(text));
        label.setText("Track " + juce::String(i + 1), juce::dontSendNotification);
        addAndMakeVisible(label);

        auto& import = importButtons[(size_t) i];
        import.setButtonText("IMPORT");
        prepButton(import);
        import.onClick = [this, i] { chooseAudioForTrack(i); };

        auto setupSlider = [this](juce::Slider& slider, double min, double max, double value)
        {
            slider.setRange(min, max, 0.01);
            slider.setValue(value, juce::dontSendNotification);
            slider.setSliderStyle(juce::Slider::LinearHorizontal);
            slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 48, 20);
            slider.setColour(juce::Slider::trackColourId, juce::Colour(amber));
            slider.setColour(juce::Slider::backgroundColourId, juce::Colour(line));
            slider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(text));
            slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(panelAlt));
            addAndMakeVisible(slider);
        };

        setupSlider(gainSliders[(size_t) i], 0.0, 1.5, 0.85);
        setupSlider(panSliders[(size_t) i], -1.0, 1.0, 0.0);
        gainSliders[(size_t) i].onValueChange = [this, i] { audioEngine.setTrackGain(i, (float) gainSliders[(size_t) i].getValue()); };
        panSliders[(size_t) i].onValueChange = [this, i] { audioEngine.setTrackPan(i, (float) panSliders[(size_t) i].getValue()); };

        muteButtons[(size_t) i].setButtonText("M");
        soloButtons[(size_t) i].setButtonText("S");
        muteButtons[(size_t) i].setColour(juce::ToggleButton::textColourId, juce::Colour(text));
        soloButtons[(size_t) i].setColour(juce::ToggleButton::textColourId, juce::Colour(text));
        addAndMakeVisible(muteButtons[(size_t) i]);
        addAndMakeVisible(soloButtons[(size_t) i]);
        muteButtons[(size_t) i].onClick = [this, i] { audioEngine.setTrackMuted(i, muteButtons[(size_t) i].getToggleState()); timeline.repaint(); };
        soloButtons[(size_t) i].onClick = [this, i] { audioEngine.setTrackSolo(i, soloButtons[(size_t) i].getToggleState()); timeline.repaint(); };
    }

    addAndMakeVisible(timeline);
    const bool ready = audioEngine.initialise();
    statusLabel.setText(ready ? "AUDIO READY" : "AUDIO ERROR", juce::dontSendNotification);
    startTimerHz(20);
}

MainComponent::~MainComponent()
{
    stopTimer();
    audioEngine.shutdown();
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(background));
    g.setColour(juce::Colour(line));
    g.drawHorizontalLine(58, 0.0f, (float) getWidth());
    g.drawHorizontalLine(124, 0.0f, (float) getWidth());
}

void MainComponent::resized()
{
    auto area = getLocalBounds();
    const int pad = juce::jlimit(8, 16, getWidth() / 50);

    auto header = area.removeFromTop(58).reduced(pad, 7);
    brandLabel.setBounds(header.removeFromLeft(190));
    statusLabel.setBounds(header);

    auto transport = area.removeFromTop(66).reduced(pad, 8);
    const int gap = 6;
    const int small = 58;
    rewindButton.setBounds(transport.removeFromLeft(small)); transport.removeFromLeft(gap);
    playButton.setBounds(transport.removeFromLeft(72)); transport.removeFromLeft(gap);
    recordButton.setBounds(transport.removeFromLeft(72)); transport.removeFromLeft(gap);
    monitorButton.setBounds(transport.removeFromLeft(66)); transport.removeFromLeft(gap);
    clockLabel.setBounds(transport.removeFromLeft(110)); transport.removeFromLeft(gap);
    saveButton.setBounds(transport.removeFromLeft(64)); transport.removeFromLeft(gap);
    openButton.setBounds(transport.removeFromLeft(64)); transport.removeFromLeft(gap);
    exportButton.setBounds(transport.removeFromLeft(76));

    auto mixer = area.removeFromBottom(230).reduced(pad, 6);
    const int rowHeight = juce::jmax(48, mixer.getHeight() / AudioEngine::trackCount);
    for (int i = 0; i < AudioEngine::trackCount; ++i)
    {
        auto row = mixer.removeFromTop(rowHeight).reduced(4, 4);
        trackLabels[(size_t) i].setBounds(row.removeFromLeft(150));
        importButtons[(size_t) i].setBounds(row.removeFromLeft(72)); row.removeFromLeft(5);
        muteButtons[(size_t) i].setBounds(row.removeFromLeft(42));
        soloButtons[(size_t) i].setBounds(row.removeFromLeft(42)); row.removeFromLeft(6);
        const int half = row.getWidth() / 2;
        gainSliders[(size_t) i].setBounds(row.removeFromLeft(half));
        panSliders[(size_t) i].setBounds(row);
    }

    timeline.setBounds(area.reduced(pad, 8));
}

void MainComponent::timerCallback()
{
    updateStatus();
    timeline.repaint();
}

void MainComponent::togglePlayback()
{
    if (audioEngine.isPlaying())
    {
        audioEngine.pause();
        playButton.setButtonText("PLAY");
    }
    else
    {
        audioEngine.play();
        playButton.setButtonText("PAUSE");
    }
}

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
}

void MainComponent::chooseAudioForTrack(int trackIndex)
{
    chooser = std::make_unique<juce::FileChooser>("Import audio", juce::File(), "*.wav;*.aif;*.aiff;*.flac;*.mp3");
    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this, trackIndex](const juce::FileChooser& fc)
        {
            const auto file = fc.getResult();
            if (file.existsAsFile() && audioEngine.importAudio(trackIndex, file))
                refreshTrackLabels();
        });
}

void MainComponent::saveProject()
{
    chooser = std::make_unique<juce::FileChooser>("Save Neusicplus project", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("NeusicplusProject.json"), "*.json");
    chooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& fc) { auto f = fc.getResult(); if (f.getFileExtension().isEmpty()) f = f.withFileExtension("json"); audioEngine.saveProject(f); });
}

void MainComponent::openProject()
{
    chooser = std::make_unique<juce::FileChooser>("Open Neusicplus project", juce::File(), "*.json");
    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& fc) { if (audioEngine.loadProject(fc.getResult())) refreshTrackLabels(); });
}

void MainComponent::exportMix()
{
    chooser = std::make_unique<juce::FileChooser>("Export stereo mix", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("NeusicplusMix.wav"), "*.wav");
    chooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& fc) { auto f = fc.getResult(); if (f.getFileExtension().isEmpty()) f = f.withFileExtension("wav"); audioEngine.exportMix(f); });
}

void MainComponent::refreshTrackLabels()
{
    for (int i = 0; i < AudioEngine::trackCount; ++i)
    {
        const auto s = audioEngine.getTrackState(i);
        trackLabels[(size_t) i].setText(s.name + (s.file.existsAsFile() ? "  •  " + juce::String(s.lengthSeconds, 1) + "s" : ""), juce::dontSendNotification);
        gainSliders[(size_t) i].setValue(s.gain, juce::dontSendNotification);
        panSliders[(size_t) i].setValue(s.pan, juce::dontSendNotification);
        muteButtons[(size_t) i].setToggleState(s.muted, juce::dontSendNotification);
        soloButtons[(size_t) i].setToggleState(s.solo, juce::dontSendNotification);
    }
    timeline.repaint();
}

void MainComponent::updateStatus()
{
    const auto pos = audioEngine.getPosition();
    const int minutes = (int) pos / 60;
    const double seconds = pos - minutes * 60;
    clockLabel.setText(juce::String::formatted("%02d:%04.1f", minutes, seconds), juce::dontSendNotification);

    if (audioEngine.isRecording())
        statusLabel.setText("RECORDING", juce::dontSendNotification);
    else
    {
        const auto rate = (int) audioEngine.getCurrentSampleRate();
        statusLabel.setText(rate > 0 ? juce::String(rate) + " HZ / " + juce::String(audioEngine.getCurrentBufferSize()) : "AUDIO OFFLINE", juce::dontSendNotification);
    }
}

void MainComponent::TimelineView::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(panel));
    g.setColour(juce::Colour(line));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 8.0f, 1.0f);

    auto bounds = getLocalBounds().reduced(12);
    const int rulerHeight = 28;
    const int trackHeight = juce::jmax(44, (bounds.getHeight() - rulerHeight) / AudioEngine::trackCount);
    const double projectLength = juce::jmax(1.0, engine.getProjectLength());

    g.setColour(juce::Colour(muted));
    g.setFont(10.0f);
    for (int marker = 0; marker <= 8; ++marker)
    {
        const int x = bounds.getX() + marker * bounds.getWidth() / 8;
        g.drawVerticalLine(x, (float) bounds.getY(), (float) bounds.getBottom());
        g.drawText(juce::String(projectLength * marker / 8.0, 1) + "s", x + 3, bounds.getY(), 45, rulerHeight, juce::Justification::centredLeft);
    }

    for (int i = 0; i < AudioEngine::trackCount; ++i)
    {
        const int y = bounds.getY() + rulerHeight + i * trackHeight;
        auto row = juce::Rectangle<int>(bounds.getX(), y, bounds.getWidth(), trackHeight - 5);
        g.setColour(juce::Colour(i % 2 == 0 ? panelAlt : background));
        g.fillRoundedRectangle(row.toFloat(), 5.0f);
        const auto state = engine.getTrackState(i);
        if (state.file.existsAsFile())
        {
            auto clip = row.reduced(6, 8);
            clip.setWidth((int) juce::jlimit(20.0, (double) clip.getWidth(), clip.getWidth() * state.lengthSeconds / projectLength));
            const auto colour = i % 2 == 0 ? juce::Colour(amber) : juce::Colour(cyan);
            g.setColour(colour.withAlpha(0.22f));
            g.fillRoundedRectangle(clip.toFloat(), 4.0f);
            g.setColour(colour);
            g.drawRoundedRectangle(clip.toFloat(), 4.0f, 1.0f);
            g.drawText(state.name, clip.reduced(8, 0), juce::Justification::centredLeft);
        }
    }

    const int playheadX = bounds.getX() + (int) (bounds.getWidth() * juce::jlimit(0.0, 1.0, engine.getPosition() / projectLength));
    g.setColour(juce::Colour(red));
    g.drawVerticalLine(playheadX, (float) bounds.getY(), (float) bounds.getBottom());
}
