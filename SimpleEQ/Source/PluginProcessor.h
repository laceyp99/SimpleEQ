/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
*/
class SimpleEQAudioProcessor  : public juce::AudioProcessor, juce::AudioProcessorValueTreeState::Listener
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif
{
public:
    //==============================================================================
    SimpleEQAudioProcessor();
    ~SimpleEQAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    void updateparameters();

    //==============================================================================
    // Value Trees
    juce::AudioProcessorValueTreeState treestate;

private:

    juce::dsp::ProcessorDuplicator<juce::dsp::StateVariableFilter::Filter<float>, juce::dsp::StateVariableFilter::Parameters<float>> highpassFilter;
    juce::dsp::ProcessorDuplicator<juce::dsp::StateVariableFilter::Filter<float>, juce::dsp::StateVariableFilter::Parameters<float>> lowpassFilter;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> peakFilter;

    float outputrawGain = 1.0;
    float inputrawGain = 1.0;
    float lowcutfreq = 20.0;
    float highcutfreq = 20000.0;
    float peakfreq = 3000.0;
    float peakgain = 3.0;
    float peakq = 0.5;

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void parameterChanged(const juce::String& parameterID, float newValue);
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleEQAudioProcessor)
};
