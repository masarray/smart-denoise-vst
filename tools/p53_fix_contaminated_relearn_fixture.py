from pathlib import Path

root = Path('.')
engine_tests = root / 'tests/SmartDenoiseTests.cpp'
product_tests = root / 'tests/SmartDenoiseProductTests.cpp'

s = engine_tests.read_text(encoding='utf-8')
old = '''    std::uint32_t state = 0x12345678u;
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
'''
new = '''    engine.startLearning (3.0);
    processGenerated (engine,
                      static_cast<int> (sampleRate * 3.6),
                      2,
                      [] (std::int64_t n, int channel)
                      {
                          const auto quietLead = static_cast<std::int64_t> (sampleRate * 0.20);
                          const float noise = stationarySample (n);
                          if (n < quietLead)
                              return noise;

                          const double time = static_cast<double> (n) / sampleRate;
                          const float stereoPhase = channel == 0 ? 0.0f : 0.35f;
                          const float program =
                              0.13f * std::sin (static_cast<float> (
                                  juce::MathConstants<double>::twoPi * 230.0 * time + stereoPhase))
                            + 0.09f * std::sin (static_cast<float> (
                                  juce::MathConstants<double>::twoPi * 1840.0 * time + 0.5 * stereoPhase));
                          return noise + program;
                      });
'''
if old not in s:
    raise RuntimeError('engine rejected-relearn fixture anchor missing')
s = s.replace(old, new, 1)
engine_tests.write_text(s, encoding='utf-8', newline='\n')

s = product_tests.read_text(encoding='utf-8')
old = '''    std::uint32_t randomState = 0xBADC0FFEu;
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
'''
new = '''    bankRestored->startNoiseLearn();
    processGenerated (*bankRestored,
                      static_cast<int> (sampleRate * 3.6),
                      [] (std::int64_t n, int channel)
                      {
                          const auto quietLead = static_cast<std::int64_t> (sampleRate * 0.20);
                          const float noise = stationarySample (n);
                          if (n < quietLead)
                              return noise;

                          const double time = static_cast<double> (n) / sampleRate;
                          const float stereoPhase = channel == 0 ? 0.0f : 0.35f;
                          const float program =
                              0.13f * std::sin (static_cast<float> (
                                  juce::MathConstants<double>::twoPi * 230.0 * time + stereoPhase))
                            + 0.09f * std::sin (static_cast<float> (
                                  juce::MathConstants<double>::twoPi * 1840.0 * time + 0.5 * stereoPhase));
                          return noise + program;
                      });
'''
if old not in s:
    raise RuntimeError('product rejected-relearn fixture anchor missing')
s = s.replace(old, new, 1)
product_tests.write_text(s, encoding='utf-8', newline='\n')

assert 'quietLead' in engine_tests.read_text(encoding='utf-8')
assert 'quietLead' in product_tests.read_text(encoding='utf-8')
