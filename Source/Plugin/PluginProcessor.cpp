#include "PluginProcessor.h"
#ifndef SMART_DENOISE_HEADLESS_PROCESSOR_TEST
#include "PluginEditor.h"
#endif

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

    inputPeakDb.store (-72.0f, std::memory_order_relaxed);
    outputPeakDb.store (-72.0f, std::memory_order_relaxed);
}

void SmartDenoiseAudioProcessor::processBlock (
    juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int inputChannels =
        getTotalNumInputChannels();

    const int samples = buffer.getNumSamples();

    float inputPeak = 0.0f;
    for (int channel = 0;
         channel < juce::jmin (inputChannels, buffer.getNumChannels());
         ++channel)
    {
        inputPeak = juce::jmax (
            inputPeak,
            buffer.getMagnitude (channel, 0, samples));
    }

    inputPeakDb.store (
        juce::Decibels::gainToDecibels (
            inputPeak,
            -72.0f),
        std::memory_order_relaxed);

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

    float outputPeak = 0.0f;
    for (int channel = 0;
         channel < juce::jmin (inputChannels, buffer.getNumChannels());
         ++channel)
    {
        outputPeak = juce::jmax (
            outputPeak,
            buffer.getMagnitude (channel, 0, samples));
    }

    outputPeakDb.store (
        juce::Decibels::gainToDecibels (
            outputPeak,
            -72.0f),
        std::memory_order_relaxed);

    const int latency =
        engine.getLatencySamples();

    if (latency != lastReportedLatency)
    {
        lastReportedLatency = latency;
        setLatencySamples (latency);
    }
}

juce::File SmartDenoiseAudioProcessor::getCapturedPresetDirectory() const
{
    const auto testOverride = juce::SystemStats::getEnvironmentVariable (
        "SMART_DENOISE_PROFILE_BANK_DIR", {});

    if (testOverride.isNotEmpty())
        return juce::File (testOverride);

    return juce::File::getSpecialLocation (
               juce::File::userApplicationDataDirectory)
        .getChildFile ("Masarray")
        .getChildFile ("Smart Denoise")
        .getChildFile ("Captured Profiles");
}

void SmartDenoiseAudioProcessor::setRawParameterValue (
    const juce::String& parameterId,
    float rawValue)
{
    if (auto* parameter = parameters.getParameter (parameterId))
    {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (rawValue));
        parameter->endChangeGesture();
    }
}

juce::StringArray SmartDenoiseAudioProcessor::getCapturedProfilePresetNames() const
{
    juce::StringArray names;
    const auto directory = getCapturedPresetDirectory();
    if (! directory.isDirectory())
        return names;

    juce::Array<juce::File> files;
    directory.findChildFiles (
        files, juce::File::findFiles, false, "*.sdprofile");

    const int currentQuality = static_cast<int> (
        parameters.getRawParameterValue ("quality")->load());

    for (const auto& file : files)
    {
        auto xml = juce::XmlDocument::parse (file);
        if (xml == nullptr)
            continue;

        const auto preset = juce::ValueTree::fromXml (*xml);
        if (! preset.isValid())
            continue;

        if (static_cast<int> (preset.getProperty ("qualityMode", -1))
            != currentQuality)
            continue;

        names.addIfNotAlreadyThere (file.getFileNameWithoutExtension());
    }

    names.sort (true);
    return names;
}

juce::String SmartDenoiseAudioProcessor::saveCapturedProfilePreset()
{
    if (! engine.hasProfile())
        return {};

    const auto encodedProfile = engine.serialiseProfile();
    if (encodedProfile.isEmpty())
        return {};

    const auto directory = getCapturedPresetDirectory();
    if (! directory.createDirectory())
        return {};

    const auto presetName = juce::Time::getCurrentTime().formatted (
        "Capture %Y-%m-%d %H-%M-%S");

    juce::ValueTree preset ("SMART_DENOISE_CAPTURE");
    preset.setProperty ("qualityMode",
        static_cast<int> (parameters.getRawParameterValue ("quality")->load()), nullptr);
    preset.setProperty ("reduction",
        parameters.getRawParameterValue ("reduction")->load(), nullptr);
    preset.setProperty ("preserve",
        parameters.getRawParameterValue ("preserve")->load(), nullptr);
    preset.setProperty ("silence",
        parameters.getRawParameterValue ("silence")->load(), nullptr);
    preset.setProperty ("thresholdOffset",
        parameters.getRawParameterValue ("thresholdOffset")->load(), nullptr);
    preset.setProperty ("profileQuality", engine.getProfileQuality(), nullptr);
    preset.setProperty ("noiseProfile", encodedProfile, nullptr);

    const auto file = directory.getChildFile (presetName + ".sdprofile");
    auto xml = preset.createXml();
    if (xml == nullptr || ! xml->writeTo (file))
        return {};

    return presetName;
}

bool SmartDenoiseAudioProcessor::loadCapturedProfilePreset (
    const juce::String& presetName)
{
    if (presetName.isEmpty())
        return false;

    const auto file = getCapturedPresetDirectory()
        .getChildFile (presetName + ".sdprofile");
    if (! file.existsAsFile())
        return false;

    auto xml = juce::XmlDocument::parse (file);
    if (xml == nullptr)
        return false;

    const auto preset = juce::ValueTree::fromXml (*xml);
    if (! preset.isValid())
        return false;

    const int currentQuality = static_cast<int> (
        parameters.getRawParameterValue ("quality")->load());
    if (static_cast<int> (preset.getProperty ("qualityMode", -1))
        != currentQuality)
        return false;

    const auto profile = preset.getProperty ("noiseProfile").toString();
    if (profile.isEmpty())
        return false;

    setRawParameterValue ("reduction",
        static_cast<float> (preset.getProperty ("reduction", 8.0f)));
    setRawParameterValue ("preserve",
        static_cast<float> (preset.getProperty ("preserve", 0.75f)));
    setRawParameterValue ("silence",
        static_cast<float> (preset.getProperty ("silence", 0.55f)));
    setRawParameterValue ("thresholdOffset",
        static_cast<float> (preset.getProperty ("thresholdOffset", 1.5f)));

    applyParametersToEngine();
    if (! engine.restoreProfile (profile))
    {
        pendingProfile = profile;
        return true;
    }

    pendingProfile.clear();
    return true;
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
#ifdef SMART_DENOISE_HEADLESS_PROCESSOR_TEST
    return nullptr;
#else
    return new SmartDenoiseAudioProcessorEditor (*this);
#endif
}

juce::AudioProcessor*
JUCE_CALLTYPE createPluginFilter()
{
    return new SmartDenoiseAudioProcessor();
}
