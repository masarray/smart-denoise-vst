#include "../Source/Plugin/PluginProcessor.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>

namespace
{
constexpr double sampleRate = 48000.0;
constexpr int blockSize = 256;

struct TestContext
{
    int failures = 0;

    void expect (bool condition, const std::string& message)
    {
        std::cout << (condition ? "[PASS] " : "[FAIL] ") << message << '\n';
        if (! condition)
            ++failures;
    }
};

float stationarySample (std::int64_t sampleIndex, float scale = 1.0f)
{
    constexpr double binHz = sampleRate / 1024.0;
    constexpr double f1 = binHz * 95.0;
    constexpr double f2 = binHz * 171.0;
    const double t = static_cast<double> (sampleIndex) / sampleRate;

    return scale * (0.012f * std::sin (static_cast<float> (juce::MathConstants<double>::twoPi * f1 * t))
                  + 0.005f * std::sin (static_cast<float> (juce::MathConstants<double>::twoPi * f2 * t)));
}

void setRawParameter (SmartDenoiseAudioProcessor& processor,
                      const juce::String& parameterId,
                      float rawValue)
{
    auto* parameter = processor.getParameters().getParameter (parameterId);
    if (parameter == nullptr)
        return;

    parameter->beginChangeGesture();
    parameter->setValueNotifyingHost (parameter->convertTo0to1 (rawValue));
    parameter->endChangeGesture();
}

float getRawParameter (SmartDenoiseAudioProcessor& processor,
                       const juce::String& parameterId)
{
    if (const auto* value = processor.getParameters().getRawParameterValue (parameterId))
        return value->load();
    return 0.0f;
}

void processGenerated (SmartDenoiseAudioProcessor& processor,
                       int totalSamples,
                       const std::function<float(std::int64_t, int)>& generator,
                       std::int64_t startIndex = 0)
{
    juce::AudioBuffer<float> buffer (2, blockSize);
    juce::MidiBuffer midi;

    int processed = 0;
    while (processed < totalSamples)
    {
        const int count = std::min (blockSize, totalSamples - processed);
        buffer.clear();

        for (int channel = 0; channel < 2; ++channel)
            for (int sample = 0; sample < count; ++sample)
                buffer.setSample (channel, sample,
                                  generator (startIndex + processed + sample, channel));

        processor.processBlock (buffer, midi);
        processed += count;
    }
}

float fingerprintEnergy (const smartdenoise::SmartDenoiseEngine& engine)
{
    float sum = 0.0f;
    for (float value : engine.getProfileDisplay())
        sum += value;
    return sum;
}

void setProfileDirectoryEnvironment (const juce::File& directory)
{
    const auto path = directory.getFullPathName().toStdString();
#if JUCE_WINDOWS
    _putenv_s ("SMART_DENOISE_PROFILE_BANK_DIR", path.c_str());
#else
    setenv ("SMART_DENOISE_PROFILE_BANK_DIR", path.c_str(), 1);
#endif
}

void clearProfileDirectoryEnvironment()
{
#if JUCE_WINDOWS
    _putenv_s ("SMART_DENOISE_PROFILE_BANK_DIR", "");
#else
    unsetenv ("SMART_DENOISE_PROFILE_BANK_DIR");
#endif
}

void testRealWorkflow (TestContext& t)
{
    auto profileDirectory = juce::File::getSpecialLocation (juce::File::tempDirectory)
        .getNonexistentChildFile ("SmartDenoise-P53-ProfileBank", {}, true);
    profileDirectory.createDirectory();
    setProfileDirectoryEnvironment (profileDirectory);

    // SmartDenoiseAudioProcessor owns the large fixed-size FFT/profile engine.
    // Keep processor fixtures on the heap so this product-level test does not
    // exhaust the default Windows executable stack when several states coexist.
    auto processor = std::make_unique<SmartDenoiseAudioProcessor>();
    processor->prepareToPlay (sampleRate, blockSize);

    t.expect (! processor->getEngine().hasProfile(),
              "EMPTY: new processor starts without a learned profile");
    t.expect (fingerprintEnergy (processor->getEngine()) < 1.0e-6f,
              "EMPTY: profile fingerprint is empty before Learn");

    processor->startNoiseLearn();
    t.expect (! processor->getEngine().hasProfile(),
              "CAPTURING: Learn temporarily removes profile authority while capturing");

    processGenerated (*processor,
                      static_cast<int> (sampleRate * 0.8),
                      [] (std::int64_t n, int) { return stationarySample (n); });
    const float progressOne = processor->getEngine().getLearningProgress();

    processGenerated (*processor,
                      static_cast<int> (sampleRate * 0.8),
                      [] (std::int64_t n, int) { return stationarySample (n); },
                      static_cast<std::int64_t> (sampleRate * 0.8));
    const float progressTwo = processor->getEngine().getLearningProgress();

    t.expect (processor->getEngine().isLearning(),
              "CAPTURING: Learn remains active before the 3 second target");
    t.expect (progressOne > 0.05f && progressOne < 0.60f,
              "CAPTURING: progress reports a meaningful partial value");
    t.expect (progressTwo > progressOne && progressTwo < 0.90f,
              "CAPTURING: progress is monotonic while audio is captured");

    processGenerated (*processor,
                      static_cast<int> (sampleRate * 2.0),
                      [] (std::int64_t n, int) { return stationarySample (n); },
                      static_cast<std::int64_t> (sampleRate * 1.6));

    t.expect (! processor->getEngine().isLearning(),
              "ACTIVE: Learn completes after enough noise-only audio");
    t.expect (processor->getEngine().hasProfile(),
              "ACTIVE: successful Learn creates a valid frozen profile");
    t.expect (processor->getEngine().getProfileQuality() >= 0.25f,
              "ACTIVE: learned profile passes the quality gate");
    t.expect (fingerprintEnergy (processor->getEngine()) > 0.1f,
              "ACTIVE: captured profile publishes a non-empty spectral fingerprint");

    setRawParameter (*processor, "reduction", 11.7f);
    setRawParameter (*processor, "preserve", 0.82f);
    setRawParameter (*processor, "silence", 0.43f);
    setRawParameter (*processor, "thresholdOffset", 2.4f);

    const auto savedName = processor->saveCapturedProfilePreset();
    t.expect (savedName.isNotEmpty(),
              "SAVED: successful Learn can be persisted to the captured Profile Bank");

    const auto names = processor->getCapturedProfilePresetNames();
    t.expect (names.contains (savedName),
              "SAVED: captured Profile Bank enumerates the saved profile snapshot");

    juce::MemoryBlock sessionState;
    processor->getStateInformation (sessionState);

    auto sessionRestored = std::make_unique<SmartDenoiseAudioProcessor>();
    sessionRestored->setStateInformation (sessionState.getData(),
                                          static_cast<int> (sessionState.getSize()));
    sessionRestored->prepareToPlay (sampleRate, blockSize);

    t.expect (sessionRestored->getEngine().hasProfile(),
              "RESTORED: host session state restores the frozen profile");
    t.expect (fingerprintEnergy (sessionRestored->getEngine()) > 0.1f,
              "RESTORED: host session restore republishes the captured fingerprint");

    auto bankRestored = std::make_unique<SmartDenoiseAudioProcessor>();
    bankRestored->prepareToPlay (sampleRate, blockSize);
    setRawParameter (*bankRestored, "reduction", 3.0f);
    setRawParameter (*bankRestored, "preserve", 0.20f);
    setRawParameter (*bankRestored, "silence", 0.10f);
    setRawParameter (*bankRestored, "thresholdOffset", -1.0f);

    t.expect (bankRestored->loadCapturedProfilePreset (savedName),
              "RESTORED: compatible captured Profile Bank entry loads successfully");
    t.expect (bankRestored->getEngine().hasProfile(),
              "RESTORED: Profile Bank load makes the saved frozen profile active");
    t.expect (fingerprintEnergy (bankRestored->getEngine()) > 0.1f,
              "RESTORED: Profile Bank load restores the spectral fingerprint");
    t.expect (std::abs (getRawParameter (*bankRestored, "reduction") - 11.7f) < 0.11f,
              "RESTORED: Profile Bank restores Reduction");
    t.expect (std::abs (getRawParameter (*bankRestored, "preserve") - 0.82f) < 0.02f,
              "RESTORED: Profile Bank restores Preserve Detail");
    t.expect (std::abs (getRawParameter (*bankRestored, "silence") - 0.43f) < 0.02f,
              "RESTORED: Profile Bank restores Silence Clean-up");
    t.expect (std::abs (getRawParameter (*bankRestored, "thresholdOffset") - 2.4f) < 0.11f,
              "RESTORED: Profile Bank restores Profile Offset");

    auto incompatible = std::make_unique<SmartDenoiseAudioProcessor>();
    setRawParameter (*incompatible, "quality", 1.0f);
    incompatible->prepareToPlay (sampleRate, blockSize);

    t.expect (! incompatible->getCapturedProfilePresetNames().contains (savedName),
              "QUALITY SAFETY: incompatible Clean/Live snapshot is hidden from the bank");
    t.expect (! incompatible->loadCapturedProfilePreset (savedName),
              "QUALITY SAFETY: incompatible Clean/Live snapshot is rejected");

    const auto frozenBeforeRejectedLearn = bankRestored->getEngine().serialiseProfile();
    std::uint32_t randomState = 0xBADC0FFEu;
    bankRestored->startNoiseLearn();
    processGenerated (*bankRestored,
                      static_cast<int> (sampleRate * 3.6),
                      [&randomState] (std::int64_t, int channel)
                      {
                          randomState = randomState * 1664525u + 1013904223u
                              + static_cast<std::uint32_t> (channel);
                          const float random = static_cast<float> ((randomState >> 8) & 0x00ffffffu)
                              / static_cast<float> (0x00ffffffu);
                          return (random * 2.0f - 1.0f) * 0.22f;
                      });

    t.expect (bankRestored->getEngine().wasLastLearnRejected(),
              "REJECTED RELEARN: contaminated capture is rejected");
    t.expect (bankRestored->getEngine().hasProfile(),
              "REJECTED RELEARN: previous valid profile remains active");
    t.expect (bankRestored->getEngine().serialiseProfile() == frozenBeforeRejectedLearn,
              "REJECTED RELEARN: frozen profile bytes remain unchanged");

    incompatible.reset();
    bankRestored.reset();
    sessionRestored.reset();
    processor.reset();
    clearProfileDirectoryEnvironment();
    profileDirectory.deleteRecursively();
}
} // namespace

int main()
{
    TestContext tests;
    std::cout << "SMART DENOISE P5.3 PRODUCT WORKFLOW TESTS\n"
              << "========================================\n";

    testRealWorkflow (tests);

    std::cout << "========================================\n"
              << "Failures: " << tests.failures << '\n';
    return tests.failures == 0 ? 0 : 1;
}
