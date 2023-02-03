/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SimpleEQAudioProcessor::SimpleEQAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ), treestate(*this, nullptr, "PARMETERS", createParameterLayout()), highpassFilter(), lowpassFilter(),
    peakFilter(juce::dsp::IIR::Coefficients <float>::makePeakFilter(getSampleRate(), 3000.0, 0.5, 1.0))
#endif
{
    treestate.addParameterListener("input gain", this);
    treestate.addParameterListener("output gain", this);
    treestate.addParameterListener("low cut", this);
    treestate.addParameterListener("high cut", this);
    treestate.addParameterListener("peak freq", this);
    treestate.addParameterListener("peak gain", this);
    treestate.addParameterListener("peak q", this);
}

SimpleEQAudioProcessor::~SimpleEQAudioProcessor()
{
    treestate.removeParameterListener("input gain", this);
    treestate.removeParameterListener("output gain", this);
    treestate.removeParameterListener("low cut", this);
    treestate.removeParameterListener("high cut", this);
    treestate.removeParameterListener("peak freq", this);
    treestate.removeParameterListener("peak gain", this);
    treestate.removeParameterListener("peak q", this);
}

juce::AudioProcessorValueTreeState::ParameterLayout SimpleEQAudioProcessor::createParameterLayout()
{
    std::vector < std::unique_ptr<juce::RangedAudioParameter >> params;

    auto pInputGain = std::make_unique<juce::AudioParameterFloat>("input gain", "Input Gain", -24.0, 24.0, 0.0);
    auto pOutputGain = std::make_unique<juce::AudioParameterFloat>("output gain", "Output Gain", -24.0, 24.0, 0.0);

    auto pLowCut = std::make_unique<juce::AudioParameterFloat>("low cut", "Low Cut", 20.0, 1000.0, 20.0);
    auto pHighCut = std::make_unique<juce::AudioParameterFloat>("high cut", "High Cut", 1000.0, 20000.0, 20000.0);
    auto pPeakFreq = std::make_unique<juce::AudioParameterFloat>("peak freq", "Peak Freq", 20.0, 20000.0, 3000.0);
    auto pPeakGain = std::make_unique<juce::AudioParameterFloat>("peak gain", "Peak Gain", 0.1, 5.0, 1.0);
    auto pPeakQ = std::make_unique<juce::AudioParameterFloat>("peak q", "Peak Q", 0.5, 10.0, 0.5);

    params.push_back(std::move(pInputGain));
    params.push_back(std::move(pOutputGain));

    params.push_back(std::move(pLowCut));
    params.push_back(std::move(pHighCut));
    params.push_back(std::move(pPeakFreq));
    params.push_back(std::move(pPeakGain));
    params.push_back(std::move(pPeakQ));

    return { params.begin(), params.end() };

}

void SimpleEQAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{

    if (parameterID == "input gain")
    {
        inputrawGain = juce::Decibels::decibelsToGain(newValue);
        //DBG("Input Gain is " << newValue);
    }
    if (parameterID == "output gain")
    {
        outputrawGain = juce::Decibels::decibelsToGain(newValue);
        //DBG("Output Gain is " << newValue);
    }
    if (parameterID == "prelow cut")
    {
        lowcutfreq = newValue;
    }
    if (parameterID == "prehigh cut")
    {
        highcutfreq = newValue;
    }
    if (parameterID == "prepeak freq")
    {
        peakfreq = newValue;
    }
    if (parameterID == "prepeak gain")
    {
        peakgain = newValue;
    }
    if (parameterID == "prepeak q")
    {
        peakq = newValue;
    }
}

//==============================================================================
const juce::String SimpleEQAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SimpleEQAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SimpleEQAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SimpleEQAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SimpleEQAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SimpleEQAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SimpleEQAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SimpleEQAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String SimpleEQAudioProcessor::getProgramName (int index)
{
    return {};
}

void SimpleEQAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void SimpleEQAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..

    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();
    spec.sampleRate = sampleRate;


    //satdistortion.reset();
    highpassFilter.prepare(spec);
    lowpassFilter.prepare(spec);
    peakFilter.prepare(spec);
    
    highpassFilter.reset();
    lowpassFilter.reset();
    peakFilter.reset();
    
    lowcutfreq = *treestate.getRawParameterValue("low cut");
    highcutfreq = *treestate.getRawParameterValue("high cut");
    peakfreq = *treestate.getRawParameterValue("peak freq");
    peakq = *treestate.getRawParameterValue("peak q");
    peakgain = *treestate.getRawParameterValue("peak gain");

    inputrawGain = juce::Decibels::decibelsToGain(static_cast<float>(*treestate.getRawParameterValue("input gain")));
    outputrawGain = juce::Decibels::decibelsToGain(static_cast<float>(*treestate.getRawParameterValue("output gain")));

    updateparameters();

}

void SimpleEQAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SimpleEQAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void SimpleEQAudioProcessor::updateparameters()
{
    float hpcutoff = *treestate.getRawParameterValue("low cut");
    float hpres = 1.0;
    highpassFilter.state->type = juce::dsp::StateVariableFilter::Parameters<float>::Type::highPass;
    highpassFilter.state->setCutOffFrequency(getSampleRate(), hpcutoff, hpres);

    float lpcutoff = *treestate.getRawParameterValue("high cut");
    float lpres = 1.0;
    lowpassFilter.state->type = juce::dsp::StateVariableFilter::Parameters<float>::Type::lowPass;
    lowpassFilter.state->setCutOffFrequency(getSampleRate(), lpcutoff, lpres);

    float pfreq = *treestate.getRawParameterValue("peak freq");
    float pQ = *treestate.getRawParameterValue("peak q");
    float pgain = *treestate.getRawParameterValue("peak gain"); // gain factor not dB
    *peakFilter.state = *juce::dsp::IIR::Coefficients <float>::makePeakFilter(getSampleRate(), pfreq, pQ, pgain);
}

void SimpleEQAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // my audio block object
    juce::dsp::AudioBlock<float> block(buffer);


    // for loops grabbing all the samples from each channel
    for (int inchannel = 0; inchannel < block.getNumChannels(); ++inchannel)
    {
        auto* inchannelData = block.getChannelPointer(inchannel);
        for (int insample = 0; insample < block.getNumSamples(); ++insample)
        {
            inchannelData[insample] *= inputrawGain;
        }
    }

    updateparameters();
    highpassFilter.process(juce::dsp::ProcessContextReplacing<float>(block));
    lowpassFilter.process(juce::dsp::ProcessContextReplacing<float>(block));
    peakFilter.process(juce::dsp::ProcessContextReplacing<float>(block));

    for (int outchannel = 0; outchannel < block.getNumChannels(); ++outchannel)
    {
        auto* outchannelData = block.getChannelPointer(outchannel);
        for (int outsample = 0; outsample < block.getNumSamples(); ++outsample)
        {
            outchannelData[outsample] *= outputrawGain;            
        }

    }
}

//==============================================================================
bool SimpleEQAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SimpleEQAudioProcessor::createEditor()
{
    // return new SimpleEQAudioProcessorEditor (*this);
    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void SimpleEQAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    juce::MemoryOutputStream stream(destData, false);
    treestate.state.writeToStream(stream);
}

void SimpleEQAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    auto tree = juce::ValueTree::readFromData(data, size_t(sizeInBytes));

    if (tree.isValid())
    {
        treestate.state = tree;
        
        lowcutfreq = *treestate.getRawParameterValue("prelow cut");
        highcutfreq = *treestate.getRawParameterValue("prehigh cut");
        peakfreq = *treestate.getRawParameterValue("prepeak freq");
        peakgain = *treestate.getRawParameterValue("prepeak gain");
        peakq = *treestate.getRawParameterValue("prepeak q");
        inputrawGain = juce::Decibels::decibelsToGain(static_cast<float>(*treestate.getRawParameterValue("input gain")));
        outputrawGain = juce::Decibels::decibelsToGain(static_cast<float>(*treestate.getRawParameterValue("output gain")));
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SimpleEQAudioProcessor();
}
