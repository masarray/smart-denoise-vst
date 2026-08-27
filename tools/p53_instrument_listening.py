from pathlib import Path

p = Path('tests/SmartDenoiseListeningHarness.cpp')
s = p.read_text(encoding='utf-8')

anchor = '''struct FixtureResult
{
    std::vector<float> clean;
    std::vector<float> noisy;
    std::vector<float> processed;
};
'''
insert = anchor + '''
void checkpoint (const std::string& message)
{
    std::cerr << "[P5.3] " << message << std::endl;
}
'''
if 'void checkpoint (const std::string& message)' not in s:
    if anchor not in s:
        raise RuntimeError('FixtureResult anchor missing')
    s = s.replace(anchor, insert, 1)

old = '''{
    auto engine = std::make_unique<smartdenoise::SmartDenoiseEngine>();
    engine->prepare (sampleRate, blockSize, channels);
    engine->setSilenceAmount (0.0f);
    engine->setReductionDb (reductionDb);
    engine->setPreserve (preserve);
    learnProfile (*engine, profileNoise);

    if (! engine->hasProfile())
        throw std::runtime_error ("Listening fixture profile Learn was rejected");

    return { clean, noisy, processAligned (*engine, noisy) };
}
'''
new = '''{
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
        throw std::runtime_error ("Listening fixture profile Learn was rejected");

    checkpoint ("runFixture: process aligned");
    auto processed = processAligned (*engine, noisy);
    checkpoint ("runFixture: complete");
    return { clean, noisy, std::move (processed) };
}
'''
if old not in s:
    raise RuntimeError('runFixture anchor missing')
s = s.replace(old, new, 1)

fixtures = [
    ('const auto hiss =', 'fixture 01 stationary hiss'),
    ('const auto hum =', 'fixture 02 50 Hz hum'),
    ('const auto fan =', 'fixture 03 broadband fan'),
    ('const auto speechClean =', 'fixture 04 speech-like'),
    ('const auto transientClean =', 'fixture 05 transient-cymbal'),
    ('const auto pluckClean =', 'fixture 06 guitar-pluck'),
    ('const auto reverbClean =', 'fixture 07 reverb-tail'),
]
for token, label in fixtures:
    marker = f'    checkpoint ("{label}");\n    {token}'
    if marker in s:
        continue
    plain = '    ' + token
    if plain not in s:
        raise RuntimeError(f'fixture anchor missing: {token}')
    s = s.replace(plain, f'    checkpoint ("{label}");\n    {token}', 1)

save_anchor = '''void saveFixture (const std::filesystem::path& root,
                  const std::string& name,
                  const FixtureResult& result)
{
'''
if save_anchor in s and 'checkpoint ("saveFixture: " + name);' not in s:
    s = s.replace(save_anchor, save_anchor + '    checkpoint ("saveFixture: " + name);\n', 1)

p.write_text(s, encoding='utf-8', newline='\n')
