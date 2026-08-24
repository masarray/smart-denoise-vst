from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def read(path):
    return (ROOT / path).read_text(encoding='utf-8')

def write(path, text):
    (ROOT / path).write_text(text, encoding='utf-8', newline='\n')

def rep(text, old, new, label):
    if old not in text:
        raise RuntimeError(f'missing anchor: {label}')
    return text.replace(old, new, 1)

h_path='Source/DSP/SmartDenoiseEngine.h'
h=read(h_path)
h=rep(h,'    static constexpr int detailHopSize = detailFftSize / 2;\n    static constexpr float maxReductionDb = 24.0f;\n','    static constexpr int detailHopSize = detailFftSize / 2;\n    static constexpr int profileDisplayBins = 48;\n    static constexpr float maxReductionDb = 24.0f;\n','display constant')
h=rep(h,'    NoiseFrameAnalysis getFrameAnalysis() const noexcept;\n\n    juce::String serialiseProfile() const;\n','    NoiseFrameAnalysis getFrameAnalysis() const noexcept;\n    std::array<float, profileDisplayBins> getProfileDisplay() const noexcept;\n\n    juce::String serialiseProfile() const;\n','display getter')
h=rep(h,'    void resetSpectralState() noexcept;\n    void invalidateProfile() noexcept;\n\n    float frequencyWeight','    void resetSpectralState() noexcept;\n    void invalidateProfile() noexcept;\n    void publishProfileDisplay() noexcept;\n\n    float frequencyWeight','display publisher decl')
h=rep(h,'    std::array<std::array<float, maxBins>, maxChannels> profileVarianceDb2 {};\n\n    std::array<float, maxBins> linkedGainState {};\n','    std::array<std::array<float, maxBins>, maxChannels> profileVarianceDb2 {};\n    std::array<std::atomic<float>, profileDisplayBins> profileDisplay {};\n\n    std::array<float, maxBins> linkedGainState {};\n','display atomics')
write(h_path,h)

cpp_path='Source/DSP/SmartDenoiseEngine.cpp'
cpp=read(cpp_path)
cpp=rep(cpp,'    profileChannels = 0;\n\n    resetSpectralState();\n','    profileChannels = 0;\n    for (auto& value : profileDisplay)\n        value.store (0.0f, std::memory_order_relaxed);\n\n    resetSpectralState();\n','clear display')
marker='void SmartDenoiseEngine::setQuality (\n'
if marker not in cpp:
    raise RuntimeError('setQuality anchor missing')
impl=r'''std::array<float, SmartDenoiseEngine::profileDisplayBins>
SmartDenoiseEngine::getProfileDisplay() const noexcept
{
    std::array<float, profileDisplayBins> result {};
    for (int index = 0; index < profileDisplayBins; ++index)
        result[static_cast<size_t> (index)] =
            profileDisplay[static_cast<size_t> (index)].load (
                std::memory_order_relaxed);
    return result;
}

void SmartDenoiseEngine::publishProfileDisplay() noexcept
{
    std::array<float, profileDisplayBins> dbValues {};
    const int sourceBins = juce::jlimit (2, maxBins, profileFftSize / 2 + 1);
    const int channels = juce::jlimit (1, maxChannels, profileChannels);

    float peakDb = -120.0f;
    for (int displayBin = 0; displayBin < profileDisplayBins; ++displayBin)
    {
        const float t0 = static_cast<float> (displayBin)
            / static_cast<float> (profileDisplayBins);
        const float t1 = static_cast<float> (displayBin + 1)
            / static_cast<float> (profileDisplayBins);
        const float curved0 = std::pow (t0, 1.55f);
        const float curved1 = std::pow (t1, 1.55f);
        const int first = juce::jlimit (
            1, sourceBins - 1,
            1 + static_cast<int> (curved0 * static_cast<float> (sourceBins - 2)));
        const int last = juce::jlimit (
            first + 1, sourceBins,
            1 + static_cast<int> (std::ceil (
                curved1 * static_cast<float> (sourceBins - 2))));

        double powerSum = 0.0;
        int count = 0;
        for (int channel = 0; channel < channels; ++channel)
        {
            for (int bin = first; bin < last; ++bin)
            {
                powerSum += profilePower[static_cast<size_t> (channel)]
                    [static_cast<size_t> (bin)];
                ++count;
            }
        }

        const float meanPower = static_cast<float> (
            powerSum / static_cast<double> (juce::jmax (1, count)));
        const float db = 10.0f * std::log10 (juce::jmax (meanPower, 1.0e-16f));
        dbValues[static_cast<size_t> (displayBin)] = db;
        peakDb = juce::jmax (peakDb, db);
    }

    const float floorDb = peakDb - 42.0f;
    for (int index = 0; index < profileDisplayBins; ++index)
    {
        const float normalized = juce::jlimit (
            0.0f, 1.0f,
            (dbValues[static_cast<size_t> (index)] - floorDb) / 42.0f);
        profileDisplay[static_cast<size_t> (index)].store (
            normalized, std::memory_order_relaxed);
    }
}

'''
cpp=cpp.replace(marker,impl+marker,1)
cpp=rep(cpp,'        profileSampleRate = sampleRate;\n        profileFftSize = fftSize;\n        profileChannels = channelCount;\n\n        profileValid.store (true);\n','        profileSampleRate = sampleRate;\n        profileFftSize = fftSize;\n        profileChannels = channelCount;\n        publishProfileDisplay();\n\n        profileValid.store (true);\n','publish learned display')
cpp=rep(cpp,'    profileSampleRate = savedRate;\n    profileFftSize = savedFftSize;\n    profileChannels = savedChannels;\n\n    lastLearnRejected.store (false);\n','    profileSampleRate = savedRate;\n    profileFftSize = savedFftSize;\n    profileChannels = savedChannels;\n    publishProfileDisplay();\n\n    lastLearnRejected.store (false);\n','publish restored display')
write(cpp_path,cpp)

test_path='tests/SmartDenoiseTests.cpp'
test=read(test_path)
test=rep(test,'    t.expect (engine.getProfileQuality() >= 0.25f, "Valid profile passes quality gate");\n\n    const auto encoded = engine.serialiseProfile();\n','    t.expect (engine.getProfileQuality() >= 0.25f, "Valid profile passes quality gate");\n\n    const auto learnedFingerprint = engine.getProfileDisplay();\n    t.expect (\n        std::any_of (learnedFingerprint.begin(), learnedFingerprint.end(),\n                    [] (float value) { return value > 0.05f; }),\n        "Learn publishes a non-empty captured-profile fingerprint");\n\n    const auto encoded = engine.serialiseProfile();\n','learned fingerprint test')
test=rep(test,'    t.expect (restored->hasProfile(), "Restored profile becomes active");\n\n    auto incompatible =','    t.expect (restored->hasProfile(), "Restored profile becomes active");\n    const auto restoredFingerprint = restored->getProfileDisplay();\n    t.expect (\n        std::any_of (restoredFingerprint.begin(), restoredFingerprint.end(),\n                    [] (float value) { return value > 0.05f; }),\n        "Restored profile republishes its visual fingerprint");\n\n    auto incompatible =','restored fingerprint test')
write(test_path,test)
