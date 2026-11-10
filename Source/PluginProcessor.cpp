#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"

//==============================================================================
APVTS::ParameterLayout AetherProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    auto pct   = [] (float v, int) { return String ((int) std::round (v * 100.0f)) + " %"; };
    auto unpct = [] (const String& t) { return t.getFloatValue() / 100.0f; };
    auto secs  = [] (float v, int) { return String (v, v < 10.0f ? 2 : 1) + " s"; };
    auto msec  = [] (float v, int) { return String (v, 1) + " ms"; };
    auto hz    = [] (float v, int) { return v >= 1000.0f ? String (v / 1000.0f, 2) + " kHz" : String ((int) v) + " Hz"; };
    auto unhz  = [] (const String& t) { return t.getFloatValue() * (t.containsIgnoreCase ("k") ? 1000.0f : 1.0f); };
    auto hzlfo = [] (float v, int) { return String (v, 2) + " Hz"; };
    auto db    = [] (float v, int) { return String (v, 1) + " dB"; };

    auto attrs = [] (auto toStr, auto fromStr) {
        return AudioParameterFloatAttributes().withStringFromValueFunction (toStr).withValueFromStringFunction (fromStr);
    };
    auto fl = [&] (const char* id, const char* name, NormalisableRange<float> r, float def, auto toStr, auto fromStr) {
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, r, def, attrs (toStr, fromStr)));
    };

    fl ("mix",      "Mix",        { 0.0f, 1.0f },                         0.35f,   pct,   unpct);
    fl ("size",     "Size",       { 0.0f, 1.0f },                         0.60f,   pct,   unpct);
    fl ("decay",    "Decay",      { 0.3f, 60.0f, 0.01f, 0.4f },           6.0f,    secs,  [] (const String& t) { return t.getFloatValue(); });
    fl ("damp",     "Damping",    { 500.0f, 20000.0f, 1.0f, 0.35f },      6000.0f, hz,    unhz);
    fl ("predelay", "Pre-Delay",  { 0.0f, 250.0f, 0.1f },                 20.0f,   msec,  [] (const String& t) { return t.getFloatValue(); });
    fl ("shimmer",  "Shimmer",    { 0.0f, 1.0f },                         0.0f,    pct,   unpct);
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "shimint", 1 }, "Shimmer Interval",
                 StringArray { "+12  Octava", "+7  Quinta", "-12  Octava abajo", "+19  Octava + Quinta" }, 0));
    fl ("moddepth", "Mod Depth",  { 0.0f, 1.0f },                         0.30f,   pct,   unpct);
    fl ("modrate",  "Mod Rate",   { 0.05f, 3.0f, 0.001f, 0.5f },          0.40f,   hzlfo, [] (const String& t) { return t.getFloatValue(); });
    fl ("tape",     "Tape",       { 0.0f, 1.0f },                         0.0f,    pct,   unpct);
    fl ("lowcut",   "Low Cut",    { 20.0f, 1000.0f, 1.0f, 0.4f },         80.0f,   hz,    unhz);
    fl ("highcut",  "High Cut",   { 1000.0f, 20000.0f, 1.0f, 0.4f },      12000.0f, hz,   unhz);
    fl ("width",    "Width",      { 0.0f, 2.0f },                         1.2f,    pct,   unpct);
    fl ("out",      "Output",     { -24.0f, 6.0f, 0.1f },                 0.0f,    db,    [] (const String& t) { return t.getFloatValue(); });
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "freeze", 1 }, "Freeze", false));

    return { p.begin(), p.end() };
}

//==============================================================================
AetherProcessor::AetherProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "AETHER", createLayout())
{
    pMix = apvts.getRawParameterValue ("mix");       pSize = apvts.getRawParameterValue ("size");
    pDecay = apvts.getRawParameterValue ("decay");   pDamp = apvts.getRawParameterValue ("damp");
    pPre = apvts.getRawParameterValue ("predelay");  pShim = apvts.getRawParameterValue ("shimmer");
    pShimInt = apvts.getRawParameterValue ("shimint");
    pModD = apvts.getRawParameterValue ("moddepth"); pModR = apvts.getRawParameterValue ("modrate");
    pTape = apvts.getRawParameterValue ("tape");     pLow = apvts.getRawParameterValue ("lowcut");
    pHigh = apvts.getRawParameterValue ("highcut");  pWidth = apvts.getRawParameterValue ("width");
    pOut = apvts.getRawParameterValue ("out");       pFreeze = apvts.getRawParameterValue ("freeze");
}

bool AetherProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet()  == juce::AudioChannelSet::stereo()
        && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void AetherProcessor::prepareToPlay (double sampleRate, int)
{
    engine.prepare (sampleRate);
}

void AetherProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    if (buffer.getNumChannels() < 2)
        return;

    aether::Params p;
    p.mix = pMix->load();           p.size = pSize->load();
    p.decay = pDecay->load();       p.damp = pDamp->load();
    p.predelay = pPre->load();      p.shimmer = pShim->load();
    p.shimInterval = (int) std::lround (pShimInt->load());
    p.modDepth = pModD->load();     p.modRate = pModR->load();
    p.tape = pTape->load();         p.lowCut = pLow->load();
    p.highCut = pHigh->load();      p.width = pWidth->load();
    p.outDb = pOut->load();         p.freeze = pFreeze->load() > 0.5f;

    engine.setParams (p);
    engine.process (buffer.getWritePointer (0), buffer.getWritePointer (1), buffer.getNumSamples());
}

//==============================================================================
int AetherProcessor::getNumPrograms() { return (int) getPresets().size(); }

const juce::String AetherProcessor::getProgramName (int index)
{
    const auto& list = getPresets();
    return juce::isPositiveAndBelow (index, (int) list.size()) ? juce::String (list[(size_t) index].name) : juce::String();
}

void AetherProcessor::setCurrentProgram (int index)
{
    const auto& list = getPresets();
    if (! juce::isPositiveAndBelow (index, (int) list.size()))
        return;

    currentProgram = index;

    for (auto* param : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (param))
            rp->setValueNotifyingHost (rp->getDefaultValue());

    for (const auto& kv : list[(size_t) index].values)
        if (auto* rp = apvts.getParameter (kv.first))
            rp->setValueNotifyingHost (rp->convertTo0to1 (kv.second));
}

//==============================================================================
void AetherProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void AetherProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto vt = juce::ValueTree::fromXml (*xml);
            currentProgram = (int) vt.getProperty ("program", 0);
            apvts.replaceState (vt);
        }
}

juce::AudioProcessorEditor* AetherProcessor::createEditor() { return new AetherEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new AetherProcessor(); }
