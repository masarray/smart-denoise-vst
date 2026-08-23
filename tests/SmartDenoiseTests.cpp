#include "../Source/DSP/SmartDenoiseEngine.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

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

void processGenerated (smartdenoise::SmartDenoiseEngine& engine,
                       int totalSamples,
                       int channels,
                       const std::function<float(std::int64_t, int)>& generator,
                       std::vector<float>* captured = nullptr,
                       std::int64_t startIndex = 0)
{
    juce::AudioBuffer<float> buffer (channels, blockSize);
    if (captured != nullptr)
        captured->reserve (captured->size() + static_cast<size_t> (totalSamples));

    int processed = 0;
    while (processed < totalSamples)
    {
        const int count = std::min (blockSize, totalSamples - processed);
        buffer.clear();

        for (int channel = 0; channel < channels; ++channel)
            for (int sample = 0; sample < count; ++sample)
                buffer.setSample (channel, sample, generator (startIndex + processed + sample, channel));

        engine.process (buffer);

        if (captured != nullptr)
            for (int sample = 0; sample < count; ++sample)
                captured->push_back (buffer.getSample (0, sample));

        processed += count;
    }
}

void learnStationaryProfile (smartdenoise::SmartDenoiseEngine& engine, std::int64_t startIndex = 0)
{
    engine.startLearning (3.0);
    processGenerated (engine,
                      static_cast<int> (sampleRate * 3.6),
                      2,
                      [] (std::int64_t n, int) { return stationarySample (n); },
                      nullptr,
                      startIndex);
}

double rms (const std::vector<float>& values, size_t start)
{
    if (start >= values.size())
        return 0.0;

    long double energy = 0.0;
    size_t count = 0;
    for (size_t i = start; i < values.size(); ++i)
    {
        const long double value = values[i];
        energy += value * value;
        ++count;
    }

    return count > 0 ? std::sqrt (static_cast<double> (energy / static_cast<long double> (count))) : 0.0;
}

void testBypassLatency (TestContext& t)
{
    smartdenoise::SmartDenoiseEngine engine;
    engine.setQuality (smartdenoise::SmartDenoiseEngine::Quality::live1024);
    engine.prepare (sampleRate, blockSize, 1);
    engine.setEnabled (false);
    engine.setSilenceAmount (0.0f);

    std::vector<float> output;
    processGenerated (engine, 4096, 1,
                      [] (std::int64_t n, int) { return n == 0 ? 1.0f : 0.0f; },
                      &output);

    t.expect (engine.getLatencySamples() == 1024, "Live mode reports 1024 samples latency");

    size_t peakIndex = 0;
    float peak = 0.0f;
    for (size_t i = 0; i < output.size(); ++i)
    {
        if (std::abs (output[i]) > peak)
        {
            peak = std::abs (output[i]);
            peakIndex = i;
        }
    }

    t.expect (peakIndex == 1024, "Bypass impulse is delayed by exactly reported latency");
    t.expect (std::abs (output[1024] - 1.0f) < 1.0e-6f, "Bypass preserves delayed sample value");
}

void testSetupLevelQuality (TestContext& t)
{
    smartdenoise::SmartDenoiseEngine engine;
    engine.prepare (sampleRate, blockSize, 2);
    t.expect (engine.getLatencySamples() == 1024, "Default quality is Live");

    engine.setQuality (smartdenoise::SmartDenoiseEngine::Quality::clean2048);
    t.expect (engine.getLatencySamples() == 1024, "Quality request does not rebuild STFT until prepare");

    engine.prepare (sampleRate, blockSize, 2);
    t.expect (engine.getLatencySamples() == 2048, "Clean quality applies on non-real-time prepare");
}

void testClamps (TestContext& t)
{
    smartdenoise::SmartDenoiseEngine engine;
    engine.setReductionDb (100.0f);
    engine.setPreserve (-3.0f);
    engine.setSilenceAmount (5.0f);

    t.expect (std::abs (engine.getReductionDb() - smartdenoise::SmartDenoiseEngine::maxReductionDb) < 1.0e-6f,
              "Reduction clamps to 24 dB");
    t.expect (std::abs (engine.getPreserve()) < 1.0e-6f, "Preserve clamps to zero");
    t.expect (std::abs (engine.getSilenceAmount() - 1.0f) < 1.0e-6f, "Silence amount clamps to one");
}

void testLearningAndPersistence (TestContext& t)
{
    smartdenoise::SmartDenoiseEngine engine;
    engine.setQuality (smartdenoise::SmartDenoiseEngine::Quality::live1024);
    engine.prepare (sampleRate, blockSize, 2);
    engine.setSilenceAmount (0.0f);
    learnStationaryProfile (engine);

    t.expect (engine.hasProfile(), "Stationary Learn creates a valid profile");
    t.expect (engine.getProfileQuality() >= 0.25f, "Valid profile passes quality gate");

    const auto encoded = engine.serialiseProfile();
    t.expect (encoded.isNotEmpty(), "Valid profile serialises");

    auto restored = std::make_unique<smartdenoise::SmartDenoiseEngine>();
    restored->setQuality (smartdenoise::SmartDenoiseEngine::Quality::live1024);
    restored->prepare (sampleRate, blockSize, 2);
    t.expect (restored->restoreProfile (encoded), "Compatible engine restores learned profile");
    t.expect (restored->hasProfile(), "Restored profile becomes active");

    auto incompatible = std::make_unique<smartdenoise::SmartDenoiseEngine>();
    incompatible->setQuality (smartdenoise::SmartDenoiseEngine::Quality::clean2048);
    incompatible->prepare (sampleRate, blockSize, 2);
    t.expect (! incompatible->restoreProfile (encoded), "FFT-grid mismatch rejects profile restore");
}

void testRejectedRelearnKeepsProfile (TestContext& t)
{
    smartdenoise::SmartDenoiseEngine engine;
    engine.prepare (sampleRate, blockSize, 2);
    engine.setSilenceAmount (0.0f);
    learnStationaryProfile (engine);
    t.expect (engine.hasProfile(), "Precondition profile exists before bad Relearn");

    std::uint32_t state = 0x12345678u;
    engine.startLearning (3.0);
    processGenerated (engine,
                      static_cast<int> (sampleRate * 3.6),
                      2,
                      [&state] (std::int64_t, int channel)
                      {
                          state = state * 1664525u + 1013904223u + static_cast<std::uint32_t> (channel);
                          const float random = static_cast<float> ((state >> 8) & 0x00ffffffu)
                                             / static_cast<float> (0x00ffffffu);
                          return (random * 2.0f - 1.0f) * 0.22f;
                      });

    t.expect (engine.wasLastLearnRejected(), "Contaminated Relearn is rejected");
    t.expect (engine.hasProfile(), "Rejected Relearn keeps previous valid profile");
}

void testAttenuationAndProtection (TestContext& t)
{
    smartdenoise::SmartDenoiseEngine engine;
    engine.prepare (sampleRate, blockSize, 2);
    engine.setSilenceAmount (0.0f);
    engine.setReductionDb (12.0f);
    engine.setPreserve (0.75f);
    learnStationaryProfile (engine);
    t.expect (engine.hasProfile(), "Attenuation test has learned profile");

    std::vector<float> sameNoise;
    const std::int64_t start = static_cast<std::int64_t> (sampleRate * 3.6);
    processGenerated (engine,
                      static_cast<int> (sampleRate * 2.0),
                      2,
                      [] (std::int64_t n, int) { return stationarySample (n); },
                      &sameNoise,
                      start);

    const double dryRms = std::sqrt ((0.012 * 0.012 + 0.005 * 0.005) / 2.0);
    const double noiseRms = rms (sameNoise, static_cast<size_t> (sampleRate * 0.8));
    t.expect (noiseRms < dryRms * 0.75, "Learned stationary noise receives material attenuation");

    std::vector<float> wanted;
    processGenerated (engine,
                      static_cast<int> (sampleRate * 1.5),
                      2,
                      [] (std::int64_t n, int) { return stationarySample (n, 8.0f); },
                      &wanted,
                      start + static_cast<std::int64_t> (sampleRate * 2.0));

    const double wantedRms = rms (wanted, static_cast<size_t> (sampleRate * 0.5));
    const double wantedDryRms = dryRms * 8.0;
    t.expect (wantedRms > wantedDryRms * 0.70, "Strong harmonic program is protected from deep reduction");
}

void testFiniteOutput (TestContext& t)
{
    smartdenoise::SmartDenoiseEngine engine;
    engine.prepare (44100.0, 127, 2);

    std::vector<float> output;
    std::uint32_t state = 1u;
    processGenerated (engine,
                      20000,
                      2,
                      [&state] (std::int64_t, int)
                      {
                          state = state * 1103515245u + 12345u;
                          return (static_cast<float> ((state >> 9) & 0x007fffffu)
                                  / static_cast<float> (0x007fffffu) - 0.5f) * 0.1f;
                      },
                      &output);

    const bool finite = std::all_of (output.begin(), output.end(),
                                     [] (float value) { return std::isfinite (value); });
    t.expect (finite, "Arbitrary finite input never produces NaN/Inf");
}
} // namespace

int main()
{
    TestContext tests;
    std::cout << "SMART DENOISE ENGINE BLACK-BOX TESTS\n====================================\n";
    testBypassLatency (tests);
    testSetupLevelQuality (tests);
    testClamps (tests);
    testLearningAndPersistence (tests);
    testRejectedRelearnKeepsProfile (tests);
    testAttenuationAndProtection (tests);
    testFiniteOutput (tests);
    std::cout << "====================================\nFailures: " << tests.failures << '\n';
    return tests.failures == 0 ? 0 : 1;
}
