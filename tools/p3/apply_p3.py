from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
HEADER = ROOT / "Source/DSP/SmartDenoiseEngine.h"
SOURCE = ROOT / "Source/DSP/SmartDenoiseEngine.cpp"


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{label}: expected exactly one anchor, found {count}")
    return text.replace(old, new, 1)


h = HEADER.read_text(encoding="utf-8")
s = SOURCE.read_text(encoding="utf-8")

h = replace_once(
    h,
    "    float spectralReductionDb = 0.0f;\n};",
    "    float spectralReductionDb = 0.0f;\n"
    "    float detailProtection = 0.0f;\n"
    "    float tailProtection = 0.0f;\n};",
    "analysis telemetry",
)

h = replace_once(
    h,
    "    static constexpr int profileGroupCount = 7;\n"
    "    static constexpr float maxReductionDb = 24.0f;",
    "    static constexpr int profileGroupCount = 7;\n"
    "    static constexpr int detailFftSize = 512;\n"
    "    static constexpr int detailBins = detailFftSize / 2 + 1;\n"
    "    static constexpr int detailHopSize = detailFftSize / 2;\n"
    "    static constexpr float maxReductionDb = 24.0f;",
    "detail constants",
)

h = replace_once(
    h,
    "    void finaliseLearningOnAudioThread() noexcept;\n"
    "    void processFrame() noexcept;\n"
    "    void applySmartExpander (juce::AudioBuffer<float>& target) noexcept;",
    "    void finaliseLearningOnAudioThread() noexcept;\n"
    "    void processDetailFrame() noexcept;\n"
    "    void processFrame() noexcept;\n"
    "    void applySmartExpander (juce::AudioBuffer<float>& target) noexcept;",
    "detail method declaration",
)

h = replace_once(
    h,
    "    float detectorFrequencyWeight (int bin) const noexcept;\n"
    "    float calculateLinkedGain (int bin, float linkedPower,",
    "    float detectorFrequencyWeight (int bin) const noexcept;\n"
    "    float detailProtectionForPrimaryBin (int bin) const noexcept;\n"
    "    float tailProtectionForPrimaryBin (int bin) const noexcept;\n"
    "    float calculateLinkedGain (int bin, float linkedPower,",
    "detail mapping declarations",
)

h = replace_once(
    h,
    "    FixedFft fixedFft;",
    "    FixedFft fixedFft;\n"
    "    FixedFft detailFft;",
    "detail fft member",
)

h = replace_once(
    h,
    "    int hopCounter = 0;\n"
    "    std::int64_t samplesSeen = 0;",
    "    int hopCounter = 0;\n"
    "    std::int64_t samplesSeen = 0;\n"
    "    int detailInputWritePos = 0;\n"
    "    int detailHopCounter = 0;\n"
    "    std::int64_t detailSamplesSeen = 0;",
    "detail scheduling state",
)

h = replace_once(
    h,
    "    std::array<float, maxBins> linkedPreviousMagnitude {};\n"
    "    std::array<float, maxBins> binTransientProbability {};\n\n"
    "    std::array<float, maxFftSize> window {};",
    "    std::array<float, maxBins> linkedPreviousMagnitude {};\n"
    "    std::array<float, maxBins> binTransientProbability {};\n"
    "    std::array<float, maxBins> gainHistoryOne {};\n"
    "    std::array<float, maxBins> gainHistoryTwo {};\n\n"
    "    std::array<std::array<float, detailFftSize>, maxChannels> detailInputRing {};\n"
    "    std::array<std::array<float, detailFftSize * 2>, maxChannels> detailWork {};\n"
    "    std::array<float, detailBins> detailPreviousMagnitude {};\n"
    "    std::array<float, detailBins> detailProtectionState {};\n"
    "    std::array<float, detailBins> detailTailMemory {};\n"
    "    std::array<float, detailFftSize> detailWindow {};\n\n"
    "    std::array<float, maxFftSize> window {};",
    "P3 state arrays",
)

h = replace_once(
    h,
    "    std::atomic<float> frameSpectralReductionDb { 0.0f };",
    "    std::atomic<float> frameSpectralReductionDb { 0.0f };\n"
    "    std::atomic<float> frameDetailProtection { 0.0f };\n"
    "    std::atomic<float> frameTailProtection { 0.0f };",
    "P3 telemetry atomics",
)

s = replace_once(
    s,
    "    size = newSize >= 2048 ? 2048 : 1024;",
    "    if (newSize >= 2048)\n"
    "        size = 2048;\n"
    "    else if (newSize >= 1024)\n"
    "        size = 1024;\n"
    "    else\n"
    "        size = 512;",
    "FFT 512 support",
)

s = replace_once(
    s,
    "SmartDenoiseEngine::SmartDenoiseEngine()\n"
    "{\n"
    "    fixedFft.configure (1024);",
    "SmartDenoiseEngine::SmartDenoiseEngine()\n"
    "{\n"
    "    fixedFft.configure (1024);\n"
    "    detailFft.configure (detailFftSize);",
    "detail FFT configure",
)

s = replace_once(
    s,
    "    linkedPreviousMagnitude.fill (0.0f);\n"
    "    binTransientProbability.fill (0.0f);",
    "    linkedPreviousMagnitude.fill (0.0f);\n"
    "    binTransientProbability.fill (0.0f);\n"
    "    gainHistoryOne.fill (1.0f);\n"
    "    gainHistoryTwo.fill (1.0f);\n"
    "    detailPreviousMagnitude.fill (0.0f);\n"
    "    detailProtectionState.fill (0.0f);\n"
    "    detailTailMemory.fill (0.0f);",
    "reset P3 spectral state",
)

s = replace_once(
    s,
    "    for (auto& channel : previousMagnitude)\n"
    "        channel.fill (0.0f);\n\n"
    "    resetSpectralState();",
    "    for (auto& channel : previousMagnitude)\n"
    "        channel.fill (0.0f);\n"
    "    for (auto& channel : detailInputRing)\n"
    "        channel.fill (0.0f);\n"
    "    for (auto& channel : detailWork)\n"
    "        channel.fill (0.0f);\n\n"
    "    resetSpectralState();",
    "reset detail buffers",
)

s = replace_once(
    s,
    "    hopCounter = 0;\n"
    "    samplesSeen = 0;",
    "    hopCounter = 0;\n"
    "    samplesSeen = 0;\n"
    "    detailInputWritePos = 0;\n"
    "    detailHopCounter = 0;\n"
    "    detailSamplesSeen = 0;",
    "reset detail scheduler",
)

s = replace_once(
    s,
    "    frameSpectralReductionDb.store (0.0f);",
    "    frameSpectralReductionDb.store (0.0f);\n"
    "    frameDetailProtection.store (0.0f);\n"
    "    frameTailProtection.store (0.0f);",
    "clear P3 telemetry",
)

s = replace_once(
    s,
    "    result.spectralReductionDb =\n"
    "        frameSpectralReductionDb.load();\n\n"
    "    return result;",
    "    result.spectralReductionDb =\n"
    "        frameSpectralReductionDb.load();\n"
    "    result.detailProtection =\n"
    "        frameDetailProtection.load();\n"
    "    result.tailProtection =\n"
    "        frameTailProtection.load();\n\n"
    "    return result;",
    "read P3 telemetry",
)

s = replace_once(
    s,
    "        window[static_cast<size_t> (i)] =\n"
    "            std::sqrt (juce::jmax (0.0f, hann));\n"
    "    }\n}",
    "        window[static_cast<size_t> (i)] =\n"
    "            std::sqrt (juce::jmax (0.0f, hann));\n"
    "    }\n\n"
    "    for (int i = 0; i < detailFftSize; ++i)\n"
    "    {\n"
    "        const auto hann =\n"
    "            0.5f\n"
    "            - 0.5f * std::cos (\n"
    "                juce::MathConstants<float>::twoPi\n"
    "                * static_cast<float> (i)\n"
    "                / static_cast<float> (detailFftSize));\n"
    "        detailWindow[static_cast<size_t> (i)] =\n"
    "            std::sqrt (juce::jmax (0.0f, hann));\n"
    "    }\n}",
    "detail window",
)

insert_before_process_frame = r'''
float SmartDenoiseEngine::detailProtectionForPrimaryBin (int bin) const noexcept
{
    const float detailPosition =
        static_cast<float> (bin * detailFftSize)
        / static_cast<float> (fftSize);
    const int lower = juce::jlimit (1, detailBins - 2,
                                    static_cast<int> (detailPosition));
    const int upper = juce::jmin (detailBins - 2, lower + 1);
    const float fraction = juce::jlimit (0.0f, 1.0f,
                                         detailPosition - static_cast<float> (lower));
    return juce::jlimit (0.0f, 1.0f,
        detailProtectionState[static_cast<size_t> (lower)]
        + fraction * (detailProtectionState[static_cast<size_t> (upper)]
                    - detailProtectionState[static_cast<size_t> (lower)]));
}

float SmartDenoiseEngine::tailProtectionForPrimaryBin (int bin) const noexcept
{
    const float detailPosition =
        static_cast<float> (bin * detailFftSize)
        / static_cast<float> (fftSize);
    const int lower = juce::jlimit (1, detailBins - 2,
                                    static_cast<int> (detailPosition));
    const int upper = juce::jmin (detailBins - 2, lower + 1);
    const float fraction = juce::jlimit (0.0f, 1.0f,
                                         detailPosition - static_cast<float> (lower));
    return juce::jlimit (0.0f, 1.0f,
        detailTailMemory[static_cast<size_t> (lower)]
        + fraction * (detailTailMemory[static_cast<size_t> (upper)]
                    - detailTailMemory[static_cast<size_t> (lower)]));
}

void SmartDenoiseEngine::processDetailFrame() noexcept
{
    const int rightChannel = juce::jmin (1, channelCount - 1);
    std::array<float, detailBins> linkedMagnitude {};

    for (int channel = 0; channel < channelCount; ++channel)
    {
        auto& work = detailWork[static_cast<size_t> (channel)];
        std::fill (work.begin(), work.end(), 0.0f);

        for (int i = 0; i < detailFftSize; ++i)
        {
            int sourceIndex = detailInputWritePos + i;
            if (sourceIndex >= detailFftSize)
                sourceIndex -= detailFftSize;
            work[static_cast<size_t> (i)] =
                detailInputRing[static_cast<size_t> (channel)]
                               [static_cast<size_t> (sourceIndex)]
                * detailWindow[static_cast<size_t> (i)];
        }

        detailFft.forward (work.data());
    }

    for (int bin = 0; bin < detailBins; ++bin)
    {
        const auto index = static_cast<size_t> (bin);
        const auto magnitudeFor = [&] (int channel)
        {
            const auto& work = detailWork[static_cast<size_t> (channel)];
            const float real = work[static_cast<size_t> (bin * 2)];
            const float imaginary = work[static_cast<size_t> (bin * 2 + 1)];
            return std::sqrt (real * real + imaginary * imaginary + kFloor);
        };

        linkedMagnitude[index] =
            0.5f * (magnitudeFor (0) + magnitudeFor (rightChannel));
    }

    const float tailDecay = std::exp (
        -static_cast<float> (detailHopSize)
        / static_cast<float> (sampleRate * 0.240));

    for (int bin = 1; bin < detailBins - 1; ++bin)
    {
        const auto index = static_cast<size_t> (bin);
        const float current = linkedMagnitude[index];
        const float previous = detailPreviousMagnitude[index];
        const float riseRatio = juce::jmax (0.0f, current - previous)
                              / (previous + 0.10f * current + 1.0e-9f);
        const float transient = smoothStep ((riseRatio - 0.06f) / 1.10f);

        const float neighbour = 0.5f
            * (linkedMagnitude[static_cast<size_t> (bin - 1)]
             + linkedMagnitude[static_cast<size_t> (bin + 1)]);
        const float tonality = smoothStep (
            (current / (neighbour + 1.0e-9f) - 1.10f) / 1.45f);
        const float tonalAttack = tonality
            * smoothStep ((riseRatio - 0.015f) / 0.55f);
        const float instant = juce::jlimit (0.0f, 1.0f,
                                            juce::jmax (transient, 0.76f * tonalAttack));

        auto& state = detailProtectionState[index];
        const float alpha = instant > state ? 0.78f : 0.20f;
        state += alpha * (instant - state);

        auto& tail = detailTailMemory[index];
        tail = juce::jmax (state, tail * tailDecay);
        detailPreviousMagnitude[index] = current;
    }
}

'''

s = replace_once(
    s,
    "void SmartDenoiseEngine::processFrame() noexcept\n{",
    insert_before_process_frame + "void SmartDenoiseEngine::processFrame() noexcept\n{",
    "P3 detail analysis implementation",
)

s = replace_once(
    s,
    "        const float localTransient =\n"
    "            juce::jmax (\n"
    "                0.64f * transientScore,\n"
    "                binTransientProbability[\n"
    "                    static_cast<size_t> (bin)]);",
    "        const float detailGuard =\n"
    "            detailProtectionForPrimaryBin (bin);\n"
    "        const float tailGuard =\n"
    "            tailProtectionForPrimaryBin (bin);\n"
    "        const float localTransient =\n"
    "            juce::jmax (\n"
    "                juce::jmax (\n"
    "                    0.64f * transientScore,\n"
    "                    binTransientProbability[\n"
    "                        static_cast<size_t> (bin)]),\n"
    "                juce::jmax (detailGuard, 0.55f * tailGuard));",
    "map Detail Guard into primary decisions",
)

consensus_anchor = "    float reductionWeight = 0.0f;\n    float reductionDbSum = 0.0f;"
consensus_code = r'''    float p3ProtectionWeight = 0.0f;
    float p3DetailSum = 0.0f;
    float p3TailSum = 0.0f;

    // P3 three-frame gain consensus only lifts isolated deep holes; it never
    // creates extra attenuation. Short-window detail/tail protection relaxes
    // the consensus so attacks can return immediately toward unity.
    for (int bin = 1; bin < bins - 1; ++bin)
    {
        const auto index = static_cast<size_t> (bin);
        const float current = smoothedGains[index];
        const float historyOne = gainHistoryOne[index];
        const float historyTwo = gainHistoryTwo[index];
        const float minimum = juce::jmin (current, juce::jmin (historyOne, historyTwo));
        const float maximum = juce::jmax (current, juce::jmax (historyOne, historyTwo));
        const float median = current + historyOne + historyTwo - minimum - maximum;
        const float lifted = juce::jmax (current, median);
        const float detailGuard = detailProtectionForPrimaryBin (bin);
        const float tailGuard = tailProtectionForPrimaryBin (bin);
        const float wantedGuard = juce::jlimit (0.0f, 1.0f,
                                                juce::jmax (detailGuard, 0.65f * tailGuard));
        const float consensusStrength = 0.86f * (1.0f - wantedGuard);
        smoothedGains[index] = current + consensusStrength * (lifted - current);
        gainHistoryTwo[index] = historyOne;
        gainHistoryOne[index] = current;

        const float weight = detectorFrequencyWeight (bin);
        if (weight > 0.0f)
        {
            p3ProtectionWeight += weight;
            p3DetailSum += weight * detailGuard;
            p3TailSum += weight * tailGuard;
        }
    }

    float reductionWeight = 0.0f;
    float reductionDbSum = 0.0f;'''

s = replace_once(s, consensus_anchor, consensus_code, "P3 gain consensus")

s = replace_once(
    s,
    "        const float programPresence =\n"
    "            juce::jlimit (\n"
    "                0.0f,\n"
    "                1.0f,\n"
    "                0.52f * excessScore\n"
    "                + 0.35f * occupancyScore\n"
    "                + 0.13f * structureScore);",
    "        const float averageDetailProtection =\n"
    "            p3ProtectionWeight > 0.0f ? p3DetailSum / p3ProtectionWeight : 0.0f;\n"
    "        const float averageTailProtection =\n"
    "            p3ProtectionWeight > 0.0f ? p3TailSum / p3ProtectionWeight : 0.0f;\n"
    "        const float programPresence =\n"
    "            juce::jlimit (\n"
    "                0.0f,\n"
    "                1.0f,\n"
    "                juce::jmax (\n"
    "                    0.52f * excessScore\n"
    "                    + 0.35f * occupancyScore\n"
    "                    + 0.13f * structureScore,\n"
    "                    0.48f * averageTailProtection));",
    "P3 tail-aware program presence",
)

s = replace_once(
    s,
    "        frameSpectralReductionDb.store (\n"
    "            reductionWeight > 0.0f\n"
    "            ? reductionDbSum / reductionWeight\n"
    "            : 0.0f);",
    "        frameSpectralReductionDb.store (\n"
    "            reductionWeight > 0.0f\n"
    "            ? reductionDbSum / reductionWeight\n"
    "            : 0.0f);\n"
    "        frameDetailProtection.store (\n"
    "            p3ProtectionWeight > 0.0f ? p3DetailSum / p3ProtectionWeight : 0.0f);\n"
    "        frameTailProtection.store (\n"
    "            p3ProtectionWeight > 0.0f ? p3TailSum / p3ProtectionWeight : 0.0f);",
    "P3 telemetry publish",
)

s = replace_once(
    s,
    "void SmartDenoiseEngine::process (\n"
    "    juce::AudioBuffer<float>& buffer) noexcept\n"
    "{\n"
    "    reconfigureIfNeeded();\n\n"
    "    if (learnRequested.exchange (false))",
    "void SmartDenoiseEngine::process (\n"
    "    juce::AudioBuffer<float>& buffer) noexcept\n"
    "{\n"
    "    // Quality/FFT reconfiguration is setup-level work and must never run\n"
    "    // from the real-time audio callback. prepare() is the authority.\n"
    "    if (learnRequested.exchange (false))",
    "remove real-time reconfigure",
)

s = replace_once(
    s,
    "            inputRing[\n"
    "                static_cast<size_t> (channel)]\n"
    "                [static_cast<size_t> (\n"
    "                    inputWritePos)]\n"
    "                = spectralInput;",
    "            inputRing[\n"
    "                static_cast<size_t> (channel)]\n"
    "                [static_cast<size_t> (\n"
    "                    inputWritePos)]\n"
    "                = spectralInput;\n\n"
    "            detailInputRing[\n"
    "                static_cast<size_t> (channel)]\n"
    "                [static_cast<size_t> (detailInputWritePos)]\n"
    "                = spectralInput;",
    "feed detail ring",
)

s = replace_once(
    s,
    "            inputRing[1][\n"
    "                static_cast<size_t> (\n"
    "                    inputWritePos)]\n"
    "                = inputRing[0][\n"
    "                    static_cast<size_t> (\n"
    "                        inputWritePos)];",
    "            inputRing[1][\n"
    "                static_cast<size_t> (\n"
    "                    inputWritePos)]\n"
    "                = inputRing[0][\n"
    "                    static_cast<size_t> (\n"
    "                        inputWritePos)];\n\n"
    "            detailInputRing[1][static_cast<size_t> (detailInputWritePos)] =\n"
    "                detailInputRing[0][static_cast<size_t> (detailInputWritePos)];",
    "mono detail copy",
)

s = replace_once(
    s,
    "        if (++inputWritePos >= fftSize)\n"
    "            inputWritePos = 0;\n\n"
    "        ++samplesSeen;\n"
    "        ++hopCounter;",
    "        if (++inputWritePos >= fftSize)\n"
    "            inputWritePos = 0;\n"
    "        if (++detailInputWritePos >= detailFftSize)\n"
    "            detailInputWritePos = 0;\n\n"
    "        ++samplesSeen;\n"
    "        ++hopCounter;\n"
    "        ++detailSamplesSeen;\n"
    "        ++detailHopCounter;\n\n"
    "        if (detailSamplesSeen >= detailFftSize\n"
    "            && detailHopCounter >= detailHopSize)\n"
    "        {\n"
    "            detailHopCounter = 0;\n"
    "            processDetailFrame();\n"
    "        }",
    "schedule detail FFT",
)

HEADER.write_text(h, encoding="utf-8", newline="\n")
SOURCE.write_text(s, encoding="utf-8", newline="\n")
print("P3 patch anchors applied successfully")
