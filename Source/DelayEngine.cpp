#include "DelayEngine.h"

MultiTapDelay::MultiTapDelay()
{
    for (auto& v : pipeValues)
        v.store (64.0f);

    // Default pattern for the initial 4 pipes (1/4 sync), matching the reference design.
    pipeValues[0].store (48.0f);
    pipeValues[1].store (84.0f);
    pipeValues[2].store (127.0f);
    pipeValues[3].store (64.0f);
}

void MultiTapDelay::prepare (double newSampleRate, int samplesPerBlock)
{
    sampleRate = newSampleRate;

    smoothedFeedback.reset (sampleRate, 0.02);
    smoothedMix.reset (sampleRate, 0.02);
    smoothedWidth.reset (sampleRate, 0.02);

    loFilterProc.reset();
    hiFilterProc.reset();

    haasLength = (int) std::ceil (sampleRate * kMaxHaasMs / 1000.0) + 8;
    haasBuffer.setSize (1, haasLength, false, true, true);
    haasWritePos = 0;

    bufferLength = 0;
    recomputeTiming();
    reset();

    juce::ignoreUnused (samplesPerBlock);
}

void MultiTapDelay::reset()
{
    delayBuffer.clear();
    haasBuffer.clear();
    writePos = 0;
    haasWritePos = 0;
    sampleCounter = 0;
    smoothedFeedback.setCurrentAndTargetValue (feedbackAmt.load() / 100.0f);
    smoothedMix.setCurrentAndTargetValue (mixAmt.load() / 100.0f);
    smoothedWidth.setCurrentAndTargetValue (widthAmt.load() / 100.0f);
}

void MultiTapDelay::setBpm (double newBpm)
{
    bpm = juce::jlimit (10.0, 400.0, newBpm);
}

void MultiTapDelay::setSyncEnabled (bool shouldSync) { syncOn = shouldSync; }
void MultiTapDelay::setDivisionIndex (int index) { divisionIndex = juce::jlimit (0, (int) kDivisionTable.size() - 1, index); }
void MultiTapDelay::setDelayTimeMs (float ms) { delayTimeMsParam = ms; }
void MultiTapDelay::setFeedback (float percent0to100) { feedbackAmt.store (percent0to100); }
void MultiTapDelay::setMix (float percent0to100) { mixAmt.store (percent0to100); }
void MultiTapDelay::setWidth (float percent0to100) { widthAmt.store (percent0to100); }
void MultiTapDelay::setLoFilterHz (float hz) { loFilterHz.store (hz); }
void MultiTapDelay::setHiFilterHz (float hz) { hiFilterHz.store (hz); }

void MultiTapDelay::setPipeValue (int index, float value0to127)
{
    if (juce::isPositiveAndBelow (index, kMaxPipes))
        pipeValues[(size_t) index].store (juce::jlimit (0.0f, 127.0f, value0to127));
}

float MultiTapDelay::getPipeValue (int index) const
{
    if (juce::isPositiveAndBelow (index, kMaxPipes))
        return pipeValues[(size_t) index].load();
    return 0.0f;
}

void MultiTapDelay::remapPipeValues (int oldCount, int newCount)
{
    if (oldCount == newCount || oldCount <= 0)
        return;

    std::array<float, kMaxPipes> oldVals {};
    for (int i = 0; i < oldCount && i < kMaxPipes; ++i)
        oldVals[(size_t) i] = pipeValues[(size_t) i].load();

    if (newCount < oldCount)
    {
        // Shrinking: average each group of old pipes into one new pipe.
        double groupSize = (double) oldCount / (double) newCount;
        for (int i = 0; i < newCount; ++i)
        {
            int start = (int) std::floor (i * groupSize);
            int end   = juce::jmax (start + 1, (int) std::floor ((i + 1) * groupSize));
            end = juce::jmin (end, oldCount);

            double sum = 0.0;
            int n = 0;
            for (int j = start; j < end; ++j) { sum += oldVals[(size_t) j]; ++n; }

            pipeValues[(size_t) i].store (n > 0 ? (float) (sum / n) : 64.0f);
        }
    }
    else
    {
        // Growing: interpolate between the nearest existing pipes.
        for (int i = 0; i < newCount; ++i)
        {
            double srcPos = (oldCount > 1)
                ? (double) i * (double) (oldCount - 1) / (double) juce::jmax (1, newCount - 1)
                : 0.0;

            int i0 = juce::jlimit (0, oldCount - 1, (int) std::floor (srcPos));
            int i1 = juce::jlimit (0, oldCount - 1, i0 + 1);
            float frac = (float) (srcPos - std::floor (srcPos));

            float v0 = oldVals[(size_t) i0];
            float v1 = oldVals[(size_t) i1];
            pipeValues[(size_t) i].store (juce::jmap (frac, 0.0f, 1.0f, v0, v1));
        }
    }
}

void MultiTapDelay::recomputeTiming()
{
    const juce::ScopedLock sl (timingLock);

    const double barLengthMs = (60000.0 / juce::jmax (1.0, bpm)) * 4.0;

    double newTapSpacingSamples;
    int newNumPipes;

    if (syncOn)
    {
        const auto& info = kDivisionTable[(size_t) juce::jlimit (0, (int) kDivisionTable.size() - 1, divisionIndex)];
        double beatMs = 60000.0 / juce::jmax (1.0, bpm);
        double repeatMs = info.beats * beatMs;
        newTapSpacingSamples = juce::jmax (1.0, repeatMs * sampleRate / 1000.0);
        newNumPipes = info.numPipes;
    }
    else
    {
        double delayMs = juce::jlimit (1.0, 2000.0, (double) delayTimeMsParam);
        newTapSpacingSamples = juce::jmax (1.0, delayMs * sampleRate / 1000.0);
        newNumPipes = (int) std::floor (barLengthMs / delayMs);
    }

    newNumPipes = juce::jlimit (1, kMaxPipes, newNumPipes);

    // Keep the total buffer span within a sane bound (30 seconds) to avoid runaway allocations
    // at extreme tempo / division combinations.
    const double maxSpanSamples = 30.0 * sampleRate;
    while (newNumPipes > 1 && newTapSpacingSamples * newNumPipes > maxSpanSamples)
        --newNumPipes;

    const int oldNumPipes = numPipes.load();
    if (newNumPipes != oldNumPipes)
    {
        remapPipeValues (oldNumPipes, newNumPipes);
        numPipes.store (newNumPipes);
    }

    tapSpacingSamples = newTapSpacingSamples;

    int requiredLength = (int) std::ceil (tapSpacingSamples * newNumPipes) + 8;
    requiredLength = juce::jmax (requiredLength, (int) (sampleRate * 0.1));

    if (requiredLength != bufferLength)
    {
        bufferLength = requiredLength;
        delayBuffer.setSize (1, bufferLength, true, true, true);
        delayBuffer.clear();
        writePos = 0;
    }
}

void MultiTapDelay::updateFilterCoefficients()
{
    auto lo = juce::dsp::IIR::Coefficients<float>::makeHighPass (sampleRate, juce::jlimit (20.0f, 20000.0f, loFilterHz.load()));
    auto hi = juce::dsp::IIR::Coefficients<float>::makeLowPass  (sampleRate, juce::jlimit (20.0f, 20000.0f, hiFilterHz.load()));

    loFilterProc.coefficients = lo;
    hiFilterProc.coefficients = hi;
}

void MultiTapDelay::process (juce::AudioBuffer<float>& buffer)
{
    recomputeTiming();
    updateFilterCoefficients();

    const int numSamples = buffer.getNumSamples();
    const int numCh = buffer.getNumChannels();
    if (numSamples <= 0 || bufferLength <= 0)
        return;

    auto* delayData = delayBuffer.getWritePointer (0);
    auto* haasData = haasBuffer.getWritePointer (0);
    const int N = numPipes.load();
    const double spacing = juce::jmax (1.0, tapSpacingSamples);
    const double totalCycleSamples = spacing * (double) N;

    float* left  = buffer.getWritePointer (0);
    float* right = numCh > 1 ? buffer.getWritePointer (1) : nullptr;

    for (int n = 0; n < numSamples; ++n)
    {
        float dryL = left[n];
        float dryR = right != nullptr ? right[n] : dryL;
        float monoIn = 0.5f * (dryL + dryR);

        float curFb    = smoothedFeedback.getNextValue();
        float curMix   = smoothedMix.getNextValue();
        float curWidth = smoothedWidth.getNextValue();

        float tapSum = 0.0f;

        for (int i = 0; i < N; ++i)
        {
            double delaySamples = spacing * (double) (i + 1);
            double readPosD = (double) writePos - delaySamples;
            while (readPosD < 0.0)
                readPosD += bufferLength;

            int r0 = ((int) readPosD) % bufferLength;
            int r1 = (r0 + 1) % bufferLength;
            float frac = (float) (readPosD - std::floor (readPosD));

            float s0 = delayData[r0];
            float s1 = delayData[r1];
            float sample = s0 + frac * (s1 - s0);

            float pv = pipeValues[(size_t) i].load();
            float gain = (pv / 127.0f) * curFb;
            tapSum += sample * gain;
        }

        float filtered = hiFilterProc.processSample (loFilterProc.processSample (tapSum));

        delayData[writePos] = monoIn + filtered;
        writePos = (writePos + 1) % bufferLength;

        // Store the wet-only signal in a short history buffer, then read a
        // width-scaled short offset from it to create a gentle Haas stereo
        // spread (this is a mono delay, so there is no hard L/R panning).
        haasData[haasWritePos] = filtered;
        double haasOffsetSamples = (double) curWidth * (kMaxHaasMs * sampleRate / 1000.0);
        double haasReadD = (double) haasWritePos - haasOffsetSamples;
        while (haasReadD < 0.0)
            haasReadD += haasLength;
        int h0 = ((int) haasReadD) % haasLength;
        int h1 = (h0 + 1) % haasLength;
        float hFrac = (float) (haasReadD - std::floor (haasReadD));
        float wetR = haasData[h0] + hFrac * (haasData[h1] - haasData[h0]);
        haasWritePos = (haasWritePos + 1) % haasLength;

        float wetL = filtered;

        left[n] = dryL * (1.0f - curMix) + wetL * curMix;
        if (right != nullptr)
            right[n] = dryR * (1.0f - curMix) + wetR * curMix;

        ++sampleCounter;
        double phaseSamples = std::fmod ((double) sampleCounter, totalCycleSamples > 0.0 ? totalCycleSamples : 1.0);
        int idx = juce::jlimit (0, juce::jmax (0, N - 1), (int) std::floor (phaseSamples / spacing));
        currentPipeIndex.store (idx);
        currentPipeLevel.store (pipeValues[(size_t) idx].load() / 127.0f);
        barPhase.store ((float) (phaseSamples / (totalCycleSamples > 0.0 ? totalCycleSamples : 1.0)));
    }
}
