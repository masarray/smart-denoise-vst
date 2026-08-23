#pragma once

#include "../DSP/SmartDenoiseEngine.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <atomic>

class SmartDenoiseAudioProcessor final
    : public juce::AudioProcessor
{
public:
    SmartDenoiseAudioProcessor();
    ~SmartDenoiseAudioProcessor() override = default;

    void prepareToPlay (double sampleRate,
                        int samplesPerBlock) override;
    void releaseResources() override {}

    bool isBusesLayoutSupported (
        const BusesLayout& layouts) const override;

    void processBlock (
        juce::AudioBuffer<float>&,
        juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override
    {
        return JucePlugin_Name;
    }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (
        int, const juce::String&) override {}

    void getStateInformation (
        juce::MemoryBlock& destData) override;
    void setStateInformation (
        const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getParameters()
    {
        return parameters;
    }

    smartdenoise::SmartDenoiseEngine& getEngine()
    {
        return engine;
    }

    const smartdenoise::SmartDenoiseEngine& getEngine() const
    {
        return engine;
    }

    void startNoiseLearn()
    {
        engine.startLearning (3.0);
    }

    float getInputPeakDb() const noexcept
    {
        return inputPeakDb.load (std::memory_order_relaxed);
    }

    float getOutputPeakDb() const noexcept
    {
        return outputPeakDb.load (std::memory_order_relaxed);
    }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout
        createParameterLayout();

    void applyParametersToEngine();

    smartdenoise::SmartDenoiseEngine engine;

    juce::AudioProcessorValueTreeState parameters;

    juce::String pendingProfile;
    int lastReportedLatency = -1;

    std::atomic<float> inputPeakDb { -72.0f };
    std::atomic<float> outputPeakDb { -72.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (
        SmartDenoiseAudioProcessor)
};
