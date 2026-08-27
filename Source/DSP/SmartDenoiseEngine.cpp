#include "SmartDenoiseEngine.h"

#include <algorithm>
#include <cmath>

namespace smartdenoise
{
namespace
{
constexpr float kFloor = 1.0e-12f;
constexpr int kProfileMagic = 0x53444e32; // SDN2
constexpr int kProfileVersion = 2;

float dbToGain (float db) noexcept
{
    return std::pow (10.0f, db / 20.0f);
}

float gainToDb (float gain) noexcept
{
    return juce::Decibels::gainToDecibels (juce::jmax (gain, 1.0e-9f), -120.0f);
}

float powerRatioToDb (float ratio) noexcept
{
    return 10.0f * std::log10 (juce::jmax (ratio, 1.0e-12f));
}

float smoothStep (float x) noexcept
{
    x = juce::jlimit (0.0f, 1.0f, x);
    return x * x * (3.0f - 2.0f * x);
}

float robustCentre (
    std::array<float, SmartDenoiseEngine::profileGroupCount> values,
    int count) noexcept
{
    count = juce::jlimit (0, SmartDenoiseEngine::profileGroupCount, count);
    if (count <= 0)
        return 0.0f;

    for (int i = 1; i < count; ++i)
    {
        const float value = values[static_cast<size_t> (i)];
        int j = i - 1;
        while (j >= 0 && values[static_cast<size_t> (j)] > value)
        {
            values[static_cast<size_t> (j + 1)] =
                values[static_cast<size_t> (j)];
            --j;
        }
        values[static_cast<size_t> (j + 1)] = value;
    }

    if (count >= 5)
    {
        const int middle = count / 2;
        return (values[static_cast<size_t> (middle - 1)]
              + values[static_cast<size_t> (middle)]
              + values[static_cast<size_t> (
                    juce::jmin (count - 1, middle + 1))]) / 3.0f;
    }

    if ((count & 1) != 0)
        return values[static_cast<size_t> (count / 2)];

    return 0.5f * (
        values[static_cast<size_t> (count / 2 - 1)]
      + values[static_cast<size_t> (count / 2)]);
}
} // namespace

void SmartDenoiseEngine::FixedFft::configure (int newSize) noexcept
{
    if (newSize >= 2048)
        size = 2048;
    else if (newSize >= 1024)
        size = 1024;
    else
        size = 512;

    stages = 0;
    for (int value = size; value > 1; value >>= 1)
        ++stages;

    for (int index = 0; index < size; ++index)
    {
        unsigned int value = static_cast<unsigned int> (index);
        unsigned int result = 0;
        for (int bit = 0; bit < stages; ++bit)
        {
            result = (result << 1u) | (value & 1u);
            value >>= 1u;
        }
        reversed[static_cast<size_t> (index)] =
            static_cast<int> (result);
    }

    for (int index = 0; index < size / 2; ++index)
    {
        const float angle =
            -juce::MathConstants<float>::twoPi
            * static_cast<float> (index)
            / static_cast<float> (size);

        cosine[static_cast<size_t> (index)] = std::cos (angle);
        sine[static_cast<size_t> (index)] = std::sin (angle);
    }
}

void SmartDenoiseEngine::FixedFft::forward (
    float* work) const noexcept
{
    for (int index = size - 1; index >= 0; --index)
    {
        work[index * 2] = work[index];
        work[index * 2 + 1] = 0.0f;
    }

    for (int index = 0; index < size; ++index)
    {
        const int other = reversed[static_cast<size_t> (index)];
        if (other <= index)
            continue;

        std::swap (work[index * 2], work[other * 2]);
        std::swap (work[index * 2 + 1], work[other * 2 + 1]);
    }

    for (int length = 2; length <= size; length <<= 1)
    {
        const int half = length >> 1;
        const int tableStep = size / length;

        for (int block = 0; block < size; block += length)
        {
            int table = 0;
            for (int offset = 0; offset < half;
                 ++offset, table += tableStep)
            {
                const int even = block + offset;
                const int odd = even + half;
                const float wr =
                    cosine[static_cast<size_t> (table)];
                const float wi =
                    sine[static_cast<size_t> (table)];

                const float oddReal = work[odd * 2];
                const float oddImag = work[odd * 2 + 1];
                const float tr = wr * oddReal - wi * oddImag;
                const float ti = wr * oddImag + wi * oddReal;

                const float er = work[even * 2];
                const float ei = work[even * 2 + 1];

                work[even * 2] = er + tr;
                work[even * 2 + 1] = ei + ti;
                work[odd * 2] = er - tr;
                work[odd * 2 + 1] = ei - ti;
            }
        }
    }
}

void SmartDenoiseEngine::FixedFft::inverse (
    float* work) const noexcept
{
    work[1] = 0.0f;
    work[size + 1] = 0.0f;

    for (int bin = 1; bin < size / 2; ++bin)
    {
        work[(size - bin) * 2] = work[bin * 2];
        work[(size - bin) * 2 + 1] = -work[bin * 2 + 1];
    }

    for (int index = 0; index < size; ++index)
        work[index * 2 + 1] = -work[index * 2 + 1];

    for (int index = 0; index < size; ++index)
    {
        const int other = reversed[static_cast<size_t> (index)];
        if (other <= index)
            continue;

        std::swap (work[index * 2], work[other * 2]);
        std::swap (work[index * 2 + 1], work[other * 2 + 1]);
    }

    for (int length = 2; length <= size; length <<= 1)
    {
        const int half = length >> 1;
        const int tableStep = size / length;

        for (int block = 0; block < size; block += length)
        {
            int table = 0;
            for (int offset = 0; offset < half;
                 ++offset, table += tableStep)
            {
                const int even = block + offset;
                const int odd = even + half;
                const float wr =
                    cosine[static_cast<size_t> (table)];
                const float wi =
                    sine[static_cast<size_t> (table)];

                const float oddReal = work[odd * 2];
                const float oddImag = work[odd * 2 + 1];
                const float tr = wr * oddReal - wi * oddImag;
                const float ti = wr * oddImag + wi * oddReal;

                const float er = work[even * 2];
                const float ei = work[even * 2 + 1];

                work[even * 2] = er + tr;
                work[even * 2 + 1] = ei + ti;
                work[odd * 2] = er - tr;
                work[odd * 2 + 1] = ei - ti;
            }
        }
    }

    const float scale = 1.0f / static_cast<float> (size);
    for (int index = 0; index < size; ++index)
        work[index] = work[index * 2] * scale;
}

void SmartDenoiseEngine::SubsonicFilter::prepare (
    double newSampleRate, float cutoffHz) noexcept
{
    const auto safeRate =
        static_cast<float> (
            newSampleRate > 0.0 ? newSampleRate : 48000.0);

    const auto safeCutoff =
        juce::jlimit (5.0f, 45.0f, cutoffHz);

    coefficient = std::exp (
        -juce::MathConstants<float>::twoPi
        * safeCutoff / safeRate);

    reset();
}

void SmartDenoiseEngine::SubsonicFilter::reset() noexcept
{
    previousInput1 = previousOutput1 = 0.0f;
    previousInput2 = previousOutput2 = 0.0f;
}

float SmartDenoiseEngine::SubsonicFilter::process (
    float input) noexcept
{
    const float stage1 =
        coefficient
        * (previousOutput1 + input - previousInput1);

    previousInput1 = input;
    previousOutput1 = stage1;

    const float stage2 =
        coefficient
        * (previousOutput2 + stage1 - previousInput2);

    previousInput2 = stage1;
    previousOutput2 = stage2;

    return std::isfinite (stage2) ? stage2 : 0.0f;
}

SmartDenoiseEngine::SmartDenoiseEngine()
{
    fixedFft.configure (1024);
    detailFft.configure (detailFftSize);
    resetSpectralState();
    clearFrameAnalysis();
}

void SmartDenoiseEngine::prepare (
    double newSampleRate, int, int channels)
{
    sampleRate =
        newSampleRate > 0.0 ? newSampleRate : 48000.0;

    channelCount =
        juce::jlimit (1, maxChannels, channels);

    qualityDirty.store (true);
    reconfigureIfNeeded();

    for (auto& filter : subsonicFilters)
        filter.prepare (sampleRate, 24.0f);

    reset();
}

void SmartDenoiseEngine::resetSpectralState() noexcept
{
    linkedGainState.fill (1.0f);
    previousFrequencyGain.fill (1.0f);
    posteriorSnrState.fill (0.0f);
    tonalityState.fill (0.0f);
    linkedPreviousMagnitude.fill (0.0f);
    binTransientProbability.fill (0.0f);
    gainHistoryOne.fill (1.0f);
    gainHistoryTwo.fill (1.0f);
    detailPreviousMagnitude.fill (0.0f);
    detailProtectionState.fill (0.0f);
    detailTailMemory.fill (0.0f);
}

void SmartDenoiseEngine::reset() noexcept
{
    for (auto& channel : inputRing)
        channel.fill (0.0f);
    for (auto& channel : outputRing)
        channel.fill (0.0f);
    for (auto& channel : dryDelayRing)
        channel.fill (0.0f);
    for (auto& channel : fftWork)
        channel.fill (0.0f);
    for (auto& channel : currentPower)
        channel.fill (0.0f);
    for (auto& channel : previousMagnitude)
        channel.fill (0.0f);
    for (auto& channel : detailInputRing)
        channel.fill (0.0f);
    for (auto& channel : detailWork)
        channel.fill (0.0f);

    resetSpectralState();

    for (auto& filter : subsonicFilters)
        filter.reset();

    inputWritePos = 0;
    outputReadPos = 0;
    hopCounter = 0;
    samplesSeen = 0;
    detailInputWritePos = 0;
    detailHopCounter = 0;
    detailSamplesSeen = 0;

    learningFramesCaptured = 0;
    learningFramesAccepted = 0;
    learningFramesRejected.store (0);

    expanderGainDbState = 0.0f;
    expanderHoldSamplesRemaining = 0;
    expanderOpenState = true;
    expanderGainReductionDb.store (0.0f);

    clearFrameAnalysis();
}

void SmartDenoiseEngine::clearFrameAnalysis() noexcept
{
    frameWeightedExcessDb.store (0.0f);
    frameActiveBandRatio.store (0.0f);
    frameTransientProbability.store (0.0f);
    frameHarmonicProbability.store (0.0f);
    frameTonalNoiseProbability.store (0.0f);
    frameProgramPresence.store (1.0f);
    frameResidualNoiseDb.store (-120.0f);
    frameSpectralReductionDb.store (0.0f);
    frameDetailProtection.store (0.0f);
    frameTailProtection.store (0.0f);
}

void SmartDenoiseEngine::invalidateProfile() noexcept
{
    profileValid.store (false);
    profileQuality.store (0.0f);
    profileSampleRate = 0.0;
    profileFftSize = 0;
    profileChannels = 0;
    for (auto& value : profileDisplay)
        value.store (0.0f, std::memory_order_relaxed);

    resetSpectralState();
    clearFrameAnalysis();
}

void SmartDenoiseEngine::clearProfile() noexcept
{
    learnRequested.store (false);
    learningActive.store (false);
    learningProgress.store (0.0f);
    profileValidBeforeLearning = false;
    lastLearnRejected.store (false);
    invalidateProfile();
}

NoiseFrameAnalysis
SmartDenoiseEngine::getFrameAnalysis() const noexcept
{
    NoiseFrameAnalysis result;
    result.profileReady =
        profileValid.load() && ! learningActive.load();

    result.weightedExcessDb =
        frameWeightedExcessDb.load();
    result.activeBandRatio =
        frameActiveBandRatio.load();
    result.transientProbability =
        frameTransientProbability.load();
    result.harmonicProbability =
        frameHarmonicProbability.load();
    result.tonalNoiseProbability =
        frameTonalNoiseProbability.load();
    result.programPresence =
        frameProgramPresence.load();
    result.residualNoiseDb =
        frameResidualNoiseDb.load();
    result.spectralReductionDb =
        frameSpectralReductionDb.load();
    result.detailProtection =
        frameDetailProtection.load();
    result.tailProtection =
        frameTailProtection.load();

    return result;
}

std::array<float, SmartDenoiseEngine::profileDisplayBins>
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

void SmartDenoiseEngine::setQuality (
    Quality quality) noexcept
{
    if (requestedQuality.exchange (quality) != quality)
        qualityDirty.store (true);
}

void SmartDenoiseEngine::setReductionDb (
    float value) noexcept
{
    reductionDb.store (
        juce::jlimit (0.0f, maxReductionDb, value));
}

void SmartDenoiseEngine::setPreserve (
    float value01) noexcept
{
    preserve.store (
        juce::jlimit (0.0f, 1.0f, value01));
}

void SmartDenoiseEngine::setThresholdOffsetDb (
    float value) noexcept
{
    thresholdOffsetDb.store (
        juce::jlimit (-6.0f, 12.0f, value));
}

void SmartDenoiseEngine::setSilenceAmount (
    float value01) noexcept
{
    silenceAmount.store (
        juce::jlimit (0.0f, 1.0f, value01));
}

void SmartDenoiseEngine::startLearning (
    double seconds) noexcept
{
    learnSecondsRequested.store (
        juce::jlimit (1.0, 6.0, seconds));
    learnRequested.store (true);
}

juce::String SmartDenoiseEngine::serialiseProfile() const
{
    if (! profileValid.load()
        || learningActive.load())
        return {};

    const int bins = profileFftSize / 2 + 1;
    if (bins <= 1 || bins > maxBins)
        return {};

    juce::MemoryOutputStream stream;

    stream.writeInt (kProfileMagic);
    stream.writeInt (kProfileVersion);
    stream.writeDouble (profileSampleRate);
    stream.writeInt (profileFftSize);
    stream.writeInt (profileChannels);
    stream.writeFloat (estimatedNoiseFloorDb.load());
    stream.writeFloat (profileQuality.load());
    stream.writeInt (bins);

    for (int channel = 0;
         channel < maxChannels; ++channel)
        for (int bin = 0; bin < bins; ++bin)
            stream.writeFloat (
                profilePower[static_cast<size_t> (channel)]
                            [static_cast<size_t> (bin)]);

    for (int channel = 0;
         channel < maxChannels; ++channel)
        for (int bin = 0; bin < bins; ++bin)
            stream.writeFloat (
                profileVarianceDb2[
                    static_cast<size_t> (channel)]
                    [static_cast<size_t> (bin)]);

    return stream.getMemoryBlock().toBase64Encoding();
}

bool SmartDenoiseEngine::restoreProfile (
    const juce::String& encodedState)
{
    if (encodedState.isEmpty()
        || learningActive.load())
        return false;

    juce::MemoryBlock block;
    if (! block.fromBase64Encoding (encodedState))
        return false;

    juce::MemoryInputStream input (block, false);

    if (input.readInt() != kProfileMagic
        || input.readInt() != kProfileVersion)
        return false;

    const double savedRate = input.readDouble();
    const int savedFftSize = input.readInt();
    const int savedChannels = input.readInt();
    const float savedFloorDb = input.readFloat();
    const float savedQuality = input.readFloat();
    const int bins = input.readInt();

    if (! std::isfinite (savedRate)
        || std::abs (savedRate - sampleRate) > 0.5
        || savedFftSize != fftSize
        || savedChannels != channelCount
        || bins != fftSize / 2 + 1
        || bins <= 1
        || bins > maxBins
        || ! std::isfinite (savedFloorDb)
        || ! std::isfinite (savedQuality))
        return false;

    std::array<
        std::array<float, maxBins>, maxChannels>
        restoredPower {};

    std::array<
        std::array<float, maxBins>, maxChannels>
        restoredVariance {};

    for (int channel = 0;
         channel < maxChannels; ++channel)
    {
        for (int bin = 0; bin < bins; ++bin)
        {
            const float value = input.readFloat();
            if (! std::isfinite (value)
                || value < 0.0f)
                return false;

            restoredPower[
                static_cast<size_t> (channel)]
                [static_cast<size_t> (bin)] = value;
        }
    }

    for (int channel = 0;
         channel < maxChannels; ++channel)
    {
        for (int bin = 0; bin < bins; ++bin)
        {
            const float value = input.readFloat();
            if (! std::isfinite (value)
                || value < 0.0f)
                return false;

            restoredVariance[
                static_cast<size_t> (channel)]
                [static_cast<size_t> (bin)] = value;
        }
    }

    profilePower = restoredPower;
    profileVarianceDb2 = restoredVariance;

    estimatedNoiseFloorDb.store (
        juce::jlimit (-120.0f, 0.0f, savedFloorDb));

    profileQuality.store (
        juce::jlimit (0.0f, 1.0f, savedQuality));

    profileSampleRate = savedRate;
    profileFftSize = savedFftSize;
    profileChannels = savedChannels;
    publishProfileDisplay();

    lastLearnRejected.store (false);
    profileValid.store (true);

    resetSpectralState();
    clearFrameAnalysis();

    return true;
}

void SmartDenoiseEngine::reconfigureIfNeeded() noexcept
{
    if (! qualityDirty.exchange (false))
        return;

    const auto quality = requestedQuality.load();

    fftSize =
        quality == Quality::clean2048 ? 2048 : 1024;
    hopSize = fftSize / 2;

    fixedFft.configure (fftSize);
    activeFftSize.store (fftSize);

    const bool compatibleProfile =
        profileValid.load()
        && profileFftSize == fftSize
        && profileChannels == channelCount
        && std::abs (profileSampleRate - sampleRate) <= 0.5;

    if (! compatibleProfile)
        invalidateProfile();

    learningActive.store (false);
    learningProgress.store (0.0f);
    lastLearnRejected.store (false);

    resetSpectralState();
    clearFrameAnalysis();

    for (int i = 0; i < fftSize; ++i)
    {
        const auto hann =
            0.5f
            - 0.5f * std::cos (
                juce::MathConstants<float>::twoPi
                * static_cast<float> (i)
                / static_cast<float> (fftSize));

        window[static_cast<size_t> (i)] =
            std::sqrt (juce::jmax (0.0f, hann));
    }

    for (int i = 0; i < detailFftSize; ++i)
    {
        const auto hann =
            0.5f
            - 0.5f * std::cos (
                juce::MathConstants<float>::twoPi
                * static_cast<float> (i)
                / static_cast<float> (detailFftSize));
        detailWindow[static_cast<size_t> (i)] =
            std::sqrt (juce::jmax (0.0f, hann));
    }
}

void SmartDenoiseEngine::beginLearningOnAudioThread() noexcept
{
    learningFramesTarget =
        juce::jmax (
            1,
            static_cast<int> (
                std::ceil (
                    learnSecondsRequested.load()
                    * sampleRate
                    / static_cast<double> (hopSize))));

    learningFramesCaptured = 0;
    learningFramesAccepted = 0;
    learningFramesRejected.store (0);

    learningBaselineDb = -120.0f;
    learningDbMean = 0.0;
    learningDbM2 = 0.0;

    profileValidBeforeLearning =
        profileValid.load();

    for (auto& channel : learningGroupPowerSum)
        for (auto& group : channel)
            group.fill (0.0);

    learningGroupAcceptedFrames.fill (0);
    learningGroupBroadbandPowerSum.fill (0.0);

    learningProgress.store (0.0f);
    learningActive.store (true);

    // Freeze old profile out of the detector while learning,
    // but remember whether a valid profile existed so a bad
    // new Learn can fall back to it.
    profileValid.store (false);
    lastLearnRejected.store (false);

    resetSpectralState();
    clearFrameAnalysis();
}

void SmartDenoiseEngine::accumulateLearningFrame (
    float framePower,
    float transientScore) noexcept
{
    if (! learningActive.load())
        return;

    const int frameIndex = learningFramesCaptured;
    ++learningFramesCaptured;

    const float frameDb =
        gainToDb (
            std::sqrt (
                juce::jmax (
                    framePower, 1.0e-16f)));

    const bool warmup = frameIndex < 4;

    if (frameIndex == 0)
        learningBaselineDb = frameDb;

    const float levelDeltaDb =
        frameDb - learningBaselineDb;

    const bool invalidFrame =
        ! std::isfinite (framePower)
        || framePower < 0.0f;

    // Stationary stochastic noise naturally has high frame-to-frame spectral
    // flux. Treat flux as capture contamination only when it arrives with a
    // meaningful broadband level rise; otherwise real hiss/fan captures would
    // be rejected even though their long-term spectrum is stable.
    const bool transientContamination =
        ! warmup
        && transientScore > 0.82f
        && levelDeltaDb > 2.5f;

    const bool levelContamination =
        ! warmup
        && levelDeltaDb > 7.0f;

    const bool reject =
        invalidFrame
        || transientContamination
        || levelContamination;

    if (! reject)
    {
        const int group =
            juce::jlimit (
                0,
                profileGroupCount - 1,
                frameIndex * profileGroupCount
                / juce::jmax (
                    1, learningFramesTarget));

        for (int channel = 0;
             channel < channelCount; ++channel)
        {
            for (int bin = 0;
                 bin < fftSize / 2 + 1; ++bin)
            {
                learningGroupPowerSum[
                    static_cast<size_t> (channel)]
                    [static_cast<size_t> (group)]
                    [static_cast<size_t> (bin)]
                    += currentPower[
                        static_cast<size_t> (channel)]
                        [static_cast<size_t> (bin)];
            }
        }

        ++learningGroupAcceptedFrames[
            static_cast<size_t> (group)];

        learningGroupBroadbandPowerSum[
            static_cast<size_t> (group)]
            += framePower;

        ++learningFramesAccepted;

        const double delta =
            static_cast<double> (frameDb)
            - learningDbMean;

        learningDbMean +=
            delta
            / static_cast<double> (
                learningFramesAccepted);

        learningDbM2 +=
            delta
            * (static_cast<double> (frameDb)
               - learningDbMean);

        const float baselineAlpha =
            frameDb < learningBaselineDb
            ? 0.14f
            : 0.025f;

        learningBaselineDb +=
            baselineAlpha
            * (frameDb - learningBaselineDb);
    }
    else
    {
        learningFramesRejected.fetch_add (1);
    }

    learningProgress.store (
        juce::jlimit (
            0.0f,
            1.0f,
            static_cast<float> (
                learningFramesCaptured)
            / static_cast<float> (
                learningFramesTarget)));

    if (learningFramesCaptured >= learningFramesTarget)
        finaliseLearningOnAudioThread();
}

void SmartDenoiseEngine::finaliseLearningOnAudioThread() noexcept
{
    const int bins = fftSize / 2 + 1;

    int populatedGroups = 0;
    for (int group = 0;
         group < profileGroupCount; ++group)
    {
        if (learningGroupAcceptedFrames[
                static_cast<size_t> (group)] > 0)
            ++populatedGroups;
    }

    const int minimumAccepted =
        juce::jmax (
            12,
            learningFramesTarget / 3);

    const bool enoughData =
        learningFramesAccepted >= minimumAccepted
        && populatedGroups >= 4;

    std::array<
        std::array<float, maxBins>, maxChannels>
        candidatePower {};

    std::array<
        std::array<float, maxBins>, maxChannels>
        candidateVariance {};

    float spectralSpreadSum = 0.0f;
    float spectralWeightSum = 0.0f;

    if (enoughData)
    {
        for (int channel = 0;
             channel < channelCount; ++channel)
        {
            for (int bin = 0; bin < bins; ++bin)
            {
                std::array<
                    float, profileGroupCount>
                    groupMeans {};

                int count = 0;

                for (int group = 0;
                     group < profileGroupCount;
                     ++group)
                {
                    const int frames =
                        learningGroupAcceptedFrames[
                            static_cast<size_t> (group)];

                    if (frames <= 0)
                        continue;

                    groupMeans[
                        static_cast<size_t> (count++)]
                        = static_cast<float> (
                            learningGroupPowerSum[
                                static_cast<size_t> (
                                    channel)]
                                [static_cast<size_t> (
                                    group)]
                                [static_cast<size_t> (
                                    bin)]
                            / static_cast<double> (
                                frames));
                }

                const float centre =
                    juce::jmax (
                        kFloor,
                        robustCentre (
                            groupMeans, count));

                candidatePower[
                    static_cast<size_t> (channel)]
                    [static_cast<size_t> (bin)]
                    = centre;

                float varianceDb2 = 0.0f;

                for (int index = 0;
                     index < count; ++index)
                {
                    const float diffDb =
                        juce::jlimit (
                            -24.0f,
                            24.0f,
                            powerRatioToDb (
                                (groupMeans[
                                    static_cast<size_t> (
                                        index)]
                                 + kFloor)
                                / (centre + kFloor)));

                    varianceDb2 +=
                        diffDb * diffDb;
                }

                if (count > 0)
                    varianceDb2 /=
                        static_cast<float> (count);

                candidateVariance[
                    static_cast<size_t> (channel)]
                    [static_cast<size_t> (bin)]
                    = varianceDb2;
            }
        }

        if (channelCount == 1)
        {
            candidatePower[1] = candidatePower[0];
            candidateVariance[1] =
                candidateVariance[0];
        }

        for (int bin = 1; bin < bins - 1; ++bin)
        {
            const float weight =
                detectorFrequencyWeight (bin);

            if (weight <= 0.0f)
                continue;

            const float variance =
                0.5f
                * (candidateVariance[0][
                        static_cast<size_t> (bin)]
                   + candidateVariance[1][
                        static_cast<size_t> (bin)]);

            spectralSpreadSum +=
                weight
                * std::sqrt (
                    juce::jmax (
                        0.0f, variance));

            spectralWeightSum += weight;
        }
    }

    std::array<
        float, profileGroupCount>
        broadbandMeans {};

    int broadbandCount = 0;

    for (int group = 0;
         group < profileGroupCount; ++group)
    {
        const int frames =
            learningGroupAcceptedFrames[
                static_cast<size_t> (group)];

        if (frames <= 0)
            continue;

        broadbandMeans[
            static_cast<size_t> (broadbandCount++)]
            = static_cast<float> (
                learningGroupBroadbandPowerSum[
                    static_cast<size_t> (group)]
                / static_cast<double> (frames));
    }

    const float acceptedRatio =
        static_cast<float> (
            learningFramesAccepted)
        / static_cast<float> (
            juce::jmax (
                1, learningFramesCaptured));

    const float levelStdDb =
        learningFramesAccepted > 1
        ? static_cast<float> (
            std::sqrt (
                learningDbM2
                / static_cast<double> (
                    learningFramesAccepted - 1)))
        : 12.0f;

    const float spectralSpreadDb =
        spectralWeightSum > 0.0f
        ? spectralSpreadSum
          / spectralWeightSum
        : 12.0f;

    const float acceptedScore =
        juce::jlimit (
            0.0f,
            1.0f,
            (acceptedRatio - 0.35f) / 0.55f);

    const float levelScore =
        1.0f
        - juce::jlimit (
            0.0f,
            1.0f,
            levelStdDb / 6.0f);

    const float spectralScore =
        1.0f
        - juce::jlimit (
            0.0f,
            1.0f,
            spectralSpreadDb / 7.0f);

    const float candidateQuality =
        enoughData
        ? juce::jlimit (
            0.0f,
            1.0f,
            0.45f * acceptedScore
            + 0.30f * levelScore
            + 0.25f * spectralScore)
        : 0.0f;

    const bool acceptedProfile =
        enoughData
        && candidateQuality >= 0.25f
        && broadbandCount >= 4;

    if (acceptedProfile)
    {
        profilePower = candidatePower;
        profileVarianceDb2 = candidateVariance;

        const float broadbandPower =
            juce::jmax (
                1.0e-16f,
                robustCentre (
                    broadbandMeans,
                    broadbandCount));

        estimatedNoiseFloorDb.store (
            gainToDb (
                std::sqrt (broadbandPower)));

        profileQuality.store (
            candidateQuality);

        profileSampleRate = sampleRate;
        profileFftSize = fftSize;
        profileChannels = channelCount;
        publishProfileDisplay();

        profileValid.store (true);
        lastLearnRejected.store (false);
    }
    else
    {
        profileValid.store (
            profileValidBeforeLearning);

        if (! profileValidBeforeLearning)
            profileQuality.store (0.0f);

        lastLearnRejected.store (true);
    }

    learningActive.store (false);
    learningProgress.store (1.0f);

    resetSpectralState();
    clearFrameAnalysis();
}

float SmartDenoiseEngine::frequencyWeight (
    int bin) const noexcept
{
    const auto frequency =
        static_cast<float> (bin)
        * static_cast<float> (sampleRate)
        / static_cast<float> (fftSize);

    if (frequency < 18.0f)    return 0.0f;
    if (frequency < 80.0f)    return 0.82f;
    if (frequency < 300.0f)   return 0.34f;
    if (frequency < 4000.0f)  return 0.48f;
    if (frequency < 12000.0f) return 1.00f;
    if (frequency < 17000.0f) return 0.72f;
    return 0.42f;
}

float SmartDenoiseEngine::detectorFrequencyWeight (
    int bin) const noexcept
{
    const auto frequency =
        static_cast<float> (bin)
        * static_cast<float> (sampleRate)
        / static_cast<float> (fftSize);

    if (frequency < 50.0f)    return 0.0f;
    if (frequency < 100.0f)   return 0.25f;
    if (frequency < 250.0f)   return 0.55f;
    if (frequency < 12000.0f) return 1.0f;
    if (frequency < 16000.0f) return 0.75f;
    if (frequency < 20000.0f) return 0.35f;
    return 0.0f;
}

float SmartDenoiseEngine::calculateLinkedGain (
    int bin,
    float linkedPower,
    float localTonality,
    float transientProtect) noexcept
{
    if (! profileValid.load())
        return 1.0f;

    const auto index =
        static_cast<size_t> (bin);

    const float profile =
        0.5f
        * (profilePower[0][index]
           + profilePower[1][index]);

    const float varianceDb2 =
        0.5f
        * (profileVarianceDb2[0][index]
           + profileVarianceDb2[1][index]);

    const float profileStdDb =
        std::sqrt (
            juce::jmax (
                0.0f, varianceDb2));

    const float noiseConfidence =
        1.0f
        - smoothStep (
            (profileStdDb - 2.5f) / 7.5f);

    const float threshold =
        profile
        * std::pow (
            10.0f,
            thresholdOffsetDb.load() / 10.0f);

    const float posterior =
        juce::jlimit (
            1.0e-4f,
            10000.0f,
            linkedPower
            / (threshold + kFloor));

    const float instantaneousPrior =
        juce::jmax (
            0.0f,
            posterior - 1.0f);

    // P2 decision-directed prior SNR. New transients
    // shorten memory so attacks do not inherit an old
    // noise decision.
    const float ddAlpha =
        juce::jmap (
            juce::jlimit (
                0.0f, 1.0f, transientProtect),
            0.94f,
            0.62f);

    const float previousDecision =
        linkedGainState[index]
        * linkedGainState[index]
        * posteriorSnrState[index];

    const float prior =
        juce::jmax (
            0.0f,
            ddAlpha * previousDecision
            + (1.0f - ddAlpha)
              * instantaneousPrior);

    posteriorSnrState[index] = posterior;

    const float excessDb =
        juce::jlimit (
            -18.0f,
            36.0f,
            powerRatioToDb (
                (linkedPower + kFloor)
                / (profile + kFloor)));

    const float stableTonalNoise =
        juce::jlimit (
            0.0f,
            1.0f,
            localTonality
            * noiseConfidence
            * (1.0f
               - smoothStep (
                    (excessDb - 1.0f)
                    / 7.0f)));

    const float harmonicSignal =
        juce::jlimit (
            0.0f,
            1.0f,
            localTonality
            * smoothStep (
                (excessDb - 2.0f)
                / 7.5f));

    const float posteriorPresence =
        smoothStep (
            (powerRatioToDb (posterior)
             - 1.5f)
            / 9.0f);

    const float signalPresence =
        juce::jlimit (
            0.0f,
            1.0f,
            juce::jmax (
                posteriorPresence,
                juce::jmax (
                    0.94f * harmonicSignal,
                    0.98f * transientProtect)));

    const float requestedReduction =
        reductionDb.load();

    if (requestedReduction <= 0.0001f)
        return 1.0f;

    const float baseWiener =
        prior / (prior + 1.0f);

    const float strength =
        juce::jmap (
            requestedReduction,
            0.0f,
            maxReductionDb,
            0.68f,
            1.28f);

    const float randomGain =
        std::pow (
            juce::jlimit (
                0.0f,
                1.0f,
                baseWiener),
            strength);

    // Stable narrow-band energy close to the profile is
    // allowed to behave more like hum/buzz than a wanted
    // musical harmonic.
    const float tonalPriorScale =
        juce::jmap (
            stableTonalNoise,
            1.0f,
            0.38f);

    const float tonalPrior =
        prior * tonalPriorScale;

    const float tonalWiener =
        tonalPrior / (tonalPrior + 1.0f);

    const float tonalGain =
        std::pow (
            juce::jlimit (
                0.0f,
                1.0f,
                tonalWiener),
            strength * 1.08f);

    float target =
        randomGain
        + stableTonalNoise
          * (tonalGain - randomGain);

    const float weight =
        frequencyWeight (bin);

    // P2: finite confidence-aware floor. Frequency
    // weighting is applied once to the final target.
    float floorReductionDb =
        requestedReduction;

    floorReductionDb *=
        0.68f + 0.32f * noiseConfidence;

    floorReductionDb *=
        1.0f + 0.08f * stableTonalNoise;

    floorReductionDb *=
        1.0f
        - 0.52f
          * signalPresence
          * preserve.load();

    floorReductionDb =
        juce::jlimit (
            0.0f,
            requestedReduction * 1.08f,
            floorReductionDb);

    const float spectralFloor =
        dbToGain (-floorReductionDb);

    target =
        juce::jmax (
            spectralFloor, target);

    target =
        1.0f
        - weight
          * (1.0f - target);

    // Low-confidence learned bins get pulled toward unity.
    target +=
        (1.0f - target)
        * (1.0f - noiseConfidence)
        * 0.25f;

    const float protectAmount =
        preserve.load();

    const float detailProtection =
        juce::jlimit (
            0.0f,
            0.96f,
            harmonicSignal
                * (0.18f
                   + 0.70f * protectAmount)
            + transientProtect
                * (0.24f
                   + 0.68f * protectAmount)
            + posteriorPresence
                * 0.10f
                * protectAmount);

    target +=
        (1.0f - target)
        * detailProtection;

    return juce::jlimit (
        spectralFloor,
        1.0f,
        target);
}


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

void SmartDenoiseEngine::processFrame() noexcept
{
    const int bins = fftSize / 2 + 1;
    const int rightChannel =
        juce::jmin (1, channelCount - 1);

    float totalFlux = 0.0f;
    float totalMagnitude = 0.0f;
    double frameEnergy = 0.0;

    for (int channel = 0;
         channel < channelCount; ++channel)
    {
        auto& work =
            fftWork[static_cast<size_t> (channel)];

        std::fill (
            work.begin(), work.end(), 0.0f);

        for (int i = 0; i < fftSize; ++i)
        {
            int sourceIndex =
                inputWritePos + i;

            if (sourceIndex >= fftSize)
                sourceIndex -= fftSize;

            const float input =
                inputRing[
                    static_cast<size_t> (channel)]
                    [static_cast<size_t> (sourceIndex)];

            frameEnergy +=
                static_cast<double> (input)
                * input;

            work[static_cast<size_t> (i)] =
                input
                * window[static_cast<size_t> (i)];
        }

        fixedFft.forward (work.data());

        for (int bin = 0; bin < bins; ++bin)
        {
            const float real =
                work[static_cast<size_t> (bin * 2)];

            const float imaginary =
                work[static_cast<size_t> (
                    bin * 2 + 1)];

            const float power =
                real * real
                + imaginary * imaginary;

            const float magnitude =
                std::sqrt (power + kFloor);

            currentPower[
                static_cast<size_t> (channel)]
                [static_cast<size_t> (bin)]
                = power;

            const float previous =
                previousMagnitude[
                    static_cast<size_t> (channel)]
                    [static_cast<size_t> (bin)];

            totalFlux +=
                juce::jmax (
                    0.0f,
                    magnitude - previous);

            totalMagnitude += magnitude;

            previousMagnitude[
                static_cast<size_t> (channel)]
                [static_cast<size_t> (bin)]
                = magnitude;
        }
    }

    const float transientScore =
        juce::jlimit (
            0.0f,
            1.0f,
            (totalFlux
             / (totalMagnitude + 1.0e-9f)
             - 0.018f)
            * 11.0f);

    // Per-bin transient state.
    for (int bin = 1; bin < bins - 1; ++bin)
    {
        const float linkedPower =
            0.5f
            * (currentPower[0][
                    static_cast<size_t> (bin)]
               + currentPower[
                    static_cast<size_t> (
                        rightChannel)]
                    [static_cast<size_t> (bin)]);

        const float linkedMagnitude =
            std::sqrt (
                linkedPower + kFloor);

        const float previous =
            linkedPreviousMagnitude[
                static_cast<size_t> (bin)];

        const float riseRatio =
            juce::jmax (
                0.0f,
                linkedMagnitude - previous)
            / (previous
               + 0.08f * linkedMagnitude
               + 1.0e-9f);

        const float localTransient =
            smoothStep (
                (riseRatio - 0.12f)
                / 1.65f);

        auto& state =
            binTransientProbability[
                static_cast<size_t> (bin)];

        const float alpha =
            localTransient > state
            ? 0.72f
            : 0.22f;

        state +=
            alpha
            * (localTransient - state);

        linkedPreviousMagnitude[
            static_cast<size_t> (bin)]
            = linkedMagnitude;
    }

    if (learningActive.load())
    {
        const float framePower =
            static_cast<float> (
                frameEnergy
                / static_cast<double> (
                    juce::jmax (
                        1,
                        fftSize * channelCount)));

        accumulateLearningFrame (
            framePower,
            transientScore);
    }

    std::array<float, maxBins> rawGains {};
    std::array<float, maxBins> tonalities {};
    std::array<float, maxBins> smoothedGains {};
    std::array<float, maxBins> tonalNoiseLikelihood {};

    rawGains.fill (1.0f);
    smoothedGains.fill (1.0f);
    tonalities.fill (0.0f);
    tonalNoiseLikelihood.fill (0.0f);

    for (int bin = 1; bin < bins - 1; ++bin)
    {
        const float linkedPower =
            0.5f
            * (currentPower[0][
                    static_cast<size_t> (bin)]
               + currentPower[
                    static_cast<size_t> (
                        rightChannel)]
                    [static_cast<size_t> (bin)]);

        const float linkedMagnitude =
            std::sqrt (
                linkedPower + kFloor);

        const float neighbour =
            0.25f
            * (std::sqrt (
                    currentPower[0][
                        static_cast<size_t> (
                            bin - 1)]
                    + kFloor)
               + std::sqrt (
                    currentPower[0][
                        static_cast<size_t> (
                            bin + 1)]
                    + kFloor)
               + std::sqrt (
                    currentPower[
                        static_cast<size_t> (
                            rightChannel)]
                        [static_cast<size_t> (
                            bin - 1)]
                    + kFloor)
               + std::sqrt (
                    currentPower[
                        static_cast<size_t> (
                            rightChannel)]
                        [static_cast<size_t> (
                            bin + 1)]
                    + kFloor));

        const float instantTonality =
            smoothStep (
                (linkedMagnitude
                 / (neighbour + 1.0e-9f)
                 - 1.12f)
                / 1.65f);

        auto& tonalState =
            tonalityState[
                static_cast<size_t> (bin)];

        tonalState +=
            0.36f
            * (instantTonality - tonalState);

        const float tonality =
            juce::jlimit (
                0.0f, 1.0f, tonalState);

        tonalities[
            static_cast<size_t> (bin)]
            = tonality;

        const float detailGuard =
            detailProtectionForPrimaryBin (bin);
        const float tailGuard =
            tailProtectionForPrimaryBin (bin);
        const float localTransient =
            juce::jmax (
                juce::jmax (
                    0.64f * transientScore,
                    binTransientProbability[
                        static_cast<size_t> (bin)]),
                juce::jmax (detailGuard, 0.55f * tailGuard));

        rawGains[
            static_cast<size_t> (bin)]
            = calculateLinkedGain (
                bin,
                linkedPower,
                tonality,
                juce::jlimit (
                    0.0f,
                    1.0f,
                    localTransient));

        if (profileValid.load())
        {
            const float profile =
                0.5f
                * (profilePower[0][
                        static_cast<size_t> (bin)]
                   + profilePower[1][
                        static_cast<size_t> (bin)]);

            const float variance =
                0.5f
                * (profileVarianceDb2[0][
                        static_cast<size_t> (bin)]
                   + profileVarianceDb2[1][
                        static_cast<size_t> (bin)]);

            const float confidence =
                1.0f
                - smoothStep (
                    (std::sqrt (
                        juce::jmax (
                            0.0f,
                            variance))
                     - 2.5f)
                    / 7.5f);

            const float excessDb =
                powerRatioToDb (
                    (linkedPower + kFloor)
                    / (profile + kFloor));

            tonalNoiseLikelihood[
                static_cast<size_t> (bin)]
                = juce::jlimit (
                    0.0f,
                    1.0f,
                    tonality
                    * confidence
                    * (1.0f
                       - smoothStep (
                            (excessDb - 1.0f)
                            / 7.0f)));
        }
    }

    // Seven-bin triangular frequency regularisation.
    for (int bin = 3; bin < bins - 3; ++bin)
    {
        smoothedGains[
            static_cast<size_t> (bin)]
            =
              rawGains[
                  static_cast<size_t> (bin - 3)]
            + 2.0f * rawGains[
                  static_cast<size_t> (bin - 2)]
            + 3.0f * rawGains[
                  static_cast<size_t> (bin - 1)]
            + 4.0f * rawGains[
                  static_cast<size_t> (bin)]
            + 3.0f * rawGains[
                  static_cast<size_t> (bin + 1)]
            + 2.0f * rawGains[
                  static_cast<size_t> (bin + 2)]
            + rawGains[
                  static_cast<size_t> (bin + 3)];

        smoothedGains[
            static_cast<size_t> (bin)]
            *= 1.0f / 16.0f;
    }

    for (int bin = 1;
         bin < juce::jmin (3, bins - 1);
         ++bin)
    {
        smoothedGains[
            static_cast<size_t> (bin)]
            = rawGains[
                static_cast<size_t> (bin)];
    }

    for (int bin = juce::jmax (1, bins - 3);
         bin < bins - 1; ++bin)
    {
        smoothedGains[
            static_cast<size_t> (bin)]
            = rawGains[
                static_cast<size_t> (bin)];
    }

    float p3ProtectionWeight = 0.0f;
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
    float reductionDbSum = 0.0f;

    const float engageSeconds =
        requestedQuality.load()
            == Quality::clean2048
        ? 0.092f
        : 0.064f;

    const float engageAlpha =
        1.0f
        - std::exp (
            -static_cast<float> (hopSize)
            / static_cast<float> (
                sampleRate * engageSeconds));

    const float releaseAlpha =
        1.0f
        - std::exp (
            -static_cast<float> (hopSize)
            / static_cast<float> (
                sampleRate * 0.010));

    // Cross-frame target + asymmetric temporal smoothing.
    for (int bin = 1; bin < bins - 1; ++bin)
    {
        const auto index =
            static_cast<size_t> (bin);

        const float currentTarget =
            smoothedGains[index];

        const float previousTarget =
            previousFrequencyGain[index];

        const bool releasing =
            currentTarget > previousTarget;

        const float timeBlend =
            releasing ? 0.88f : 0.72f;

        const float target2d =
            timeBlend * currentTarget
            + (1.0f - timeBlend)
              * previousTarget;

        previousFrequencyGain[index] =
            currentTarget;

        auto& state =
            linkedGainState[index];

        const float alpha =
            target2d < state
            ? engageAlpha
            : releaseAlpha;

        state +=
            alpha * (target2d - state);

        rawGains[index] =
            juce::jlimit (
                dbToGain (
                    -reductionDb.load()
                    * 1.08f),
                1.0f,
                state);

        const float weight =
            detectorFrequencyWeight (bin);

        if (weight > 0.0f)
        {
            reductionWeight += weight;
            reductionDbSum +=
                weight
                * juce::jmax (
                    0.0f,
                    -gainToDb (
                        rawGains[index]));
        }
    }

    if (profileValid.load()
        && ! learningActive.load())
    {
        float weightSum = 0.0f;
        float excessSum = 0.0f;
        float activeWeight = 0.0f;

        float harmonicWeight = 0.0f;
        float harmonicSum = 0.0f;

        float tonalNoiseWeight = 0.0f;
        float tonalNoiseSum = 0.0f;

        float profileTotal = 0.0f;
        float residualTotal = 0.0f;

        for (int bin = 1;
             bin < bins - 1; ++bin)
        {
            const float detectorWeight =
                detectorFrequencyWeight (bin);

            if (detectorWeight <= 0.0f)
                continue;

            const float profile =
                0.5f
                * (profilePower[0][
                        static_cast<size_t> (bin)]
                   + profilePower[1][
                        static_cast<size_t> (bin)]);

            const float linkedPower =
                0.5f
                * (currentPower[0][
                        static_cast<size_t> (bin)]
                   + currentPower[
                        static_cast<size_t> (
                            rightChannel)]
                        [static_cast<size_t> (
                            bin)]);

            const float excessDb =
                juce::jlimit (
                    -12.0f,
                    24.0f,
                    powerRatioToDb (
                        (linkedPower + kFloor)
                        / (profile + kFloor)));

            weightSum += detectorWeight;
            excessSum +=
                detectorWeight * excessDb;

            if (excessDb > 6.0f)
                activeWeight += detectorWeight;

            if (excessDb > 2.5f)
            {
                harmonicWeight += detectorWeight;
                harmonicSum +=
                    detectorWeight
                    * tonalities[
                        static_cast<size_t> (
                            bin)];
            }

            tonalNoiseWeight += detectorWeight;
            tonalNoiseSum +=
                detectorWeight
                * tonalNoiseLikelihood[
                    static_cast<size_t> (bin)];

            const float gain =
                rawGains[
                    static_cast<size_t> (bin)];

            profileTotal +=
                detectorWeight * profile;

            residualTotal +=
                detectorWeight
                * profile
                * gain
                * gain;
        }

        const float weightedExcessDb =
            weightSum > 0.0f
            ? excessSum / weightSum
            : 0.0f;

        const float activeBandRatio =
            weightSum > 0.0f
            ? activeWeight / weightSum
            : 0.0f;

        const float harmonicProbability =
            harmonicWeight > 0.0f
            ? harmonicSum / harmonicWeight
            : 0.0f;

        const float tonalNoiseProbability =
            tonalNoiseWeight > 0.0f
            ? tonalNoiseSum / tonalNoiseWeight
            : 0.0f;

        const float excessScore =
            smoothStep (
                (weightedExcessDb - 2.0f)
                / 7.0f);

        const float occupancyScore =
            smoothStep (
                (activeBandRatio - 0.08f)
                / 0.20f);

        const float structureScore =
            juce::jmax (
                transientScore,
                smoothStep (
                    (harmonicProbability
                     - 0.25f)
                    / 0.55f));

        const float averageDetailProtection =
            p3ProtectionWeight > 0.0f ? p3DetailSum / p3ProtectionWeight : 0.0f;
        const float averageTailProtection =
            p3ProtectionWeight > 0.0f ? p3TailSum / p3ProtectionWeight : 0.0f;
        const float programPresence =
            juce::jlimit (
                0.0f,
                1.0f,
                juce::jmax (
                    0.52f * excessScore
                    + 0.35f * occupancyScore
                    + 0.13f * structureScore,
                    0.48f * averageTailProtection));

        float residualNoiseDb =
            estimatedNoiseFloorDb.load();

        if (profileTotal > kFloor)
        {
            residualNoiseDb +=
                powerRatioToDb (
                    (residualTotal + kFloor)
                    / (profileTotal + kFloor));
        }

        frameWeightedExcessDb.store (
            weightedExcessDb);
        frameActiveBandRatio.store (
            activeBandRatio);
        frameTransientProbability.store (
            transientScore);
        frameHarmonicProbability.store (
            juce::jlimit (
                0.0f,
                1.0f,
                harmonicProbability));
        frameTonalNoiseProbability.store (
            juce::jlimit (
                0.0f,
                1.0f,
                tonalNoiseProbability));
        frameProgramPresence.store (
            programPresence);
        frameResidualNoiseDb.store (
            juce::jlimit (
                -120.0f,
                0.0f,
                residualNoiseDb));
        frameSpectralReductionDb.store (
            reductionWeight > 0.0f
            ? reductionDbSum / reductionWeight
            : 0.0f);
        frameDetailProtection.store (
            p3ProtectionWeight > 0.0f ? p3DetailSum / p3ProtectionWeight : 0.0f);
        frameTailProtection.store (
            p3ProtectionWeight > 0.0f ? p3TailSum / p3ProtectionWeight : 0.0f);
    }
    else
    {
        clearFrameAnalysis();
    }

    // Shared stereo-linked gain map applied to both channels.
    for (int channel = 0;
         channel < channelCount; ++channel)
    {
        auto& work =
            fftWork[static_cast<size_t> (channel)];

        for (int bin = 0; bin < bins; ++bin)
        {
            const float gain =
                rawGains[
                    static_cast<size_t> (bin)];

            work[
                static_cast<size_t> (bin * 2)]
                *= gain;

            work[
                static_cast<size_t> (
                    bin * 2 + 1)]
                *= gain;
        }

        fixedFft.inverse (work.data());

        for (int i = 0; i < fftSize; ++i)
        {
            int destination =
                outputReadPos + 1 + i;

            if (destination >= outputFifoSize)
                destination -= outputFifoSize;

            const float sample =
                work[static_cast<size_t> (i)]
                * window[static_cast<size_t> (i)];

            outputRing[
                static_cast<size_t> (channel)]
                [static_cast<size_t> (
                    destination)]
                += sample;

            if (channelCount == 1)
            {
                outputRing[1][
                    static_cast<size_t> (
                        destination)]
                    += sample;
            }
        }
    }
}

void SmartDenoiseEngine::applySmartExpander (
    juce::AudioBuffer<float>& target) noexcept
{
    const int samples = target.getNumSamples();
    const int channels =
        juce::jmin (
            2, target.getNumChannels());

    if (channels <= 0 || samples <= 0)
        return;

    const float value =
        silenceAmount.load();

    const bool usable =
        enabled.load()
        && ! hearRemoved.load();

    const auto analysis =
        usable
        ? getFrameAnalysis()
        : NoiseFrameAnalysis {};

    if (value <= 0.0001f
        || ! usable
        || ! analysis.profileReady)
    {
        expanderOpenState = true;
        expanderHoldSamplesRemaining = 0;

        const float gainOpen =
            1.0f
            - std::exp (
                -1.0f
                / static_cast<float> (
                    sampleRate * 0.004));

        for (int sample = 0;
             sample < samples; ++sample)
        {
            expanderGainDbState +=
                gainOpen
                * (0.0f
                   - expanderGainDbState);

            const float gain =
                dbToGain (
                    expanderGainDbState);

            for (int channel = 0;
                 channel < channels; ++channel)
            {
                target.setSample (
                    channel,
                    sample,
                    target.getSample (
                        channel, sample)
                    * gain);
            }
        }

        expanderGainReductionDb.store (
            juce::jmax (
                0.0f,
                -expanderGainDbState));

        return;
    }

    const float rangeDb =
        juce::jmap (
            value, 0.0f, 30.0f);

    const float openExcessDb = 7.0f;
    const float closeExcessDb = 2.5f;

    const float openOccupancy =
        juce::jmap (
            value,
            0.22f,
            0.16f);

    const float closeOccupancy =
        juce::jmap (
            value,
            0.11f,
            0.075f);

    const int holdLength =
        static_cast<int> (
            sampleRate
            * juce::jmap (
                value,
                0.145f,
                0.095f));

    const bool strongStructuredSignal =
        analysis.harmonicProbability > 0.58f
        && analysis.weightedExcessDb > 2.5f;

    const bool openCandidate =
        analysis.weightedExcessDb
            > openExcessDb
        || analysis.activeBandRatio
            > openOccupancy
        || analysis.transientProbability
            > 0.62f
        || analysis.programPresence
            > 0.58f
        || strongStructuredSignal;

    const bool closeCandidate =
        analysis.weightedExcessDb
            < closeExcessDb
        && analysis.activeBandRatio
            < closeOccupancy
        && analysis.transientProbability
            < 0.24f
        && analysis.programPresence
            < 0.30f
        && ! strongStructuredSignal;

    if (expanderOpenState)
    {
        if (closeCandidate)
        {
            if (expanderHoldSamplesRemaining <= 0)
            {
                expanderHoldSamplesRemaining =
                    holdLength;
            }
            else
            {
                expanderHoldSamplesRemaining -=
                    samples;

                if (expanderHoldSamplesRemaining <= 0)
                    expanderOpenState = false;
            }
        }
        else
        {
            expanderHoldSamplesRemaining =
                holdLength;
        }
    }
    else if (openCandidate)
    {
        expanderOpenState = true;
        expanderHoldSamplesRemaining =
            holdLength;
    }

    float desiredGainDb = 0.0f;

    if (! expanderOpenState)
    {
        const float excessQuiet =
            1.0f
            - smoothStep (
                (analysis.weightedExcessDb
                 - closeExcessDb)
                / juce::jmax (
                    1.0f,
                    openExcessDb
                    - closeExcessDb));

        const float occupancyQuiet =
            1.0f
            - smoothStep (
                (analysis.activeBandRatio
                 - closeOccupancy)
                / juce::jmax (
                    0.01f,
                    openOccupancy
                    - closeOccupancy));

        const float quietConfidence =
            juce::jlimit (
                0.0f,
                1.0f,
                juce::jmin (
                    excessQuiet,
                    occupancyQuiet));

        const float detailProtection =
            juce::jlimit (
                0.0f,
                0.80f,
                0.70f
                    * analysis.programPresence
                + 0.20f
                    * analysis.harmonicProbability
                + 0.10f
                    * analysis.transientProbability);

        const float closure =
            quietConfidence
            * (1.0f - detailProtection);

        desiredGainDb =
            -rangeDb
            * juce::jlimit (
                0.0f,
                1.0f,
                closure);
    }

    const float gainOpen =
        1.0f
        - std::exp (
            -1.0f
            / static_cast<float> (
                sampleRate * 0.003));

    const float closeSeconds =
        juce::jmap (
            value,
            0.48f,
            0.34f);

    const float gainClose =
        1.0f
        - std::exp (
            -1.0f
            / static_cast<float> (
                sampleRate
                * closeSeconds));

    for (int sample = 0;
         sample < samples; ++sample)
    {
        const float gainAlpha =
            desiredGainDb
                > expanderGainDbState
            ? gainOpen
            : gainClose;

        expanderGainDbState +=
            gainAlpha
            * (desiredGainDb
               - expanderGainDbState);

        const float gain =
            dbToGain (
                expanderGainDbState);

        for (int channel = 0;
             channel < channels; ++channel)
        {
            target.setSample (
                channel,
                sample,
                target.getSample (
                    channel, sample)
                * gain);
        }
    }

    expanderGainReductionDb.store (
        juce::jmax (
            0.0f,
            -expanderGainDbState));
}

void SmartDenoiseEngine::process (
    juce::AudioBuffer<float>& buffer) noexcept
{
    // Quality/FFT reconfiguration is setup-level work and must never run
    // from the real-time audio callback. prepare() is the authority.
    if (learnRequested.exchange (false))
        beginLearningOnAudioThread();

    const int samples =
        buffer.getNumSamples();

    const int channels =
        juce::jmin (
            channelCount,
            buffer.getNumChannels());

    const bool denoiseEnabled =
        enabled.load();

    const bool removedOnly =
        hearRemoved.load();

    for (int sample = 0;
         sample < samples; ++sample)
    {
        float dry[maxChannels] {};

        int dryWritePosition =
            outputReadPos + fftSize;

        if (dryWritePosition >= outputFifoSize)
            dryWritePosition -= outputFifoSize;

        for (int channel = 0;
             channel < channels; ++channel)
        {
            dry[channel] =
                buffer.getSample (
                    channel, sample);

            dryDelayRing[
                static_cast<size_t> (channel)]
                [static_cast<size_t> (
                    dryWritePosition)]
                = dry[channel];

            const float spectralInput =
                subsonicFilters[
                    static_cast<size_t> (
                        channel)]
                    .process (dry[channel]);

            inputRing[
                static_cast<size_t> (channel)]
                [static_cast<size_t> (
                    inputWritePos)]
                = spectralInput;

            detailInputRing[
                static_cast<size_t> (channel)]
                [static_cast<size_t> (detailInputWritePos)]
                = spectralInput;
        }

        if (channels == 1)
        {
            inputRing[1][
                static_cast<size_t> (
                    inputWritePos)]
                = inputRing[0][
                    static_cast<size_t> (
                        inputWritePos)];

            detailInputRing[1][static_cast<size_t> (detailInputWritePos)] =
                detailInputRing[0][static_cast<size_t> (detailInputWritePos)];

            dryDelayRing[1][
                static_cast<size_t> (
                    dryWritePosition)]
                = dry[0];
        }

        if (++inputWritePos >= fftSize)
            inputWritePos = 0;
        if (++detailInputWritePos >= detailFftSize)
            detailInputWritePos = 0;

        ++samplesSeen;
        ++hopCounter;
        ++detailSamplesSeen;
        ++detailHopCounter;

        if (detailSamplesSeen >= detailFftSize
            && detailHopCounter >= detailHopSize)
        {
            detailHopCounter = 0;
            processDetailFrame();
        }

        if (samplesSeen >= fftSize
            && hopCounter >= hopSize)
        {
            hopCounter = 0;
            processFrame();
        }

        for (int channel = 0;
             channel < channels; ++channel)
        {
            float wet =
                outputRing[
                    static_cast<size_t> (channel)]
                    [static_cast<size_t> (
                        outputReadPos)];

            const float delayedDry =
                dryDelayRing[
                    static_cast<size_t> (channel)]
                    [static_cast<size_t> (
                        outputReadPos)];

            outputRing[
                static_cast<size_t> (channel)]
                [static_cast<size_t> (
                    outputReadPos)]
                = 0.0f;

            dryDelayRing[
                static_cast<size_t> (channel)]
                [static_cast<size_t> (
                    outputReadPos)]
                = 0.0f;

            if (! denoiseEnabled)
                wet = delayedDry;
            else if (removedOnly)
                wet = delayedDry - wet;

            buffer.setSample (
                channel,
                sample,
                std::isfinite (wet)
                    ? wet
                    : 0.0f);
        }

        if (++outputReadPos >= outputFifoSize)
            outputReadPos = 0;
    }

    applySmartExpander (buffer);
}

} // namespace smartdenoise
