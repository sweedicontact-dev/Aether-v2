#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Dsp.h"

using APVTS = juce::AudioProcessorValueTreeState;

class AetherProcessor : public juce::AudioProcessor
{
public:
    AetherProcessor();
    ~AetherProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 60.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    APVTS apvts;

private:
    static APVTS::ParameterLayout createLayout();

    aether::Engine engine;
    int currentProgram = 0;

    std::atomic<float> *pMix, *pSize, *pDecay, *pDamp, *pPre, *pShim, *pShimInt, *pModD, *pModR,
                       *pTape, *pLow, *pHigh, *pWidth, *pOut, *pFreeze;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AetherProcessor)
};
