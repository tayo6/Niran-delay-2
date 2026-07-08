#pragma once
#include <JuceHeader.h>
#include "Parameters.h"
#include <array>
#include <atomic>

/**
    MultiTapDelay
    -------------
    A mono-in, stereo-out delay where the classic single feedback coefficient
    is replaced by an array of per-repeat ("pipe") gains, one per tap inside
    a 1-bar cycle. Each tap's contribution to the feedback loop is:

        tapGain[i] = (pipeValue[i] / 127) * feedbackPercent

    Taps are spaced evenly according to the selected sync division (or a free
    ms value when unsynced). All taps feed back into a single mono delay
    buffer, so the pattern repeats indefinitely and decays according to the
    pipe heights the user has drawn.
*/
class MultiTapDelay
{
public:
    MultiTapDelay();

    void prepare (double sampleRate, int samplesPerBlock);
    void reset();

    void setBpm (double newBpm);
    void setSyncEnabled (bool shouldSync);
    void setDivisionIndex (int index);
    void setDelayTimeMs (float ms);
    void setFeedback (float percent0to100);
    void setMix (float percent0to100);
    void setWidth (float percent0to100);
    void setLoFilterHz (float hz);
    void setHiFilterHz (float hz);

    // Pipe data -------------------------------------------------------------
    void setPipeValue (int index, float value0to127);
    float getPipeValue (int index) const;
    int getNumPipes() const noexcept { return numPipes.load(); }

    // For the GUI's real-time VU meter / playhead
    int   getCurrentPlayingPipeIndex() const noexcept { return currentPipeIndex.load(); }
    float getCurrentPlayingLevel()     const noexcept { return currentPipeLevel.load(); }
    float getBarPhase()                const noexcept { return barPhase.load(); }

    void process (juce::AudioBuffer<float>& buffer);

private:
    void recomputeTiming();
    void remapPipeValues (int oldCount, int newCount);
    void updateFilterCoefficients();

    double sampleRate = 44100.0;
    double bpm = 120.0;
    bool   syncOn = true;
    int    divisionIndex = kDefaultDivisionIndex;
    float  delayTimeMsParam = 250.0f;

    std::atomic<float> feedbackAmt { 0.5f };
    std::atomic<float> mixAmt      { 1.0f };
    std::atomic<float> widthAmt    { 1.0f };
    std::atomic<float> loFilterHz  { 20.0f };
    std::atomic<float> hiFilterHz  { 20000.0f };

    std::array<std::atomic<float>, kMaxPipes> pipeValues;
    std::atomic<int> numPipes { 4 };

    double tapSpacingSamples = 22050.0;

    juce::AudioBuffer<float> delayBuffer;
    int writePos = 0;
    int bufferLength = 0;

    // Small dedicated buffer holding only the wet (feedback) signal, used to
    // create a gentle Haas-style stereo spread driven by the WIDTH knob
    // (this is a mono delay, so there is no ping-pong; width instead offsets
    // a short copy of the wet signal between channels).
    juce::AudioBuffer<float> haasBuffer;
    int haasWritePos = 0;
    int haasLength = 0;
    static constexpr float kMaxHaasMs = 18.0f;

    juce::dsp::IIR::Filter<float> loFilterProc, hiFilterProc;
    juce::SmoothedValue<float> smoothedFeedback, smoothedMix, smoothedWidth;

    int64_t sampleCounter = 0;
    std::atomic<int>   currentPipeIndex { 0 };
    std::atomic<float> currentPipeLevel { 0.0f };
    std::atomic<float> barPhase { 0.0f };

    juce::CriticalSection timingLock;
};
