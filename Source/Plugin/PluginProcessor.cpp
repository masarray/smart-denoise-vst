#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <memory>
#include <vector>

SmartDenoiseAudioProcessor::SmartDenoiseAudioProcessor()
    : juce::AudioProcessor (
          BusesProperties()
              .withInput (
                  "Input",
                  juce::AudioChannelSet::stereo(),
                  true)
              .withOutput (
                  "Output",
                  juce::AudioChannelSet::stereo(),
                  true)),
      parameters (
          *this,
          nullptr,
          "PARAMETERS",
          createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout
SmartDenoiseAudioProcessor::createParameterLayout()
{
    using APF = juce::AudioParameterFloat;
    using APB = juce::AudioParameterBool;
    using APC = juce::AudioParameterChoice;

    std::vector<
        std::unique_ptr<juce::RangedAudioParameter>>
        result;

    result.push_back (
        std::make_unique<APB> (
            "enabled",
            "Denoise",
            true));

    result.push_back (
        std::make_unique<APF> (
            "reduction",
            "Reduction",
            juce::NormalisableRange<float> (
                0.0f,
                smartdenoise::SmartDenoiseEngine::
                    maxReductionDb,
                0.1f),
            8.0f,
            juce::AudioParameterFloatAttributes()
                .withLabel ("dB")));

    result.push_back (
        std::make_unique<APF> (
            "preserve",
            "Preserve",
            juce::NormalisableRange<float> (
                0.0f, 1.0f, 0.01f),
            0.75f));

    result.push_back (
        std::make_unique<APF> (
            "silence",
            "Silence",
            juce::NormalisableRange<float> (
                0.0f, 1.0f, 0.01f),
            0.55f));

    result.push_back (
        std::make_unique<APF> (
            "thresholdOffset",
            "Profile Offset",
            juce::NormalisableRange<float> (
                -6.0f, 12.0f, 0.1f),
            1.5f,
            juce::AudioParameterFloatAttributes()
                .withLabel ("dB")));

    result.push_back (
        std::make_unique<APC> (
            "quality",
            "Quality",
            juce::StringArray {
                "Live - FFT 1024",
                "Clean - FFT 2048"
            },
            0));

    result.push_back (
        std::make_unique<APB> (
            "hearRemoved",
            "Hear Removed",
            false));

    return { result.begin(), result.end() };
}

bool SmartDenoiseAudioProcessor::isBusesLayoutSupported (
    const BusesLayout& layouts) const
{
    const auto input =
        layouts.getMainInputChannelSet();

    const auto output =
        layouts.getMainOutputChannelSet();

    if (input != output)
        return false;

    return input == juce::AudioChannelSet::mono()
        || input == juce::AudioChannelSet::stereo();
}

void SmartDenoiseAudioProcessor::applyParametersToEngine()
{
    engine.setEnabled (
        parameters.getRawParameterValue (
            "enabled")->load() >= 0.5f);

    engine.setReductionDb (
        parameters.getRawParameterValue (
            "reduction")->load());

    engine.setPreserve (
        parameters.getRawParameterValue (
            "preserve")->load());

    engine.setSilenceAmount (
        parameters.getRawParameterValue (
            "silence")->load());

    engine.setThresholdOffsetDb (
        parameters.getRawParameterValue (
            "thresholdOffset")->load());

    engine.setHearRemoved (
        parameters.getRawParameterValue (
            "hearRemoved")->load() >= 0.5f);

    const auto qualityIndex =
        static_cast<int> (
            parameters.getRawParameterValue (
                "quality")->load());

    engine.setQuality (
        qualityIndex == 1
            ? smartdenoise::SmartDenoiseEngine::
                Quality::clean2048
            : smartdenoise::SmartDenoiseEngine::
                Quality::live1024);
}

void SmartDenoiseAudioProcessor::prepareToPlay (
    double sampleRate,
    int samplesPerBlock)
{
    const int channels =
        juce::jmax (
            1,
            getTotalNumInputChannels());

    applyParametersToEngine();

    engine.prepare (
        sampleRate,
        samplesPerBlock,
        channels);

    if (pendingProfile.isNotEmpty())
    {
        engine.restoreProfile (pendingProfile);
        pendingProfile.clear();
    }

    lastReportedLatency =
        engine.getLatencySamples();

    setLatencySamples (
        lastReportedLatency);
}

void SmartDenoiseAudioProcessor::processBlock (
    juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int inputChannels =
        getTotalNumInputChannels();

    for (int channel = inputChannels;
         channel < buffer.getNumChannels();
         ++channel)
    {
        buffer.clear (
            channel,
            0,
            buffer.getNumSamples());
    }

    applyParametersToEngine();
    engine.process (buffer);

    const int latency =
        engine.getLatencySamples();

    if (latency != lastReportedLatency)
    {
        lastReportedLatency = latency;
        setLatencySamples (latency);
    }
}

void SmartDenoiseAudioProcessor::getStateInformation (
    juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();

    const auto profile =
        engine.serialiseProfile();

    if (profile.isNotEmpty())
        state.setProperty (
            "noiseProfile",
            profile,
            nullptr);

    std::unique_ptr<juce::XmlElement> xml (
        state.createXml());

    copyXmlToBinary (
        *xml,
        destData);
}

void SmartDenoiseAudioProcessor::setStateInformation (
    const void* data,
    int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (
        getXmlFromBinary (
            data,
            sizeInBytes));

    if (xml == nullptr)
        return;

    const auto state =
        juce::ValueTree::fromXml (*xml);

    if (! state.isValid())
        return;

    pendingProfile =
        state.getProperty (
            "noiseProfile").toString();

    auto parameterOnly = state.createCopy();
    parameterOnly.removeProperty (
        "noiseProfile",
        nullptr);

    parameters.replaceState (
        parameterOnly);
}

juce::AudioProcessorEditor*
SmartDenoiseAudioProcessor::createEditor()
{
    return new SmartDenoiseAudioProcessorEditor (*this);
}

juce::AudioProcessor*
JUCE_CALLTYPE createPluginFilter()
{
    return new SmartDenoiseAudioProcessor();
}
