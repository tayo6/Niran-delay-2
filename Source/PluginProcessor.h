#pragma once
#include <JuceHeader.h>
#include "Parameters.h"
#include "DelayEngine.h"

class HadenianMonoDelayAudioProcessor : public juce::AudioProcessor
{
public:
    HadenianMonoDelayAudioProcessor();
    ~HadenianMonoDelayAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==============================================================================
    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    //==============================================================================
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    // Public interface used by the editor
    juce::AudioProcessorValueTreeState apvts;

    MultiTapDelay& getEngine() noexcept { return engine; }

    int   getNumPipes() const noexcept { return engine.getNumPipes(); }
    float getPipeValue (int index) const noexcept { return engine.getPipeValue (index); }
    void  setPipeValue (int index, float value0to127) { engine.setPipeValue (index, value0to127); }

    int   getCurrentPlayingPipeIndex() const noexcept { return engine.getCurrentPlayingPipeIndex(); }
    float getCurrentPlayingLevel()     const noexcept { return engine.getCurrentPlayingLevel(); }
    float getBarPhase()                const noexcept { return engine.getBarPhase(); }

    double getCurrentBpm() const noexcept { return currentBpm.load(); }

private:
    void updateEngineFromParameters();

    MultiTapDelay engine;
    std::atomic<double> currentBpm { 120.0 };

    std::atomic<float>* syncParam     = nullptr;
    std::atomic<float>* divisionParam = nullptr;
    std::atomic<float>* delayMsParam  = nullptr;
    std::atomic<float>* feedbackParam = nullptr;
    std::atomic<float>* mixParam      = nullptr;
    std::atomic<float>* loFilterParam = nullptr;
    std::atomic<float>* hiFilterParam = nullptr;
    std::atomic<float>* widthParam    = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HadenianMonoDelayAudioProcessor)
};
