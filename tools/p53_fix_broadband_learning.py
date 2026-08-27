from pathlib import Path

root = Path('.')
engine = root / 'Source/DSP/SmartDenoiseEngine.cpp'
tests = root / 'tests/SmartDenoiseTests.cpp'
listening = root / 'tests/SmartDenoiseListeningHarness.cpp'
qa = root / 'qa/verify_p53_validation.py'

s = engine.read_text(encoding='utf-8')
old = '''    const bool transientContamination =
        ! warmup
        && transientScore > 0.70f;
'''
new = '''    // Stationary stochastic noise naturally has high frame-to-frame spectral
    // flux. Treat flux as capture contamination only when it arrives with a
    // meaningful broadband level rise; otherwise real hiss/fan captures would
    // be rejected even though their long-term spectrum is stable.
    const bool transientContamination =
        ! warmup
        && transientScore > 0.82f
        && levelDeltaDb > 2.5f;
'''
if old not in s:
    raise RuntimeError('transient contamination anchor missing')
s = s.replace(old, new, 1)
engine.write_text(s, encoding='utf-8', newline='\n')

s = tests.read_text(encoding='utf-8')
anchor = '''float stationarySample (std::int64_t sampleIndex, float scale = 1.0f)
{
    constexpr double binHz = sampleRate / 1024.0;
    constexpr double f1 = binHz * 95.0;
    constexpr double f2 = binHz * 171.0;
    const double t = static_cast<double> (sampleIndex) / sampleRate;

    return scale * (0.012f * std::sin (static_cast<float> (juce::MathConstants<double>::twoPi * f1 * t))
                  + 0.005f * std::sin (static_cast<float> (juce::MathConstants<double>::twoPi * f2 * t)));
}
'''
addition = anchor + '''
float deterministicNoise (std::int64_t sampleIndex, std::uint32_t seed)
{
    std::uint32_t x = static_cast<std::uint32_t> (sampleIndex) + seed * 0x9e3779b9u;
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    x ^= x >> 16;
    return static_cast<float> (x) / static_cast<float> (0xffffffffu) * 2.0f - 1.0f;
}

float stationaryHissSample (std::int64_t sampleIndex)
{
    const float current = deterministicNoise (sampleIndex, 2u);
    const float previous = deterministicNoise (sampleIndex - 1, 2u);
    return (current - 0.78f * previous) * 0.0105f;
}
'''
if 'float stationaryHissSample' not in s:
    if anchor not in s:
        raise RuntimeError('stationary sample anchor missing')
    s = s.replace(anchor, addition, 1)

insert_before = '''void testRejectedRelearnKeepsProfile (TestContext& t)
'''
test_fn = '''void testBroadbandHissLearning (TestContext& t)
{
    auto engine = std::make_unique<smartdenoise::SmartDenoiseEngine>();
    engine->setQuality (smartdenoise::SmartDenoiseEngine::Quality::live1024);
    engine->prepare (sampleRate, blockSize, 2);
    engine->setSilenceAmount (0.0f);
    engine->startLearning (3.0);

    processGenerated (*engine,
                      static_cast<int> (sampleRate * 3.6),
                      2,
                      [] (std::int64_t n, int) { return stationaryHissSample (n); });

    t.expect (engine->hasProfile(),
              "Stationary stochastic hiss Learn creates a valid profile");
    t.expect (! engine->wasLastLearnRejected(),
              "Stationary stochastic hiss is not misclassified as transient contamination");
    t.expect (engine->getProfileQuality() >= 0.25f,
              "Stationary stochastic hiss profile passes quality gate");

    const auto fingerprint = engine->getProfileDisplay();
    t.expect (std::any_of (fingerprint.begin(), fingerprint.end(),
                          [] (float value) { return value > 0.05f; }),
              "Stationary stochastic hiss publishes a captured-profile fingerprint");
}

'''
if 'void testBroadbandHissLearning' not in s:
    if insert_before not in s:
        raise RuntimeError('test insertion anchor missing')
    s = s.replace(insert_before, test_fn + insert_before, 1)

call_anchor = '    testLearningAndPersistence (tests);\n'
if '    testBroadbandHissLearning (tests);\n' not in s:
    if call_anchor not in s:
        raise RuntimeError('test call anchor missing')
    s = s.replace(call_anchor, call_anchor + '    testBroadbandHissLearning (tests);\n', 1)
tests.write_text(s, encoding='utf-8', newline='\n')

s = listening.read_text(encoding='utf-8')
old = '''    if (! engine->hasProfile())
        throw std::runtime_error ("Listening fixture profile Learn was rejected");

    checkpoint ("runFixture: process aligned");
'''
new = '''    if (! engine->hasProfile())
    {
        std::cerr << "[P5.3] runFixture: Learn rejected; quality="
                  << engine->getProfileQuality()
                  << " rejectedFrames=" << engine->getRejectedLearningFrames()
                  << std::endl;
        throw std::runtime_error ("Listening fixture profile Learn was rejected");
    }

    checkpoint ("runFixture: Learn accepted");
    checkpoint ("runFixture: process aligned");
'''
if old not in s:
    raise RuntimeError('listening rejection anchor missing')
s = s.replace(old, new, 1)
listening.write_text(s, encoding='utf-8', newline='\n')

s = qa.read_text(encoding='utf-8')
if 'engine_cpp =' not in s:
    s = s.replace(
        'processor_cpp = (ROOT / "Source/Plugin/PluginProcessor.cpp").read_text(encoding="utf-8")\n',
        'processor_cpp = (ROOT / "Source/Plugin/PluginProcessor.cpp").read_text(encoding="utf-8")\nengine_cpp = (ROOT / "Source/DSP/SmartDenoiseEngine.cpp").read_text(encoding="utf-8")\nengine_tests = (ROOT / "tests/SmartDenoiseTests.cpp").read_text(encoding="utf-8")\n',
        1)
check_anchor = 'check("P5.3 listening hiss fixture", \'"01-stationary-hiss"\' in listening)\n'
extra = check_anchor + 'check("P5.3 broadband stochastic Learn regression", "testBroadbandHissLearning" in engine_tests and "stationaryHissSample" in engine_tests)\ncheck("P5.3 transient rejection requires level rise", "transientScore > 0.82f" in engine_cpp and "levelDeltaDb > 2.5f" in engine_cpp)\n'
if 'P5.3 broadband stochastic Learn regression' not in s:
    if check_anchor not in s:
        raise RuntimeError('QA insertion anchor missing')
    s = s.replace(check_anchor, extra, 1)
qa.write_text(s, encoding='utf-8', newline='\n')
