from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
def read(p): return (ROOT/p).read_text(encoding='utf-8')
def write(p,s): (ROOT/p).write_text(s,encoding='utf-8',newline='\n')
def rep(s,a,b,label):
    if a not in s: raise RuntimeError(f'missing anchor: {label}')
    return s.replace(a,b,1)

h_path='Source/Plugin/PluginProcessor.h'
h=read(h_path)
h=rep(h,'    void startNoiseLearn()\n    {\n        engine.startLearning (3.0);\n    }\n\n    float getInputPeakDb() const noexcept\n','    void startNoiseLearn()\n    {\n        engine.startLearning (3.0);\n    }\n\n    juce::StringArray getCapturedProfilePresetNames() const;\n    juce::String saveCapturedProfilePreset();\n    bool loadCapturedProfilePreset (const juce::String& presetName);\n\n    float getInputPeakDb() const noexcept\n','public preset api')
h=rep(h,'    void applyParametersToEngine();\n\n    smartdenoise::SmartDenoiseEngine engine;\n','    void applyParametersToEngine();\n    juce::File getCapturedPresetDirectory() const;\n    void setRawParameterValue (const juce::String& parameterId, float rawValue);\n\n    smartdenoise::SmartDenoiseEngine engine;\n','private preset api')
write(h_path,h)

cpp_path='Source/Plugin/PluginProcessor.cpp'
cpp=read(cpp_path)
anchor='void SmartDenoiseAudioProcessor::getStateInformation (\n'
if anchor not in cpp: raise RuntimeError('state anchor missing')
impl=r'''juce::File SmartDenoiseAudioProcessor::getCapturedPresetDirectory() const
{
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

'''
cpp=cpp.replace(anchor,impl+anchor,1)
write(cpp_path,cpp)
