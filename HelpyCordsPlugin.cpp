#include "HelpyCordsPlugin.h"
#include "HelpyCordsEditor.h"

HelpyCordsPlugin::HelpyCordsPlugin()
    : AudioProcessor(BusesProperties()
        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    addParameter(instrumentParam = new juce::AudioParameterChoice(
        "instrument", "Instrument",
        juce::StringArray("Grand Piano", "Electric Piano", "Acoustic Guitar", "Electric Guitar",
                         "Classical Guitar", "Violin", "Cello", "Trumpet", "Saxophone", "Flute", "Bass", "Harpsichord"), 0));

    addParameter(chordTypeParam = new juce::AudioParameterChoice(
        "chordType", "Chord Type",
        juce::StringArray("Major", "Minor", "Dominant 7", "Major 7", "Minor 7", "Sus2", "Sus4"), 0));

    addParameter(keyParam = new juce::AudioParameterChoice(
        "key", "Key",
        juce::StringArray("C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"), 0));

    addParameter(autoCorrectParam = new juce::AudioParameterBool(
        "autoCorrect", "Auto Correct", true));

    addParameter(sustainParam = new juce::AudioParameterFloat(
        "sustain", "Sustain", 0.0f, 2.0f, 1.0f));

    addParameter(decayParam = new juce::AudioParameterFloat(
        "decay", "Decay", 0.01f, 2.0f, 0.5f));

    addParameter(reverbParam = new juce::AudioParameterFloat(
        "reverb", "Reverb", 0.0f, 1.0f, 0.3f));

    addParameter(volumeParam = new juce::AudioParameterFloat(
        "volume", "Volume", -30.0f, 0.0f, -6.0f));

    addParameter(filterParam = new juce::AudioParameterFloat(
        "filter", "Filter Cutoff", 0.0f, 1.0f, 1.0f));

    addParameter(keyboardZoomParam = new juce::AudioParameterFloat(
        "keyboardZoom", "Keyboard Zoom", 0.5f, 2.0f, 1.0f));

    updateChordMapping();
    updateInstrumentPreset();
}

HelpyCordsPlugin::~HelpyCordsPlugin()
{
}

void HelpyCordsPlugin::prepareToPlay(double sr, int block)
{
    sampleRate      = sr;
    samplesPerBlock = block;

    // FIX: MidiMessageCollector must be reset with the sample rate before use.
    midiCollector.reset(sr);
}

void HelpyCordsPlugin::releaseResources()
{
}

bool HelpyCordsPlugin::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return true;
}

float HelpyCordsPlugin::noteToFrequency(int midiNote)
{
    return 440.0f * std::pow(2.0f, (midiNote - 69) / 12.0f);
}

int HelpyCordsPlugin::getCorrectedNote(int inputNote, int chordType, int key)
{
    juce::ignoreUnused(chordType);

    if (!autoCorrectParam->get())
        return inputNote;

    int noteInOctave = inputNote % 12;
    int octave       = inputNote / 12;

    int bestNote    = inputNote;
    int minDistance = 12;

    for (int i = 0; i < 12; i++)
    {
        bool isInChord = false;
        for (int interval : currentChordIntervals)
        {
            if ((key + interval) % 12 == i)
            {
                isInChord = true;
                break;
            }
        }

        if (isInChord)
        {
            int distance = std::abs(noteInOctave - i);
            if (distance > 6) distance = 12 - distance;

            if (distance < minDistance)
            {
                minDistance = distance;
                bestNote    = octave * 12 + i;
            }
        }
    }

    return bestNote;
}

void HelpyCordsPlugin::updateChordMapping()
{
    int chordType = chordTypeParam->getIndex();
    currentKey    = keyParam->getIndex();

    switch (chordType)
    {
        case 0: currentChordIntervals = {0, 4, 7, 12}; break;
        case 1: currentChordIntervals = {0, 3, 7, 12}; break;
        case 2: currentChordIntervals = {0, 4, 7, 10}; break;
        case 3: currentChordIntervals = {0, 4, 7, 11}; break;
        case 4: currentChordIntervals = {0, 3, 7, 10}; break;
        case 5: currentChordIntervals = {0, 2, 7, 12}; break;
        case 6: currentChordIntervals = {0, 5, 7, 12}; break;
        default: currentChordIntervals = {0, 4, 7, 12};
    }
}

void HelpyCordsPlugin::updateInstrumentPreset()
{
    int instIndex = instrumentParam->getIndex();
    if (instIndex >= 0 && instIndex < static_cast<int>(Instrument::COUNT))
        currentInstrument = INSTRUMENT_PRESETS[instIndex];
}

float HelpyCordsPlugin::generateWaveform(float phase, float sineAmp, float triangleAmp, float sawtoothAmp)
{
    float sine     = std::sin(phase * 6.28318f);
    float triangle = (phase < 0.5f) ? (4.0f * phase - 1.0f) : (4.0f * (1.0f - phase) - 1.0f);
    float sawtooth = 2.0f * phase - 1.0f;
    return sineAmp * sine + triangleAmp * triangle + sawtoothAmp * sawtooth;
}

float HelpyCordsPlugin::calculateEnvelope(float phase, float attack, float decay, float sustain, float release)
{
    float totalTime       = attack + decay + sustain + release;
    float normalizedPhase = std::fmod(phase, totalTime);

    if (normalizedPhase < attack)
        return normalizedPhase / (attack + 0.001f);
    else if (normalizedPhase < attack + decay)
        return 1.0f - ((normalizedPhase - attack) / decay) * (1.0f - sustain);
    else if (normalizedPhase < attack + decay + sustain)
        return sustain;
    else
        return sustain * (1.0f - ((normalizedPhase - attack - decay - sustain) / (release + 0.001f)));
}

float HelpyCordsPlugin::simpleFilter(float input, float& state, float cutoff)
{
    state = state + cutoff * (input - state);
    return state;
}

void HelpyCordsPlugin::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    for (auto i = getTotalNumInputChannels(); i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Pull any MIDI messages injected by the editor's piano keyboard
    juce::MidiBuffer editorMidi;
    midiCollector.removeNextBlockOfMessages(editorMidi, buffer.getNumSamples());
    for (const auto meta : editorMidi)
        midiMessages.addEvent(meta.getMessage(), meta.samplePosition);

    updateChordMapping();
    updateInstrumentPreset();

    for (const auto meta : midiMessages)
    {
        auto msg = meta.getMessage();

        if (msg.isNoteOn())
        {
            for (auto& voice : voices)
            {
                if (!voice.isActive)
                {
                    voice.isActive      = true;
                    voice.midiNote      = msg.getNoteNumber();
                    voice.correctedNote = getCorrectedNote(msg.getNoteNumber(), chordTypeParam->getIndex(), currentKey);
                    voice.frequency     = noteToFrequency(voice.correctedNote);
                    voice.velocity      = msg.getVelocity() / 127.0f;
                    voice.envelopePhase = 0.0f;
                    voice.filterState   = 0.0f;
                    break;
                }
            }
        }
        else if (msg.isNoteOff())
        {
            for (auto& voice : voices)
            {
                if (voice.midiNote == msg.getNoteNumber())
                    voice.isActive = false;
            }
        }
    }

    auto* channelL = buffer.getWritePointer(0);
    auto* channelR = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : channelL;

    float masterVolume = std::pow(10.0f, volumeParam->get() / 20.0f);
    float filterCutoff = 0.01f + filterParam->get() * 0.15f;

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        float outL = 0.0f, outR = 0.0f;

        for (auto& voice : voices)
        {
            if (voice.isActive)
            {
                voice.envelopePhase += (1.0f / static_cast<float>(sampleRate));

                float envelope = calculateEnvelope(
                    voice.envelopePhase,
                    currentInstrument.attackTime,
                    currentInstrument.decayTime,
                    currentInstrument.sustainLevel,
                    currentInstrument.releaseTime);

                voice.phase += voice.frequency / static_cast<float>(sampleRate);
                if (voice.phase >= 1.0f)
                    voice.phase -= 1.0f;

                float waveform = generateWaveform(
                    voice.phase,
                    currentInstrument.waveformMix[0],
                    currentInstrument.waveformMix[1],
                    currentInstrument.waveformMix[2]);

                float filtered    = simpleFilter(waveform, voice.filterState, filterCutoff);
                float sampleValue = filtered * envelope * voice.velocity * 0.1f;

                outL += sampleValue;
                outR += sampleValue * (1.0f - reverbParam->get() * 0.3f);
            }
        }

        channelL[sample] = outL * masterVolume;
        if (buffer.getNumChannels() > 1)
            channelR[sample] = outR * masterVolume;
    }
}

juce::AudioProcessorEditor* HelpyCordsPlugin::createEditor()
{
    return new HelpyCordsEditor(*this);
}

const juce::String HelpyCordsPlugin::getName() const
{
    return "HelpyCords";
}

void HelpyCordsPlugin::getStateInformation(juce::MemoryBlock& /*destData*/)
{
}

void HelpyCordsPlugin::setStateInformation(const void* /*data*/, int /*sizeInBytes*/)
{
}
