#include "HelpyCordsPlugin.h"
#include "HelpyCordsEditor.h"

//==============================================================================
HelpyCordsPlugin::HelpyCordsPlugin()
    : AudioProcessor (BusesProperties()
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    // Build StringArrays for choice parameters
    juce::StringArray instrumentNames, chordNames, keyNames;
    for (int i = 0; i < NUM_INSTRUMENTS;   ++i) instrumentNames.add (INSTRUMENT_PRESETS[i].name);
    for (int i = 0; i < NUM_CHORD_TYPES;   ++i) chordNames.add (CHORD_TYPE_NAMES[i]);
    for (int i = 0; i < 12;                ++i) keyNames.add (KEY_NAMES[i]);

    addParameter (instrumentParam  = new juce::AudioParameterChoice ("instrument",   "Instrument",  instrumentNames, 0));
    addParameter (chordTypeParam   = new juce::AudioParameterChoice ("chordType",    "Chord Type",  chordNames, 0));
    addParameter (keyParam         = new juce::AudioParameterChoice ("key",          "Key",         keyNames, 0));
    addParameter (autoCorrectParam = new juce::AudioParameterBool   ("autoCorrect",  "Auto Correct", true));
    addParameter (sustainParam     = new juce::AudioParameterFloat  ("sustain",      "Sustain",     0.0f,  2.0f,  1.0f));
    addParameter (decayParam       = new juce::AudioParameterFloat  ("decay",        "Decay",       0.01f, 2.0f,  0.5f));
    addParameter (reverbParam      = new juce::AudioParameterFloat  ("reverb",       "Reverb",      0.0f,  1.0f,  0.3f));
    addParameter (volumeParam      = new juce::AudioParameterFloat  ("volume",       "Volume",      -30.0f, 0.0f, -6.0f));
    addParameter (filterParam      = new juce::AudioParameterFloat  ("filter",       "Filter",      0.0f,  1.0f,  1.0f));

    reverbBufferL.fill (0.0f);
    reverbBufferR.fill (0.0f);
}

HelpyCordsPlugin::~HelpyCordsPlugin() {}

//==============================================================================
void HelpyCordsPlugin::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sampleRate_ = sampleRate;
    blockSize_  = samplesPerBlock;
    midiCollector.reset (sampleRate);
    reverbBufferL.fill (0.0f);
    reverbBufferR.fill (0.0f);
    reverbWritePos = 0;

    for (auto& v : voices)
        v = VoiceData{};
}

void HelpyCordsPlugin::releaseResources() {}

bool HelpyCordsPlugin::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

//==============================================================================
float HelpyCordsPlugin::noteToFrequency (int midiNote) const
{
    return 440.0f * std::pow (2.0f, (midiNote - 69) / 12.0f);
}

int HelpyCordsPlugin::getCorrectedNote (int inputNote) const
{
    if (! autoCorrectParam->get()) return inputNote;

    int noteInOct = inputNote % 12;
    int octave    = inputNote / 12;
    const int* intervals = CHORD_INTERVALS[currentChordType];

    int bestNote    = inputNote;
    int minDistance = 12;

    for (int i = 0; i < 4; ++i)
    {
        int chordNote = (currentKey + intervals[i]) % 12;
        int distance  = std::abs (noteInOct - chordNote);
        if (distance > 6) distance = 12 - distance;

        if (distance < minDistance)
        {
            minDistance = distance;
            bestNote    = octave * 12 + chordNote;
            // Keep corrected note in a reasonable octave range
            if (bestNote < 21)  bestNote += 12;
            if (bestNote > 108) bestNote -= 12;
        }
    }
    return bestNote;
}

bool HelpyCordsPlugin::isNoteInChord (int midiNote) const
{
    int noteInOct    = midiNote % 12;
    const int* intervals = CHORD_INTERVALS[currentChordType];
    for (int i = 0; i < 4; ++i)
        if ((currentKey + intervals[i]) % 12 == noteInOct)
            return true;
    return false;
}

void HelpyCordsPlugin::updateChordAndKey()
{
    currentChordType = chordTypeParam->getIndex();
    currentKey       = keyParam->getIndex();
    int instIdx      = instrumentParam->getIndex();
    if (instIdx >= 0 && instIdx < NUM_INSTRUMENTS)
        currentInstrument = INSTRUMENT_PRESETS[instIdx];

    // Update chord-note lookup table
    for (int i = 0; i < 128; ++i)
        noteInCurrentChord[i] = isNoteInChord (i);
}

//==============================================================================
float HelpyCordsPlugin::generateWaveform (float phase, const InstrumentPreset& inst) const
{
    const float twoPi = 6.28318530718f;
    float sine     = std::sin (phase * twoPi);
    float triangle = (phase < 0.5f) ? (4.0f * phase - 1.0f) : (3.0f - 4.0f * phase);
    float sawtooth = 2.0f * phase - 1.0f;

    float base = inst.waveformMix[0] * sine
               + inst.waveformMix[1] * triangle
               + inst.waveformMix[2] * sawtooth;

    // Harmonic richness: add 2nd + 3rd overtone
    if (inst.harmonicRichness > 0.0f)
    {
        float h2 = std::sin (phase * twoPi * 2.0f) * 0.5f;
        float h3 = std::sin (phase * twoPi * 3.0f) * 0.25f;
        base += inst.harmonicRichness * (h2 + h3) * 0.3f;
    }

    return base;
}

float HelpyCordsPlugin::calculateEnvelope (const VoiceData& v, const InstrumentPreset& inst) const
{
    float t       = v.envelopePhase;
    float attack  = inst.attackTime;
    float decay   = inst.decayTime;
    float sustain = inst.sustainLevel;

    if (t < attack)
        return t / (attack + 1e-6f);

    t -= attack;
    if (t < decay)
        return 1.0f - (t / (decay + 1e-6f)) * (1.0f - sustain);

    if (! v.releasing)
        return sustain;

    // Release
    float release = inst.releaseTime;
    return sustain * std::max (0.0f, 1.0f - v.releasePhase / (release + 1e-6f));
}

// Simple 2-pole low-pass (state-variable style)
float HelpyCordsPlugin::applyFilter (float input, float& state1, float& state2, float cutoffNorm) const
{
    float f = 0.02f + cutoffNorm * 0.45f;   // 0.02 – 0.47
    state1 += f * (input   - state1);
    state2 += f * (state1  - state2);
    return state2;
}

//==============================================================================
void HelpyCordsPlugin::processBlock (juce::AudioBuffer<float>& buffer,
                                     juce::MidiBuffer& midiMessages)
{
    const int numSamples    = buffer.getNumSamples();
    const int numOutCh      = getTotalNumOutputChannels();

    for (int ch = getTotalNumInputChannels(); ch < numOutCh; ++ch)
        buffer.clear (ch, 0, numSamples);

    // Collect MIDI from the editor's piano keyboard
    juce::MidiBuffer editorMidi;
    midiCollector.removeNextBlockOfMessages (editorMidi, numSamples);
    for (const auto meta : editorMidi)
        midiMessages.addEvent (meta.getMessage(), meta.samplePosition);

    updateChordAndKey();

    // Process MIDI events
    for (const auto meta : midiMessages)
    {
        const auto msg = meta.getMessage();

        if (msg.isNoteOn())
        {
            // Find a free voice (or steal the quietest)
            int   target   = -1;
            float minAmp   = 1e9f;

            for (int i = 0; i < NUM_VOICES; ++i)
            {
                if (! voices[i].isActive) { target = i; break; }
                float amp = calculateEnvelope (voices[i], currentInstrument);
                if (amp < minAmp) { minAmp = amp; target = i; }
            }

            if (target >= 0)
            {
                auto& v       = voices[target];
                v.isActive    = true;
                v.releasing   = false;
                v.releasePhase= 0.0f;
                v.midiNote    = msg.getNoteNumber();
                v.correctedNote = getCorrectedNote (v.midiNote);
                v.frequency   = noteToFrequency (v.correctedNote);
                v.velocity    = msg.getVelocity() / 127.0f;
                v.phase       = 0.0f;
                v.envelopePhase = 0.0f;
                v.filterState = 0.0f;
                v.filterState2= 0.0f;
            }
        }
        else if (msg.isNoteOff())
        {
            for (auto& v : voices)
                if (v.isActive && v.midiNote == msg.getNoteNumber() && ! v.releasing)
                {
                    v.releasing    = true;
                    v.releasePhase = 0.0f;
                }
        }
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
        {
            for (auto& v : voices) v.isActive = false;
        }
    }

    // Render
    float* outL = buffer.getWritePointer (0);
    float* outR = numOutCh > 1 ? buffer.getWritePointer (1) : outL;

    const float masterGain  = std::pow (10.0f, volumeParam->get() / 20.0f);
    const float filterCut   = filterParam->get();
    const float reverbWet   = reverbParam->get();
    const float dt          = 1.0f / static_cast<float> (sampleRate_);

    for (int s = 0; s < numSamples; ++s)
    {
        float mixL = 0.0f, mixR = 0.0f;

        for (auto& v : voices)
        {
            if (! v.isActive) continue;

            v.envelopePhase += dt;
            if (v.releasing) v.releasePhase += dt;

            float env = calculateEnvelope (v, currentInstrument);
            if (env < 1e-5f && v.releasing) { v.isActive = false; continue; }

            v.phase += v.frequency * dt;
            if (v.phase >= 1.0f) v.phase -= 1.0f;

            float wave     = generateWaveform (v.phase, currentInstrument);
            float filtered = applyFilter (wave, v.filterState, v.filterState2, filterCut);
            float out      = filtered * env * v.velocity * 0.08f;

            mixL += out;
            mixR += out;
        }

        // Simple reverb (Schroeder-ish comb with single tap)
        const int d1 = 4801, d2 = 3001;
        int r1L = (reverbWritePos + REVERB_SIZE - d1) & (REVERB_SIZE - 1);
        int r2R = (reverbWritePos + REVERB_SIZE - d2) & (REVERB_SIZE - 1);

        float revL = reverbBufferL[r1L] * 0.55f;
        float revR = reverbBufferR[r2R] * 0.55f;

        reverbBufferL[reverbWritePos] = mixL + revL * 0.4f;
        reverbBufferR[reverbWritePos] = mixR + revR * 0.4f;
        reverbWritePos = (reverbWritePos + 1) & (REVERB_SIZE - 1);

        outL[s] = (mixL + revL * reverbWet) * masterGain;
        outR[s] = (mixR + revR * reverbWet) * masterGain;
    }
}

//==============================================================================
juce::AudioProcessorEditor* HelpyCordsPlugin::createEditor()
{
    return new HelpyCordsEditor (*this);
}

void HelpyCordsPlugin::getStateInformation (juce::MemoryBlock& destData)
{
    juce::MemoryOutputStream stream (destData, true);
    stream.writeInt (instrumentParam->getIndex());
    stream.writeInt (chordTypeParam->getIndex());
    stream.writeInt (keyParam->getIndex());
    stream.writeBool (autoCorrectParam->get());
    stream.writeFloat (sustainParam->get());
    stream.writeFloat (decayParam->get());
    stream.writeFloat (reverbParam->get());
    stream.writeFloat (volumeParam->get());
    stream.writeFloat (filterParam->get());
}

void HelpyCordsPlugin::setStateInformation (const void* data, int sizeInBytes)
{
    juce::MemoryInputStream stream (data, static_cast<size_t>(sizeInBytes), false);
    if (stream.getDataSize() < 4) return;
    instrumentParam->setValueNotifyingHost  (instrumentParam->convertTo0to1 (stream.readInt()));
    chordTypeParam->setValueNotifyingHost   (chordTypeParam->convertTo0to1  (stream.readInt()));
    keyParam->setValueNotifyingHost         (keyParam->convertTo0to1        (stream.readInt()));
    autoCorrectParam->setValueNotifyingHost (stream.readBool() ? 1.0f : 0.0f);
    sustainParam->setValueNotifyingHost     (sustainParam->convertTo0to1    (stream.readFloat()));
    decayParam->setValueNotifyingHost       (decayParam->convertTo0to1      (stream.readFloat()));
    reverbParam->setValueNotifyingHost      (reverbParam->convertTo0to1     (stream.readFloat()));
    volumeParam->setValueNotifyingHost      (volumeParam->convertTo0to1     (stream.readFloat()));
    filterParam->setValueNotifyingHost      (filterParam->convertTo0to1     (stream.readFloat()));
}

//==============================================================================
// Plugin entry point
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HelpyCordsPlugin();
}
