#include "../Source/DSP/SmartDenoiseEngine.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
constexpr double sampleRate = 48000.0;
constexpr int blockSize = 256;
constexpr int channels = 2;
constexpr double durationSeconds = 2.6;
constexpr int sampleCount = static_cast<int> (sampleRate * durationSeconds);

struct Metric
{
    std::string fixture;
    std::string name;
    double value = 0.0;
    std::string unit;
    double minimum = -std::numeric_limits<double>::infinity();
    bool gated = false;
    bool pass = true;
};

float hashNoise (std::int64_t n, std::uint32_t seed)
{
    std::uint32_t x = static_cast<std::uint32_t> (n) + seed * 0x9e3779b9u;
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    x ^= x >> 16;
    return static_cast<float> (x) / static_cast<float> (0xffffffffu) * 2.0f - 1.0f;
}

float hissNoise (std::int64_t n, std::uint32_t seed = 1u)
{
    const float current = hashNoise (n, seed);
    const float previous = hashNoise (n - 1, seed);
    return (current - 0.78f * previous) * 0.0105f;
}

float humNoise (std::int64_t n)
{
    const double t = static_cast<double> (n) / sampleRate;
    return 0.018f * std::sin (static_cast<float> (juce::MathConstants<double>::twoPi * 50.0 * t))
         + 0.010f * std::sin (static_cast<float> (juce::MathConstants<double>::twoPi * 100.0 * t))
         + 0.006f * std::sin (static_cast<float> (juce::MathConstants<double>::twoPi * 150.0 * t))
         + 0.003f * std::sin (static_cast<float> (juce::MathConstants<double>::twoPi * 250.0 * t));
}

float fanNoise (std::int64_t n, std::uint32_t seed = 17u)
{
    const double t = static_cast<double> (n) / sampleRate;
    const float broadband = hashNoise (n, seed) * 0.0055f;
    const float slowTexture = hashNoise (n / 5, seed + 31u) * 0.0045f;
    return broadband + slowTexture
         + 0.006f * std::sin (static_cast<float> (juce::MathConstants<double>::twoPi * 118.0 * t))
         + 0.003f * std::sin (static_cast<float> (juce::MathConstants<double>::twoPi * 236.0 * t));
}

float speechLike (std::int64_t n)
{
    const double t = static_cast<double> (n) / sampleRate;
    const double phase = std::fmod (t, 0.42);
    const float syllable = static_cast<float> (0.18 + 0.82 * std::exp (-4.5 * phase));
    const double f0 = 145.0 + 9.0 * std::sin (juce::MathConstants<double>::twoPi * 1.7 * t);

    float value = 0.0f;
    for (int harmonic = 1; harmonic <= 8; ++harmonic)
    {
        const float weight = 0.050f / static_cast<float> (harmonic);
        value += weight * std::sin (static_cast<float> (
            juce::MathConstants<double>::twoPi * f0 * harmonic * t));
    }

    return value * syllable;
}

float transientLike (std::int64_t n)
{
    const double t = static_cast<double> (n) / sampleRate;
    const double period = 0.48;
    const double local = std::fmod (t, period);
    if (local > 0.16)
        return 0.0f;

    const float env = static_cast<float> (std::exp (-34.0 * local));
    return env * (0.12f * std::sin (static_cast<float> (juce::MathConstants<double>::twoPi * 3200.0 * t))
                + 0.075f * std::sin (static_cast<float> (juce::MathConstants<double>::twoPi * 7100.0 * t)));
}

float pluckLike (std::int64_t n)
{
    const double t = static_cast<double> (n) / sampleRate;
    const double period = 0.68;
    const double local = std::fmod (t, period);
    const float env = static_cast<float> (std::exp (-7.8 * local));
    return env * (0.080f * std::sin (static_cast<float> (juce::MathConstants<double>::twoPi * 220.0 * t))
                + 0.045f * std::sin (static_cast<float> (juce::MathConstants<double>::twoPi * 440.0 * t))
                + 0.025f * std::sin (static_cast<float> (juce::MathConstants<double>::twoPi * 660.0 * t)));
}

float reverbLike (std::int64_t n)
{
    const double t = static_cast<double> (n) / sampleRate;
    if (t < 0.20)
        return 0.0f;

    const double local = t - 0.20;
    const float env = static_cast<float> (std::exp (-1.75 * local));
    return env * (0.052f * std::sin (static_cast<float> (juce::MathConstants<double>::twoPi * 330.0 * t))
                + 0.036f * std::sin (static_cast<float> (juce::MathConstants<double>::twoPi * 495.0 * t))
                + 0.024f * std::sin (static_cast<float> (juce::MathConstants<double>::twoPi * 660.0 * t)));
}

std::vector<float> makeSignal (const std::function<float(std::int64_t)>& generator)
{
    std::vector<float> result (static_cast<size_t> (sampleCount));
    for (int i = 0; i < sampleCount; ++i)
        result[static_cast<size_t> (i)] = generator (i);
    return result;
}

std::vector<float> addSignals (const std::vector<float>& a,
                               const std::vector<float>& b)
{
    const size_t count = std::min (a.size(), b.size());
    std::vector<float> result (count);
    for (size_t i = 0; i < count; ++i)
        result[i] = a[i] + b[i];
    return result;
}

void learnProfile (smartdenoise::SmartDenoiseEngine& engine,
                   const std::function<float(std::int64_t)>& noise)
{
    engine.startLearning (3.0);
    juce::AudioBuffer<float> buffer (channels, blockSize);
    int processed = 0;
    const int total = static_cast<int> (sampleRate * 3.6);

    while (processed < total)
    {
        const int count = std::min (blockSize, total - processed);
        buffer.clear();
        for (int sample = 0; sample < count; ++sample)
        {
            const float value = noise (processed + sample);
            for (int channel = 0; channel < channels; ++channel)
                buffer.setSample (channel, sample, value);
        }
        engine.process (buffer);
        processed += count;
    }
}

std::vector<float> processAligned (smartdenoise::SmartDenoiseEngine& engine,
                                   const std::vector<float>& input)
{
    const int latency = engine.getLatencySamples();
    const int total = static_cast<int> (input.size()) + latency + blockSize;
    juce::AudioBuffer<float> buffer (channels, blockSize);
    std::vector<float> raw;
    raw.reserve (static_cast<size_t> (total));

    int processed = 0;
    while (processed < total)
    {
        const int count = std::min (blockSize, total - processed);
        buffer.clear();
        for (int sample = 0; sample < count; ++sample)
        {
            const int index = processed + sample;
            const float value = index < static_cast<int> (input.size())
                ? input[static_cast<size_t> (index)]
                : 0.0f;
            for (int channel = 0; channel < channels; ++channel)
                buffer.setSample (channel, sample, value);
        }

        engine.process (buffer);
        for (int sample = 0; sample < count; ++sample)
            raw.push_back (buffer.getSample (0, sample));
        processed += count;
    }

    std::vector<float> aligned (input.size(), 0.0f);
    for (size_t i = 0; i < input.size(); ++i)
    {
        const size_t source = static_cast<size_t> (latency) + i;
        if (source < raw.size())
            aligned[i] = raw[source];
    }
    return aligned;
}

double rms (const std::vector<float>& values,
            size_t start = 0,
            size_t end = std::numeric_limits<size_t>::max())
{
    end = std::min (end, values.size());
    if (start >= end)
        return 0.0;

    long double energy = 0.0;
    for (size_t i = start; i < end; ++i)
        energy += static_cast<long double> (values[i]) * values[i];
    return std::sqrt (static_cast<double> (energy / static_cast<long double> (end - start)));
}

double peak (const std::vector<float>& values)
{
    double result = 0.0;
    for (float value : values)
        result = std::max (result, static_cast<double> (std::abs (value)));
    return result;
}

double dbRatio (double numerator, double denominator)
{
    return 20.0 * std::log10 (std::max (numerator, 1.0e-12) / std::max (denominator, 1.0e-12));
}

std::vector<float> difference (const std::vector<float>& a,
                               const std::vector<float>& b)
{
    const size_t count = std::min (a.size(), b.size());
    std::vector<float> result (count);
    for (size_t i = 0; i < count; ++i)
        result[i] = a[i] - b[i];
    return result;
}

bool allFinite (const std::vector<float>& values)
{
    return std::all_of (values.begin(), values.end(),
                        [] (float value) { return std::isfinite (value); });
}

void writeU16 (std::ofstream& stream, std::uint16_t value)
{
    char bytes[2] { static_cast<char> (value & 0xffu),
                    static_cast<char> ((value >> 8u) & 0xffu) };
    stream.write (bytes, 2);
}

void writeU32 (std::ofstream& stream, std::uint32_t value)
{
    char bytes[4] {
        static_cast<char> (value & 0xffu),
        static_cast<char> ((value >> 8u) & 0xffu),
        static_cast<char> ((value >> 16u) & 0xffu),
        static_cast<char> ((value >> 24u) & 0xffu)
    };
    stream.write (bytes, 4);
}

bool writeWav16Mono (const std::filesystem::path& path,
                     const std::vector<float>& samples)
{
    std::ofstream stream (path, std::ios::binary);
    if (! stream)
        return false;

    const std::uint32_t dataSize = static_cast<std::uint32_t> (samples.size() * sizeof (std::int16_t));
    stream.write ("RIFF", 4);
    writeU32 (stream, 36u + dataSize);
    stream.write ("WAVE", 4);
    stream.write ("fmt ", 4);
    writeU32 (stream, 16u);
    writeU16 (stream, 1u);
    writeU16 (stream, 1u);
    writeU32 (stream, static_cast<std::uint32_t> (sampleRate));
    writeU32 (stream, static_cast<std::uint32_t> (sampleRate) * 2u);
    writeU16 (stream, 2u);
    writeU16 (stream, 16u);
    stream.write ("data", 4);
    writeU32 (stream, dataSize);

    for (float sample : samples)
    {
        const float clipped = std::clamp (sample, -0.999f, 0.999f);
        const auto pcm = static_cast<std::int16_t> (std::lround (clipped * 32767.0f));
        writeU16 (stream, static_cast<std::uint16_t> (pcm));
    }

    return static_cast<bool> (stream);
}

void addGateMetric (std::vector<Metric>& metrics,
                    const std::string& fixture,
                    const std::string& name,
                    double value,
                    const std::string& unit,
                    double minimum)
{
    metrics.push_back ({ fixture, name, value, unit, minimum, true, value >= minimum });
}

void addReportMetric (std::vector<Metric>& metrics,
                      const std::string& fixture,
                      const std::string& name,
                      double value,
                      const std::string& unit)
{
    metrics.push_back ({ fixture, name, value, unit, 0.0, false, true });
}

struct FixtureResult
{
    std::vector<float> clean;
    std::vector<float> noisy;
    std::vector<float> processed;
};

void checkpoint (const std::string& message)
{
    std::cerr << "[P5.3] " << message << std::endl;
}

FixtureResult runFixture (const std::function<float(std::int64_t)>& profileNoise,
                          const std::vector<float>& clean,
                          const std::vector<float>& noisy,
                          float reductionDb = 12.0f,
                          float preserve = 0.80f)
{
    checkpoint ("runFixture: allocate engine");
    auto engine = std::make_unique<smartdenoise::SmartDenoiseEngine>();
    checkpoint ("runFixture: prepare");
    engine->prepare (sampleRate, blockSize, channels);
    engine->setSilenceAmount (0.0f);
    engine->setReductionDb (reductionDb);
    engine->setPreserve (preserve);
    checkpoint ("runFixture: learn profile");
    learnProfile (*engine, profileNoise);

    if (! engine->hasProfile())
    {
        std::cerr << "[P5.3] runFixture: Learn rejected; quality="
                  << engine->getProfileQuality()
                  << " rejectedFrames=" << engine->getRejectedLearningFrames()
                  << std::endl;
        throw std::runtime_error ("Listening fixture profile Learn was rejected");
    }

    checkpoint ("runFixture: Learn accepted");
    checkpoint ("runFixture: process aligned");
    auto processed = processAligned (*engine, noisy);
    checkpoint ("runFixture: complete");
    return { clean, noisy, std::move (processed) };
}

void saveFixture (const std::filesystem::path& root,
                  const std::string& name,
                  const FixtureResult& result)
{
    checkpoint ("saveFixture: " + name);
    const auto directory = root / name;
    std::filesystem::create_directories (directory);
    writeWav16Mono (directory / "01-clean-reference.wav", result.clean);
    writeWav16Mono (directory / "02-noisy-input.wav", result.noisy);
    writeWav16Mono (directory / "03-smart-denoise.wav", result.processed);
}

void evaluateProgramFixture (std::vector<Metric>& metrics,
                             const std::string& name,
                             const FixtureResult& result)
{
    const auto inputError = difference (result.noisy, result.clean);
    const auto outputError = difference (result.processed, result.clean);
    const double signalRms = rms (result.clean);
    const double snrIn = dbRatio (signalRms, rms (inputError));
    const double snrOut = dbRatio (signalRms, rms (outputError));

    addReportMetric (metrics, name, "input_snr", snrIn, "dB");
    addReportMetric (metrics, name, "output_snr", snrOut, "dB");
    addReportMetric (metrics, name, "snr_delta", snrOut - snrIn, "dB");
    addGateMetric (metrics, name, "finite_output", allFinite (result.processed) ? 1.0 : 0.0, "bool", 1.0);
}
} // namespace

int main (int argc, char** argv)
{
    const std::filesystem::path outputRoot = argc >= 2
        ? std::filesystem::path (argv[1])
        : std::filesystem::path ("listening-validation");

    std::filesystem::create_directories (outputRoot);
    std::vector<Metric> metrics;

    const auto silence = std::vector<float> (static_cast<size_t> (sampleCount), 0.0f);

    checkpoint ("fixture 01 stationary hiss");
    const auto hiss = makeSignal ([] (std::int64_t n) { return hissNoise (n + 400000, 9u); });
    auto hissResult = runFixture (
        [] (std::int64_t n) { return hissNoise (n, 2u); }, silence, hiss, 12.0f, 0.78f);
    saveFixture (outputRoot, "01-stationary-hiss", hissResult);
    addGateMetric (metrics, "stationary-hiss", "noise_attenuation",
                   dbRatio (rms (hissResult.noisy), rms (hissResult.processed)), "dB", 1.5);

    checkpoint ("fixture 02 50 Hz hum");
    const auto hum = makeSignal ([] (std::int64_t n) { return humNoise (n + 17000); });
    auto humResult = runFixture (
        [] (std::int64_t n) { return humNoise (n); }, silence, hum, 12.0f, 0.78f);
    saveFixture (outputRoot, "02-50hz-hum", humResult);
    addGateMetric (metrics, "50hz-hum", "noise_attenuation",
                   dbRatio (rms (humResult.noisy), rms (humResult.processed)), "dB", 1.5);

    checkpoint ("fixture 03 broadband fan");
    const auto fan = makeSignal ([] (std::int64_t n) { return fanNoise (n + 250000, 23u); });
    auto fanResult = runFixture (
        [] (std::int64_t n) { return fanNoise (n, 17u); }, silence, fan, 12.0f, 0.78f);
    saveFixture (outputRoot, "03-broadband-fan", fanResult);
    addGateMetric (metrics, "broadband-fan", "noise_attenuation",
                   dbRatio (rms (fanResult.noisy), rms (fanResult.processed)), "dB", 0.75);

    checkpoint ("fixture 04 speech-like");
    const auto speechClean = makeSignal ([] (std::int64_t n) { return speechLike (n); });
    const auto speechNoise = makeSignal ([] (std::int64_t n) { return hissNoise (n + 800000, 13u); });
    auto speechResult = runFixture (
        [] (std::int64_t n) { return hissNoise (n, 3u); },
        speechClean, addSignals (speechClean, speechNoise), 10.0f, 0.86f);
    saveFixture (outputRoot, "04-speech-like", speechResult);
    evaluateProgramFixture (metrics, "speech-like", speechResult);
    addGateMetric (metrics, "speech-like", "program_rms_retention",
                   dbRatio (rms (speechResult.processed), rms (speechResult.clean)), "dB", -4.0);

    checkpoint ("fixture 05 transient-cymbal");
    const auto transientClean = makeSignal ([] (std::int64_t n) { return transientLike (n); });
    const auto transientNoise = makeSignal ([] (std::int64_t n) { return hissNoise (n + 1200000, 19u); });
    auto transientResult = runFixture (
        [] (std::int64_t n) { return hissNoise (n, 5u); },
        transientClean, addSignals (transientClean, transientNoise), 12.0f, 0.90f);
    saveFixture (outputRoot, "05-transient-cymbal", transientResult);
    evaluateProgramFixture (metrics, "transient-cymbal", transientResult);
    addGateMetric (metrics, "transient-cymbal", "peak_retention",
                   peak (transientResult.processed) / std::max (peak (transientResult.clean), 1.0e-9), "ratio", 0.55);

    checkpoint ("fixture 06 guitar-pluck");
    const auto pluckClean = makeSignal ([] (std::int64_t n) { return pluckLike (n); });
    const auto pluckNoise = makeSignal ([] (std::int64_t n) { return fanNoise (n + 900000, 37u); });
    auto pluckResult = runFixture (
        [] (std::int64_t n) { return fanNoise (n, 17u); },
        pluckClean, addSignals (pluckClean, pluckNoise), 10.0f, 0.88f);
    saveFixture (outputRoot, "06-guitar-pluck", pluckResult);
    evaluateProgramFixture (metrics, "guitar-pluck", pluckResult);
    addGateMetric (metrics, "guitar-pluck", "program_rms_retention",
                   rms (pluckResult.processed) / std::max (rms (pluckResult.clean), 1.0e-9), "ratio", 0.50);

    checkpoint ("fixture 07 reverb-tail");
    const auto reverbClean = makeSignal ([] (std::int64_t n) { return reverbLike (n); });
    const auto reverbNoise = makeSignal ([] (std::int64_t n) { return hissNoise (n + 1400000, 29u); });
    auto reverbResult = runFixture (
        [] (std::int64_t n) { return hissNoise (n, 7u); },
        reverbClean, addSignals (reverbClean, reverbNoise), 10.0f, 0.92f);
    saveFixture (outputRoot, "07-reverb-tail", reverbResult);
    evaluateProgramFixture (metrics, "reverb-tail", reverbResult);

    const size_t tailStart = static_cast<size_t> (sampleRate * 1.35);
    addGateMetric (metrics, "reverb-tail", "tail_rms_retention",
                   rms (reverbResult.processed, tailStart) / std::max (rms (reverbResult.clean, tailStart), 1.0e-9),
                   "ratio", 0.42);

    bool allPassed = true;
    std::ofstream csv (outputRoot / "metrics.csv");
    csv << "fixture,metric,value,unit,minimum,gated,pass\n";
    csv << std::fixed << std::setprecision (5);

    std::cout << "SMART DENOISE P5.3 LISTENING VALIDATION\n"
              << "=======================================\n";
    for (const auto& metric : metrics)
    {
        csv << metric.fixture << ',' << metric.name << ',' << metric.value << ','
            << metric.unit << ',';
        if (metric.gated)
            csv << metric.minimum;
        csv << ',' << (metric.gated ? "yes" : "no") << ',' << (metric.pass ? "PASS" : "FAIL") << '\n';

        std::cout << (metric.pass ? "[PASS] " : "[FAIL] ")
                  << metric.fixture << " / " << metric.name << " = "
                  << std::fixed << std::setprecision (3) << metric.value << ' ' << metric.unit;
        if (metric.gated)
            std::cout << " (min " << metric.minimum << ')';
        std::cout << '\n';

        if (metric.gated && ! metric.pass)
            allPassed = false;
    }

    std::ofstream readme (outputRoot / "LISTENING_GUIDE.txt");
    readme << "Smart Denoise P5.3 listening pack\n"
           << "=================================\n\n"
           << "Each fixture contains:\n"
           << "  01-clean-reference.wav (silence for noise-only fixtures)\n"
           << "  02-noisy-input.wav\n"
           << "  03-smart-denoise.wav\n\n"
           << "Listen level-matched. Focus on noise reduction AND damage to wanted content.\n"
           << "Check for metallic/chirpy residuals, watery texture, pumping, consonant loss,\n"
           << "cymbal/transient softening, stereo/timbre changes and reverb-tail truncation.\n\n"
           << "metrics.csv is an automated regression guard only. It is not subjective approval.\n";

    std::cout << "=======================================\n"
              << "Listening pack: " << outputRoot.string() << '\n'
              << "Result: " << (allPassed ? "PASS" : "FAIL") << '\n';

    return allPassed ? 0 : 1;
}
