#pragma once

#include <JuceHeader.h>
#include <array>
#include <vector>
#include <cmath>
#include <map>
#include <string>

enum class Instrument
{
    GrandPiano = 0,
    ElectricPiano,
    AcousticGuitar,
    ElectricGuitar,
    ClassicalGuitar,
    Violin,
    Cello,
    Trumpet,
    Saxophone,
    Flute,
    Bass,
    Harpsichord,
    COUNT
};

// std::string is NOT constexpr in C++17 - use const char* instead
struct InstrumentPreset
{
    const char* name;
    float waveformMix[3]; // sine, triangle, sawtooth
    float attackTime;
    float decayTime;
    float sustainLevel;
    float releaseTime;
    float filterCutoff;
    float filterResonance;
};

class HelpyCordsPlugin : public juce::AudioProcessor
{
public:
    HelpyCordsPlugin();
    ~HelpyCordsPlugin() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override;

    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // FIX: Expose MidiMessageCollector so the editor's piano keyboard can
    //      inject note-on / note-off messages into the audio thread safely.
    juce::MidiMessageCollector& getMidiMessageCollector() { return midiCollector; }

    // Plugin parameters
    juce::AudioParameterChoice* instrumentParam  = nullptr;
    juce::AudioParameterChoice* chordTypeParam   = nullptr;
    juce::AudioParameterChoice* keyParam         = nullptr;
    juce::AudioParameterBool*   autoCorrectParam = nullptr;
    juce::AudioParameterFloat*  sustainParam     = nullptr;
    juce::AudioParameterFloat*  decayParam       = nullptr;
    juce::AudioParameterFloat*  reverbParam      = nullptr;
    juce::AudioParameterFloat*  volumeParam      = nullptr;
    juce::AudioParameterFloat*  filterParam      = nullptr;
    juce::AudioParameterFloat*  keyboardZoomParam= nullptr;

    // Synthesis voice
    struct VoiceData
    {
        bool  isActive      = false;
        int   midiNote      = -1;
        int   correctedNote = -1;
        float envelope      = 0.0f;
        float phase         = 0.0f;
        float frequency     = 0.0f;
        float velocity      = 0.0f;
        float envelopePhase = 0.0f;
        float filterState   = 0.0f;
    };

    std::array<VoiceData, 32> voices;
    static constexpr int NUM_VOICES = 32;

    // Instrument presets
    static constexpr std::array<InstrumentPreset, static_cast<int>(Instrument::COUNT)> INSTRUMENT_PRESETS = {{
        // name                    sine  tri   saw    att    dec   sus   rel    fCut  fRes
        {"Grand Piano",           {0.3f, 0.7f, 0.0f}, 0.01f, 0.5f, 0.3f, 1.0f,  1.0f, 0.0f},
        {"Electric Piano",        {0.5f, 0.5f, 0.0f}, 0.005f,0.3f, 0.4f, 0.8f,  0.9f, 0.2f},
        {"Acoustic Guitar",       {0.2f, 0.4f, 0.4f}, 0.015f,0.8f, 0.1f, 0.5f,  0.85f,0.1f},
        {"Electric Guitar",       {0.1f, 0.3f, 0.6f}, 0.01f, 0.4f, 0.3f, 0.6f,  0.8f, 0.3f},
        {"Classical Guitar",      {0.25f,0.35f,0.4f}, 0.02f, 0.7f, 0.15f,0.55f, 0.82f,0.15f},
        {"Violin",                {0.6f, 0.3f, 0.1f}, 0.08f, 0.2f, 0.7f, 0.9f,  0.85f,0.2f},
        {"Cello",                 {0.5f, 0.4f, 0.1f}, 0.1f,  0.3f, 0.6f, 0.88f, 0.80f,0.15f},
        {"Trumpet",               {0.7f, 0.2f, 0.1f}, 0.05f, 0.4f, 0.5f, 0.92f, 0.75f,0.0f},
        {"Saxophone",             {0.65f,0.25f,0.1f}, 0.06f, 0.35f,0.6f, 0.90f, 0.80f,0.1f},
        {"Flute",                 {0.9f, 0.1f, 0.0f}, 0.04f, 0.3f, 0.4f, 0.95f, 0.70f,0.0f},
        {"Bass",                  {0.3f, 0.5f, 0.2f}, 0.02f, 0.6f, 0.5f, 0.75f, 0.90f,0.25f},
        {"Harpsichord",           {0.0f, 1.0f, 0.0f}, 0.001f,0.8f, 0.05f,0.3f,  1.0f, 0.0f}
    }};

    // Helper functions
    int   getCorrectedNote(int inputNote, int chordType, int key);
    float noteToFrequency(int midiNote);
    void  updateChordMapping();
    void  updateInstrumentPreset();
    float generateWaveform(float phase, float sineAmp, float triangleAmp, float sawtoothAmp);
    float calculateEnvelope(float phase, float attack, float decay, float sustain, float release);
    float simpleFilter(float input, float& state, float cutoff);

private:
    // FIX: MidiMessageCollector lets the editor thread safely send MIDI to
    //      processBlock() without data races. Must be reset() in prepareToPlay.
    juce::MidiMessageCollector midiCollector;

    double sampleRate    = 44100.0;
    int samplesPerBlock  = 512;

    // FIX: Use std::vector<int> instead of std::array<int,12> because
    //      chord types have different numbers of intervals (4 notes each but
    //      assigned via initializer_list which requires vector or explicit copy).
    std::vector<int> currentChordIntervals;

    int currentKey = 0;
    InstrumentPreset currentInstrument = INSTRUMENT_PRESETS[0];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HelpyCordsPlugin)
};
