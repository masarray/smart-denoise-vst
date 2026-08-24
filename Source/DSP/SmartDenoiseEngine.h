#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

#include <array>
#include <atomic>
#include <cstdint>

namespace smartdenoise
{

struct NoiseFrameAnalysis
{
    bool profileReady = false;
    float weightedExcessDb = 0.0f;
    float activeBandRatio = 0.0f;
    float transientProbability = 0.0f;
    float harmonicProbability = 0.0f;
    float tonalNoiseProbability = 0.0f;
    float programPresence = 1.0f;
    float residualNoiseDb = -120.0f;
    float spectralReductionDb = 0.0f;
    float detailProtection = 0.0f;
    float tailProtection = 0.0f;
};

class SmartDenoiseEngine
{
public:
    static constexpr int maxChannels = 2;
    static constexpr int maxFftSize = 2048;
    static constexpr int maxBins = maxFftSize / 2 + 1;
    static constexpr int profileGroupCount = 7;
    static constexpr int detailFftSize = 512;
    static constexpr int detailBins = detailFftSize / 2 + 1;
    static constexpr int detailHopSize = detailFftSize / 2;
    static constexpr int profileDisplayBins = 48;
    static constexpr float maxReductionDb = 24.0f;

    enum class Quality
    {
        live1024 = 0,
        clean2048 = 1
    };

    SmartDenoiseEngine();

    void prepare (double sampleRate, int maximumBlockSize, int channels);
    void reset() noexcept;

    void setQuality (Quality quality) noexcept;
    Quality getQuality() const noexcept { return requestedQuality.load(); }

    void setEnabled (bool enabledIn) noexcept { enabled.store (enabledIn); }
    bool isEnabled() const noexcept { return enabled.load(); }

    void setReductionDb (float value) noexcept;
    float getReductionDb() const noexcept { return reductionDb.load(); }

    void setPreserve (float value01) noexcept;
    float getPreserve() const noexcept { return preserve.load(); }

    void setThresholdOffsetDb (float value) noexcept;
    void setSilenceAmount (float value01) noexcept;
    float getSilenceAmount() const noexcept { return silenceAmount.load(); }

    void setHearRemoved (bool enabledIn) noexcept { hearRemoved.store (enabledIn); }
    bool isHearRemoved() const noexcept { return hearRemoved.load(); }

    void startLearning (double seconds = 3.0) noexcept;
    bool isLearning() const noexcept { return learningActive.load(); }
    float getLearningProgress() const noexcept { return learningProgress.load(); }
    bool hasProfile() const noexcept { return profileValid.load(); }
    bool wasLastLearnRejected() const noexcept { return lastLearnRejected.load(); }
    float getProfileQuality() const noexcept { return profileQuality.load(); }
    float getEstimatedNoiseFloorDb() const noexcept { return estimatedNoiseFloorDb.load(); }
    int getRejectedLearningFrames() const noexcept { return learningFramesRejected.load(); }

    NoiseFrameAnalysis getFrameAnalysis() const noexcept;
    std::array<float, profileDisplayBins> getProfileDisplay() const noexcept;

    juce::String serialiseProfile() const;
    bool restoreProfile (const juce::String& encodedState);
    void clearProfile() noexcept;

    void process (juce::AudioBuffer<float>& buffer) noexcept;

    int getLatencySamples() const noexcept { return activeFftSize.load(); }
    float getExpanderGainReductionDb() const noexcept { return expanderGainReductionDb.load(); }

private:
    struct FixedFft
    {
        void configure (int newSize) noexcept;
        void forward (float* interleavedWork) const noexcept;
        void inverse (float* interleavedWork) const noexcept;

        int size = 1024;
        int stages = 10;
        std::array<int, maxFftSize> reversed {};
        std::array<float, maxFftSize / 2> cosine {};
        std::array<float, maxFftSize / 2> sine {};
    };

    struct SubsonicFilter
    {
        void prepare (double sampleRate, float cutoffHz) noexcept;
        void reset() noexcept;
        float process (float input) noexcept;

        float coefficient = 0.0f;
        float previousInput1 = 0.0f;
        float previousOutput1 = 0.0f;
        float previousInput2 = 0.0f;
        float previousOutput2 = 0.0f;
    };

    void reconfigureIfNeeded() noexcept;
    void beginLearningOnAudioThread() noexcept;
    void accumulateLearningFrame (float framePower, float transientScore) noexcept;
    void finaliseLearningOnAudioThread() noexcept;
    void processDetailFrame() noexcept;
    void processFrame() noexcept;
    void applySmartExpander (juce::AudioBuffer<float>& target) noexcept;

    void clearFrameAnalysis() noexcept;
    void resetSpectralState() noexcept;
    void invalidateProfile() noexcept;
    void publishProfileDisplay() noexcept;

    float frequencyWeight (int bin) const noexcept;
    float detectorFrequencyWeight (int bin) const noexcept;
    float detailProtectionForPrimaryBin (int bin) const noexcept;
    float tailProtectionForPrimaryBin (int bin) const noexcept;
    float calculateLinkedGain (int bin, float linkedPower,
                               float localTonality, float transientProtect) noexcept;

    FixedFft fixedFft;
    FixedFft detailFft;

    double sampleRate = 48000.0;
    int channelCount = 2;
    int fftSize = 1024;
    int hopSize = 512;
    int inputWritePos = 0;
    int outputReadPos = 0;
    int hopCounter = 0;
    std::int64_t samplesSeen = 0;
    int detailInputWritePos = 0;
    int detailHopCounter = 0;
    std::int64_t detailSamplesSeen = 0;

    static constexpr int outputFifoSize = maxFftSize * 4;

    std::array<std::array<float, maxFftSize>, maxChannels> inputRing {};
    std::array<std::array<float, outputFifoSize>, maxChannels> outputRing {};
    std::array<std::array<float, outputFifoSize>, maxChannels> dryDelayRing {};
    std::array<std::array<float, maxFftSize * 2>, maxChannels> fftWork {};
    std::array<std::array<float, maxBins>, maxChannels> currentPower {};
    std::array<std::array<float, maxBins>, maxChannels> previousMagnitude {};
    std::array<std::array<float, maxBins>, maxChannels> profilePower {};
    std::array<std::array<float, maxBins>, maxChannels> profileVarianceDb2 {};
    std::array<std::atomic<float>, profileDisplayBins> profileDisplay {};

    std::array<float, maxBins> linkedGainState {};
    std::array<float, maxBins> previousFrequencyGain {};
    std::array<float, maxBins> posteriorSnrState {};
    std::array<float, maxBins> tonalityState {};
    std::array<float, maxBins> linkedPreviousMagnitude {};
    std::array<float, maxBins> binTransientProbability {};
    std::array<float, maxBins> gainHistoryOne {};
    std::array<float, maxBins> gainHistoryTwo {};

    std::array<std::array<float, detailFftSize>, maxChannels> detailInputRing {};
    std::array<std::array<float, detailFftSize * 2>, maxChannels> detailWork {};
    std::array<float, detailBins> detailPreviousMagnitude {};
    std::array<float, detailBins> detailProtectionState {};
    std::array<float, detailBins> detailTailMemory {};
    std::array<float, detailFftSize> detailWindow {};

    std::array<float, maxFftSize> window {};
    std::array<SubsonicFilter, maxChannels> subsonicFilters {};

    std::array<std::array<std::array<double, maxBins>, profileGroupCount>, maxChannels>
        learningGroupPowerSum {};
    std::array<int, profileGroupCount> learningGroupAcceptedFrames {};
    std::array<double, profileGroupCount> learningGroupBroadbandPowerSum {};

    std::atomic<Quality> requestedQuality { Quality::live1024 };
    std::atomic<int> activeFftSize { 1024 };
    std::atomic<bool> qualityDirty { false };

    std::atomic<bool> enabled { true };
    std::atomic<float> reductionDb { 8.0f };
    std::atomic<float> preserve { 0.75f };
    std::atomic<float> thresholdOffsetDb { 1.5f };
    std::atomic<float> silenceAmount { 0.55f };
    std::atomic<bool> hearRemoved { false };

    std::atomic<bool> learnRequested { false };
    std::atomic<double> learnSecondsRequested { 3.0 };
    std::atomic<bool> learningActive { false };
    std::atomic<bool> profileValid { false };
    std::atomic<bool> lastLearnRejected { false };
    std::atomic<float> learningProgress { 0.0f };
    std::atomic<float> estimatedNoiseFloorDb { -90.0f };
    std::atomic<float> profileQuality { 0.0f };
    std::atomic<int> learningFramesRejected { 0 };

    std::atomic<float> frameWeightedExcessDb { 0.0f };
    std::atomic<float> frameActiveBandRatio { 0.0f };
    std::atomic<float> frameTransientProbability { 0.0f };
    std::atomic<float> frameHarmonicProbability { 0.0f };
    std::atomic<float> frameTonalNoiseProbability { 0.0f };
    std::atomic<float> frameProgramPresence { 1.0f };
    std::atomic<float> frameResidualNoiseDb { -120.0f };
    std::atomic<float> frameSpectralReductionDb { 0.0f };
    std::atomic<float> frameDetailProtection { 0.0f };
    std::atomic<float> frameTailProtection { 0.0f };

    int learningFramesTarget = 1;
    int learningFramesCaptured = 0;
    int learningFramesAccepted = 0;
    float learningBaselineDb = -120.0f;
    double learningDbMean = 0.0;
    double learningDbM2 = 0.0;
    bool profileValidBeforeLearning = false;

    double profileSampleRate = 0.0;
    int profileFftSize = 0;
    int profileChannels = 0;

    float expanderGainDbState = 0.0f;
    int expanderHoldSamplesRemaining = 0;
    bool expanderOpenState = true;
    std::atomic<float> expanderGainReductionDb { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SmartDenoiseEngine)
};

} // namespace smartdenoise
