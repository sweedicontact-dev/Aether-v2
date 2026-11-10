#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

//==============================================================================
class AetherLookAndFeel : public juce::LookAndFeel_V4
{
public:
    AetherLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
};

//==============================================================================
class AetherEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit AetherEditor (AetherProcessor&);
    ~AetherEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void stepProgram (int delta);

    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<APVTS::SliderAttachment> attachment;
    };

    AetherProcessor& proc;
    AetherLookAndFeel laf;

    std::vector<std::unique_ptr<Knob>> knobs;
    juce::ComboBox presetBox, intervalBox;
    juce::TextButton prevBtn { "<" }, nextBtn { ">" };
    juce::ToggleButton freezeBtn { "FREEZE" };
    juce::Label intervalLabel;
    std::unique_ptr<APVTS::ComboBoxAttachment> intervalAtt;
    std::unique_ptr<APVTS::ButtonAttachment> freezeAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AetherEditor)
};
