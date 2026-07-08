#include "PluginProcessor.h"
#include "PluginEditor.h"

HadenianMonoDelayAudioProcessor::HadenianMonoDelayAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    syncParam     = apvts.getRawParameterValue (ParamIDs::sync);
    divisionParam = apvts.getRawParameterValue (ParamIDs::division);
    delayMsParam  = apvts.getRawParameterValue (ParamIDs::delayMs);
    feedbackParam = apvts.getRawParameterValue (ParamIDs::feedback);
    mixParam      = apvts.getRawParameterValue (ParamIDs::mix);
    loFilterParam = apvts.getRawParameterValue (ParamIDs::loFilter);
    hiFilterParam = apvts.getRawParameterValue (ParamIDs::hiFilter);
    widthParam    = apvts.getRawParameterValue (ParamIDs::width);
}

HadenianMonoDelayAudioProcessor::~HadenianMonoDelayAudioProcessor() {}

void HadenianMonoDelayAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock);
    updateEngineFromParameters();
}

void HadenianMonoDelayAudioProcessor::releaseResources() {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool HadenianMonoDelayAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto mono = juce::AudioChannelSet::mono();
    const auto stereo = juce::AudioChannelSet::stereo();
    const auto in = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();

    if (out != mono && out != stereo)
        return false;
    if (in != mono && in != stereo)
        return false;

    return true;
}
#endif

void HadenianMonoDelayAudioProcessor::updateEngineFromParameters()
{
    if (auto* ph = getPlayHead())
    {
        if (auto pos = ph->getPosition())
        {
            if (auto bpmOpt = pos->getBpm())
                currentBpm.store (*bpmOpt);
        }
    }

    engine.setBpm (currentBpm.load());
    engine.setSyncEnabled (syncParam != nullptr && syncParam->load() > 0.5f);
    engine.setDivisionIndex (divisionParam != nullptr ? (int) divisionParam->load() : kDefaultDivisionIndex);
    engine.setDelayTimeMs (delayMsParam != nullptr ? delayMsParam->load() : 250.0f);
    engine.setFeedback (feedbackParam != nullptr ? feedbackParam->load() : 50.0f);
    engine.setMix (mixParam != nullptr ? mixParam->load() : 100.0f);
    engine.setLoFilterHz (loFilterParam != nullptr ? loFilterParam->load() : 20.0f);
    engine.setHiFilterHz (hiFilterParam != nullptr ? hiFilterParam->load() : 20000.0f);
    engine.setWidth (widthParam != nullptr ? widthParam->load() : 100.0f);
}

void HadenianMonoDelayAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    midiMessages.clear();

    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    updateEngineFromParameters();
    engine.process (buffer);
}

juce::AudioProcessorEditor* HadenianMonoDelayAudioProcessor::createEditor()
{
    return new HadenianMonoDelayAudioProcessorEditor (*this);
}

void HadenianMonoDelayAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    auto xml = state.createXml();

    auto* pipesXml = xml->createNewChildElement ("PIPES");
    int numPipes = engine.getNumPipes();
    pipesXml->setAttribute ("count", numPipes);

    juce::String csv;
    for (int i = 0; i < numPipes; ++i)
    {
        if (i > 0) csv << ",";
        csv << engine.getPipeValue (i);
    }
    pipesXml->setAttribute ("values", csv);

    copyXmlToBinary (*xml, destData);
}

void HadenianMonoDelayAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml == nullptr)
        return;

    if (xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));

    if (auto* pipesXml = xml->getChildByName ("PIPES"))
    {
        int count = pipesXml->getIntAttribute ("count", 4);
        juce::String csv = pipesXml->getStringAttribute ("values");
        juce::StringArray tokens;
        tokens.addTokens (csv, ",", "");

        for (int i = 0; i < count && i < tokens.size(); ++i)
            engine.setPipeValue (i, tokens[i].getFloatValue());
    }

    updateEngineFromParameters();
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HadenianMonoDelayAudioProcessor();
}
