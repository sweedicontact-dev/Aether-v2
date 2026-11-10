#include "PluginEditor.h"

namespace
{
const juce::Colour kBgTop    { 0xff0b1118 };
const juce::Colour kBgBottom { 0xff10222c };
const juce::Colour kAccent   { 0xff6fd3e0 };
const juce::Colour kText     { 0xffcfe3f0 };
const juce::Colour kDim      { 0xff7f93a6 };
}

//==============================================================================
AetherLookAndFeel::AetherLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, kText);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff16212d));
    setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff2e3f52));
    setColour (juce::ComboBox::textColourId, kText);
    setColour (juce::ComboBox::arrowColourId, kAccent);
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff121b25));
    setColour (juce::PopupMenu::textColourId, kText);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xff24485a));
    setColour (juce::TextButton::buttonColourId, juce::Colour (0xff16212d));
    setColour (juce::TextButton::textColourOffId, kAccent);
}

void AetherLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                          float startAngle, float endAngle, juce::Slider&)
{
    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (8.0f);
    const float r = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto c  = bounds.getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);
    const float arcR = r - 3.0f;

    juce::Path track;
    track.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
    g.setColour (juce::Colour (0xff1f2c3a));
    g.strokePath (track, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path val;
    val.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAngle, angle, true);
    g.setColour (kAccent);
    g.strokePath (val, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const float bodyR = r - 12.0f;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff22303f), c.x, c.y - bodyR,
                                             juce::Colour (0xff111a23), c.x, c.y + bodyR, false));
    g.fillEllipse (c.x - bodyR, c.y - bodyR, bodyR * 2.0f, bodyR * 2.0f);

    juce::Path pointer;
    pointer.addRoundedRectangle (-1.8f, -bodyR + 3.0f, 3.6f, bodyR * 0.5f, 1.8f);
    g.setColour (kText);
    g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (c.x, c.y));
}

void AetherLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool highlighted, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    const bool on = b.getToggleState();

    g.setColour (on ? kAccent : juce::Colour (highlighted ? 0xff223142 : 0xff16212d));
    g.fillRoundedRectangle (r, 8.0f);
    g.setColour (on ? kAccent : juce::Colour (0xff2e3f52));
    g.drawRoundedRectangle (r, 8.0f, 1.5f);

    g.setColour (on ? juce::Colour (0xff0b1118) : kDim);
    g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    g.drawText (b.getButtonText(), b.getLocalBounds(), juce::Justification::centred);
}

//==============================================================================
AetherEditor::AetherEditor (AetherProcessor& p) : juce::AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&laf);

    struct Def { const char* id; const char* label; };
    static const Def defs[] = {
        { "size", "SIZE" }, { "decay", "DECAY" }, { "damp", "DAMPING" }, { "predelay", "PRE-DELAY" },
        { "moddepth", "MOD DEPTH" }, { "modrate", "MOD RATE" }, { "tape", "TAPE" },
        { "shimmer", "SHIMMER" }, { "lowcut", "LOW CUT" }, { "highcut", "HIGH CUT" },
        { "width", "WIDTH" }, { "mix", "MIX" }, { "out", "OUTPUT" }
    };

    for (const auto& d : defs)
    {
        auto k = std::make_unique<Knob>();
        k->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        k->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 96, 18);
        k->slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                       juce::MathConstants<float>::pi * 2.75f, true);
        k->attachment = std::make_unique<APVTS::SliderAttachment> (proc.apvts, d.id, k->slider);

        k->label.setText (d.label, juce::dontSendNotification);
        k->label.setJustificationType (juce::Justification::centred);
        k->label.setColour (juce::Label::textColourId, kDim);
        k->label.setFont (juce::FontOptions (12.0f, juce::Font::bold));

        addAndMakeVisible (k->slider);
        addAndMakeVisible (k->label);
        knobs.push_back (std::move (k));
    }

    intervalLabel.setText ("SHIMMER INTERVAL", juce::dontSendNotification);
    intervalLabel.setJustificationType (juce::Justification::centred);
    intervalLabel.setColour (juce::Label::textColourId, kDim);
    intervalLabel.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    addAndMakeVisible (intervalLabel);

    intervalBox.addItemList ({ "+12  Octava", "+7  Quinta", "-12  Octava abajo", "+19  Octava + Quinta" }, 1);
    addAndMakeVisible (intervalBox);
    intervalAtt = std::make_unique<APVTS::ComboBoxAttachment> (proc.apvts, "shimint", intervalBox);

    addAndMakeVisible (freezeBtn);
    freezeAtt = std::make_unique<APVTS::ButtonAttachment> (proc.apvts, "freeze", freezeBtn);

    for (int i = 0; i < proc.getNumPrograms(); ++i)
        presetBox.addItem (proc.getProgramName (i), i + 1);
    presetBox.setSelectedItemIndex (proc.getCurrentProgram(), juce::dontSendNotification);
    presetBox.onChange = [this] { proc.setCurrentProgram (presetBox.getSelectedItemIndex()); };
    addAndMakeVisible (presetBox);

    prevBtn.onClick = [this] { stepProgram (-1); };
    nextBtn.onClick = [this] { stepProgram (+1); };
    addAndMakeVisible (prevBtn);
    addAndMakeVisible (nextBtn);

    setSize (900, 500); // al final: resized() necesita que los controles ya existan
    startTimerHz (8);
}

AetherEditor::~AetherEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void AetherEditor::stepProgram (int delta)
{
    const int n = proc.getNumPrograms();
    const int next = (proc.getCurrentProgram() + delta + n) % n;
    proc.setCurrentProgram (next);
    presetBox.setSelectedItemIndex (next, juce::dontSendNotification);
}

void AetherEditor::timerCallback()
{
    const int cur = proc.getCurrentProgram();
    if (presetBox.getSelectedItemIndex() != cur)
        presetBox.setSelectedItemIndex (cur, juce::dontSendNotification);
}

void AetherEditor::paint (juce::Graphics& g)
{
    g.setGradientFill (juce::ColourGradient (kBgTop, 0.0f, 0.0f, kBgBottom, 0.0f, (float) getHeight(), false));
    g.fillAll();

    g.setGradientFill (juce::ColourGradient (kAccent.withAlpha (0.10f), (float) getWidth() * 0.5f, (float) getHeight() * 0.75f,
                                             juce::Colours::transparentBlack, (float) getWidth() * 0.5f, 0.0f, true));
    g.fillAll();

    g.setColour (kText);
    g.setFont (juce::FontOptions (34.0f, juce::Font::bold));
    g.drawText ("A E T H E R", 24, 18, 340, 40, juce::Justification::centredLeft);

    g.setColour (kDim);
    g.setFont (juce::FontOptions (12.0f));
    g.drawText ("AMBIENT TEXTURE ENGINE", 26, 54, 340, 18, juce::Justification::centredLeft);

    g.setColour (juce::Colour (0xff1f2c3a));
    g.fillRect (20, 92, getWidth() - 40, 1);
}

void AetherEditor::resized()
{
    if (knobs.size() < 13)
        return;

    auto area = getLocalBounds();
    auto header = area.removeFromTop (92).reduced (20, 26);

    freezeBtn.setBounds (header.removeFromRight (100));
    header.removeFromRight (14);
    nextBtn.setBounds (header.removeFromRight (36));
    header.removeFromRight (6);
    presetBox.setBounds (header.removeFromRight (270));
    header.removeFromRight (6);
    prevBtn.setBounds (header.removeFromRight (36));

    area.reduce (20, 8);
    const int cols = 7, rows = 2;
    const int cw = area.getWidth() / cols, ch = area.getHeight() / rows;

    auto cell = [&] (int r, int c) {
        return juce::Rectangle<int> (area.getX() + c * cw, area.getY() + r * ch, cw, ch).reduced (4);
    };
    auto place = [] (Knob& k, juce::Rectangle<int> b) {
        k.label.setBounds (b.removeFromTop (18));
        k.slider.setBounds (b);
    };

    for (int i = 0; i < 7; ++i)
        place (*knobs[(size_t) i], cell (0, i));

    place (*knobs[7], cell (1, 0));

    auto ib = cell (1, 1);
    intervalLabel.setBounds (ib.removeFromTop (18));
    intervalBox.setBounds (ib.withSizeKeepingCentre (ib.getWidth() - 6, 30).translated (0, -18));

    for (int i = 8; i < 13; ++i)
        place (*knobs[(size_t) i], cell (1, i - 6));
}

