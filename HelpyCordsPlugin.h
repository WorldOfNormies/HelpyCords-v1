#pragma once

#include <JuceHeader.h>
#include <array>
#include <vector>
#include <cmath>

//==============================================================================
// Instrument preset data
//==============================================================================
struct InstrumentPreset
{
    const char* name;
    float waveformMix[3];   // sine, triangle, sawtooth
    float attackTime;
    float decayTime;
    float sustainLevel;
    float releaseTime;
    float filterCutoffNorm; // 0-1
    float filterResonance;
    float harmonicRichness; // extra overtone blend 0-1
};

static constexpr int NUM_INSTRUMENTS = 13;

static const InstrumentPreset INSTRUMENT_PRESETS[NUM_INSTRUMENTS] = {
    // name                sine   tri    saw    att    dec   sus   rel    fCut  fRes  harm
    {"Grand Piano",       {0.4f, 0.6f, 0.0f}, 0.008f, 0.6f, 0.25f, 1.2f,  1.0f, 0.0f, 0.3f},
    {"Electric Piano",    {0.5f, 0.5f, 0.0f}, 0.005f, 0.4f, 0.35f, 0.9f,  0.9f, 0.2f, 0.15f},
    {"Acoustic Guitar",   {0.2f, 0.4f, 0.4f}, 0.015f, 0.9f, 0.08f, 0.6f,  0.85f,0.1f, 0.4f},
    {"Electric Guitar",   {0.1f, 0.3f, 0.6f}, 0.010f, 0.5f, 0.28f, 0.7f,  0.8f, 0.3f, 0.5f},
    {"Classical Guitar",  {0.25f,0.35f,0.4f}, 0.020f, 0.8f, 0.12f, 0.6f,  0.82f,0.15f,0.3f},
    {"Violin",            {0.6f, 0.3f, 0.1f}, 0.080f, 0.2f, 0.70f, 0.9f,  0.85f,0.2f, 0.2f},
    {"Cello",             {0.5f, 0.4f, 0.1f}, 0.100f, 0.3f, 0.60f, 0.88f, 0.80f,0.15f,0.25f},
    {"Trumpet",           {0.7f, 0.2f, 0.1f}, 0.050f, 0.4f, 0.50f, 0.92f, 0.75f,0.0f, 0.35f},
    {"Saxophone",         {0.65f,0.25f,0.1f}, 0.060f, 0.35f,0.60f, 0.90f, 0.80f,0.1f, 0.3f},
    {"Flute",             {0.90f,0.10f,0.0f}, 0.040f, 0.3f, 0.40f, 0.95f, 0.70f,0.0f, 0.05f},
    {"Bass",              {0.3f, 0.5f, 0.2f}, 0.020f, 0.6f, 0.50f, 0.75f, 0.90f,0.25f,0.2f},
    {"Harpsichord",       {0.0f, 1.0f, 0.0f}, 0.001f, 0.8f, 0.04f, 0.3f,  1.0f, 0.0f, 0.6f},
    {"Harp",              {0.8f, 0.2f, 0.0f}, 0.010f, 0.7f, 0.05f, 1.5f,  0.9f, 0.0f, 0.2f}
};

//==============================================================================
// Chord interval tables
//==============================================================================
static const int NUM_CHORD_TYPES = 7;
static const char* CHORD_TYPE_NAMES[NUM_CHORD_TYPES] = {
    "Major", "Minor", "Dominant 7", "Major 7", "Minor 7", "Sus2", "Sus4"
};
// Each chord has up to 4 intervals (root + 3 notes)
static const int CHORD_INTERVALS[NUM_CHORD_TYPES][4] = {
    {0, 4,  7,  12}, // Major
    {0, 3,  7,  12}, // Minor
    {0, 4,  7,  10}, // Dom7
    {0, 4,  7,  11}, // Maj7
    {0, 3,  7,  10}, // Min7
    {0, 2,  7,  12}, // Sus2
    {0, 5,  7,  12}, // Sus4
};

static const char* KEY_NAMES[12] = {
    "C","C#","D","D#","E","F","F#","G","G#","A","A#","B"
};

//==============================================================================
class HelpyCordsPlugin : public juce::AudioProcessor
{
public:
    HelpyCordsPlugin();
    ~HelpyCordsPlugin() override;

    //==========================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==========================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "HelpyCords"; }

    bool acceptsMidi()  const override { return true;  }
    bool producesMidi() const override { return true;  }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }

    int getNumPrograms()   override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==========================================================================
    // MIDI injection from the editor (thread-safe)
    juce::MidiMessageCollector& getMidiMessageCollector() { return midiCollector; }

    //==========================================================================
    // Parameters (owned by the AudioProcessor base class)
    juce::AudioParameterChoice* instrumentParam  = nullptr;
    juce::AudioParameterChoice* chordTypeParam   = nullptr;
    juce::AudioParameterChoice* keyParam         = nullptr;
    juce::AudioParameterBool*   autoCorrectParam = nullptr;
    juce::AudioParameterFloat*  sustainParam     = nullptr;
    juce::AudioParameterFloat*  decayParam       = nullptr;
    juce::AudioParameterFloat*  reverbParam      = nullptr;
    juce::AudioParameterFloat*  volumeParam      = nullptr;
    juce::AudioParameterFloat*  filterParam      = nullptr;
    juce::AudioParameterFloat*  pitchParam       = nullptr;
    juce::AudioParameterFloat*  speedParam       = nullptr;
    juce::AudioParameterFloat*  toneParam        = nullptr;
    juce::AudioParameterFloat*  delayParam       = nullptr;
    juce::AudioParameterFloat*  compressionParam = nullptr;

    //==========================================================================
    // Voice data (read from editor for key-highlight display)
    struct VoiceData
    {
        bool  isActive      = false;
        int   midiNote      = -1;   // original note pressed
        int   correctedNote = -1;   // snapped note (may differ when autocorrect on)
        float phase         = 0.0f;
        float frequency     = 0.0f;
        float velocity      = 0.0f;
        float envelopePhase = 0.0f;
        float filterState   = 0.0f;
        float filterState2  = 0.0f;
        bool  releasing     = false;
        float releasePhase  = 0.0f;
    };

    static constexpr int NUM_VOICES = 32;
    std::array<VoiceData, NUM_VOICES> voices;

    // Which notes are "in chord" (for green highlight) — updated each block
    bool noteInCurrentChord[128] = {};

    //==========================================================================
    // Helpers also used by editor
    int  getCorrectedNote (int inputNote) const;
    bool isNoteInChord    (int midiNote)  const;

private:
    juce::MidiMessageCollector midiCollector;

    double sampleRate_   = 44100.0;
    int    blockSize_    = 512;

    InstrumentPreset currentInstrument = INSTRUMENT_PRESETS[0];
    int currentKey                     = 0;
    int currentChordType               = 0;

    // Simple reverb state
    static constexpr int REVERB_SIZE = 16384;
    std::array<float, REVERB_SIZE> reverbBufferL{}, reverbBufferR{};
    int reverbWritePos = 0;

    // Simple delay state
    static constexpr int DELAY_SIZE = 44100;
    std::array<float, DELAY_SIZE> delayBufferL{}, delayBufferR{};
    int delayWritePos = 0;

    void  updateChordAndKey();
    float noteToFrequency (int midiNote) const;
    float generateWaveform (float phase, const InstrumentPreset& inst) const;
    float calculateEnvelope (const VoiceData& v, const InstrumentPreset& inst) const;
    float applyFilter (float input, float& state1, float& state2, float cutoffNorm) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HelpyCordsPlugin)
};
