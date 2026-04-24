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

HelpyCordsPlugin::~HelpyCordsPlugin() {}

void HelpyCordsPlugin::prepareToPlay(double sr, int block)
{
    sampleRate = sr;
    samplesPerBlock = block;
    midiCollector.reset(sr);
}

void HelpyCordsPlugin::releaseResources() {}

bool HelpyCordsPlugin::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono()
        || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
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
    int octave = inputNote / 12;

    int bestNote = inputNote;
    int minDistance = 12;

    for (int i = 0; i < 12; i++)
    {
        for (int interval : currentChordIntervals)
        {
            if ((key + interval) % 12 == i)
            {
                int distance = std::abs(noteInOctave - i);
                if (distance > 6) distance = 12 - distance;

                if (distance < minDistance)
                {
                    minDistance = distance;
                    bestNote = octave * 12 + i;
                }
            }
        }
    }

    return bestNote;
}

void HelpyCordsPlugin::updateChordMapping()
{
    int chordType = chordTypeParam->getIndex();
    currentKey = keyParam->getIndex();

    switch (chordType)
    {
        case 0: currentChordIntervals = {0,4,7,12}; break;
        case 1: currentChordIntervals = {0,3,7,12}; break;
        case 2: currentChordIntervals = {0,4,7,10}; break;
        case 3: currentChordIntervals = {0,4,7,11}; break;
        case 4: currentChordIntervals = {0,3,7,10}; break;
        case 5: currentChordIntervals = {0,2,7,12}; break;
        case 6: currentChordIntervals = {0,5,7,12}; break;
        default: currentChordIntervals = {0,4,7,12};
    }
}

void HelpyCordsPlugin::updateInstrumentPreset()
{
    int i = instrumentParam->getIndex();
    if (i >= 0 && i < (int)Instrument::COUNT)
        currentInstrument = INSTRUMENT_PRESETS[i];
}

void HelpyCordsPlugin::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    buffer.clear();

    juce::MidiBuffer editorMidi;
    midiCollector.removeNextBlockOfMessages(editorMidi, buffer.getNumSamples());
    midi.addEvents(editorMidi, 0, buffer.getNumSamples(), 0);

    updateChordMapping();
    updateInstrumentPreset();
}

juce::AudioProcessorEditor* HelpyCordsPlugin::createEditor()
{
    return new HelpyCordsEditor(*this);
}

const juce::String HelpyCordsPlugin::getName() const
{
    return "HelpyCords";
}

void HelpyCordsPlugin::getStateInformation(juce::MemoryBlock&) {}
void HelpyCordsPlugin::setStateInformation(const void*, int) {}


// ==========================================================
// 🔥 THIS IS THE FIX YOU WERE MISSING
// ==========================================================

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HelpyCordsPlugin();
}
