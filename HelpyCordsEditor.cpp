#include "HelpyCordsEditor.h"

//=============================================================================
// Colour palette
//=============================================================================
namespace Colors
{
    const juce::Colour bg         { 0xFF0F0F1A };
    const juce::Colour panel      { 0xFF16162A };
    const juce::Colour panelBorder{ 0xFF2A2A50 };
    const juce::Colour accent     { 0xFF4D88FF };
    const juce::Colour accent2    { 0xFF8855FF };
    const juce::Colour text       { 0xFFD0D8FF };
    const juce::Colour textDim    { 0xFF6670A0 };
    const juce::Colour keyWhite   { 0xFFEEF0FF };
    const juce::Colour keyBlack   { 0xFF181830 };
    const juce::Colour keyPressed { 0xFF4D88FF };
    const juce::Colour keyCorrect { 0xFF33CC88 };  // green — note is in chord
    const juce::Colour keyWrong   { 0xFFFF4455 };  // red   — pressed but wrong
}

//=============================================================================
// PianoKeyboardComponent
//=============================================================================
const int   PianoKeyboardComponent::WHITE_KEY_NOTES[7]  = { 0, 2, 4, 5, 7, 9, 11 };
const int   PianoKeyboardComponent::BLACK_KEY_NOTES[5]  = { 1, 3, 6, 8, 10 };
const float PianoKeyboardComponent::BLACK_KEY_OFFSETS[5]= { 0.65f, 1.65f, 3.65f, 4.65f, 5.65f };
const char* PianoKeyboardComponent::NOTE_NAMES[12]      = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

PianoKeyboardComponent::PianoKeyboardComponent (HelpyCordsPlugin& p)
    : processor (p)
{
    std::fill (std::begin (mouseHeld), std::end (mouseHeld), false);
    startTimerHz (30);
}

PianoKeyboardComponent::~PianoKeyboardComponent()
{
    stopTimer();
    // Release any held notes
    for (int i = 0; i < 128; ++i)
        if (mouseHeld[i]) sendNote (i, false);
}

bool PianoKeyboardComponent::isBlackKey (int noteInOct) const
{
    for (int b : BLACK_KEY_NOTES)
        if (b == noteInOct) return true;
    return false;
}

void PianoKeyboardComponent::setZoom (float z)
{
    zoom = juce::jlimit (0.5f, 2.5f, z);
    repaint();
}

void PianoKeyboardComponent::timerCallback()
{
    repaint();
}

//------------------------------------------------------------------------------
void PianoKeyboardComponent::paint (juce::Graphics& g)
{
    const int W = getWidth(), H = getHeight();
    g.fillAll (Colors::bg);

    // Always 24 keys (including black keys)
    // 24 chromatic keys is 2 octaves.
    const int totalKeys = 24;
    whiteKeyW = W / 14; // 14 white keys in 24 chromatic keys (C to B, C to B)
    blackKeyW = (int)(whiteKeyW * 0.6f);
    keyAreaX  = 0;

    const int whiteH = H - 2;
    const int blackH = static_cast<int> (whiteH * 0.62f);

    int startMidi = (octaveOffset + 1) * 12; // C1 is 24, C2 is 36...

    // ---- Pass 1: White keys ----
    int whiteKeyIndex = 0;
    for (int i = 0; i < totalKeys; ++i)
    {
        int midi = startMidi + i;
        int noteInOct = midi % 12;
        if (isBlackKey(noteInOct)) continue;

        int xPos = whiteKeyIndex * whiteKeyW;
        whiteKeyIndex++;

        juce::Colour col = Colors::keyWhite;
        bool inChord = processor.noteInCurrentChord[midi];

        // Basic instrument ranges
        auto range = processor.getInstrumentRange();
        bool outOfRange = (midi < range.first || midi > range.second);

        if (mouseHeld[midi])
        {
            col = inChord ? Colors::keyCorrect : Colors::keyWrong;
        }
        else if (inChord)
        {
            col = juce::Colour(0xFF444444); // Gray for chord in idle
        }

        if (outOfRange && !mouseHeld[midi])
            col = col.withAlpha(0.3f);

        g.setColour (col);
        g.fillRoundedRectangle ((float)xPos + 1, 1.0f,
                                 (float)(whiteKeyW - 2), (float)(whiteH - 2), 3.0f);

        g.setColour (Colors::panelBorder);
        g.drawRoundedRectangle ((float)xPos + 1, 1.0f,
                                 (float)(whiteKeyW - 2), (float)(whiteH - 2), 3.0f, 1.0f);

        // Note names
        g.setColour (mouseHeld[midi] ? Colors::bg : Colors::textDim);
        g.setFont (juce::Font (10.0f));
        g.drawText (NOTE_NAMES[noteInOct] + juce::String (midi / 12 - 1),
                     xPos + 1, whiteH - 16, whiteKeyW - 2, 14,
                     juce::Justification::centred);
    }

    // ---- Pass 2: Black keys ----
    whiteKeyIndex = 0;
    for (int i = 0; i < totalKeys; ++i)
    {
        int midi = startMidi + i;
        int noteInOct = midi % 12;

        if (isBlackKey(noteInOct))
        {
            // Calculate position based on previous white key
            int xPos = (whiteKeyIndex) * whiteKeyW - blackKeyW / 2;

            juce::Colour col = Colors::keyBlack;
            bool inChord = processor.noteInCurrentChord[midi];

            // Reuse same range logic
            auto range = processor.getInstrumentRange();
            bool outOfRange = (midi < range.first || midi > range.second);

            if (mouseHeld[midi])
            {
                col = inChord ? Colors::keyCorrect.darker (0.2f) : Colors::keyWrong.darker (0.2f);
            }
            else if (inChord)
            {
                col = juce::Colour(0xFF222222); // Darker gray for black keys in chord
            }

            if (outOfRange && !mouseHeld[midi])
                col = col.withAlpha(0.3f);

            g.setColour (col);
            g.fillRoundedRectangle ((float)xPos, 1.0f,
                                     (float)(blackKeyW - 1), (float)(blackH - 1), 2.0f);

            g.setColour (Colors::accent.withAlpha (0.3f));
            g.drawRoundedRectangle ((float)xPos, 1.0f,
                                     (float)(blackKeyW - 1), (float)(blackH - 1), 2.0f, 1.0f);

            // Note name
            g.setColour (Colors::textDim);
            g.setFont (juce::Font (8.0f));
            g.drawText (NOTE_NAMES[noteInOct],
                         xPos, blackH - 12, blackKeyW, 10,
                         juce::Justification::centred);
        }
        else
        {
            whiteKeyIndex++;
        }
    }

    // Legend strip at top
    g.setColour (Colors::panel.withAlpha (0.85f));
    g.fillRect (0, 0, W, 14);
    g.setFont (juce::Font (9.0f));
    g.setColour (Colors::keyCorrect);
    g.drawText ("■ In chord", 4, 1, 70, 12, juce::Justification::left);
    g.setColour (Colors::keyWrong);
    g.drawText ("■ Wrong note", 76, 1, 80, 12, juce::Justification::left);
    g.setColour (Colors::accent);
    g.drawText ("■ Pressed", 160, 1, 70, 12, juce::Justification::left);

    g.setColour (Colors::textDim);
    g.drawText ("Use Mouse Wheel to slide Octaves (C0-C8)", W - 180, 1, 175, 12, juce::Justification::right);
}

//------------------------------------------------------------------------------
int PianoKeyboardComponent::noteAtMouse (int x, int y) const
{
    const int H     = getHeight();
    const int blackH= static_cast<int> ((H - 2) * 0.62f);
    const int totalKeys = 24;
    int startMidi = (octaveOffset + 1) * 12;

    // Check black keys first (drawn on top)
    int whiteKeyIndex = 0;
    for (int i = 0; i < totalKeys; ++i)
    {
        int midi = startMidi + i;
        int noteInOct = midi % 12;
        if (isBlackKey(noteInOct))
        {
            int bx = (whiteKeyIndex) * whiteKeyW - blackKeyW / 2;
            if (x >= bx && x < bx + blackKeyW && y < blackH)
                return midi;
        }
        else
        {
            whiteKeyIndex++;
        }
    }

    // White keys
    whiteKeyIndex = 0;
    for (int i = 0; i < totalKeys; ++i)
    {
        int midi = startMidi + i;
        int noteInOct = midi % 12;
        if (isBlackKey(noteInOct)) continue;

        int xPos = whiteKeyIndex * whiteKeyW;
        if (x >= xPos && x < xPos + whiteKeyW)
            return midi;
        whiteKeyIndex++;
    }
    return -1;
}

void PianoKeyboardComponent::sendNote (int midiNote, bool on)
{
    if (midiNote < 0 || midiNote > 127) return;
    juce::MidiMessage msg = on
        ? juce::MidiMessage::noteOn  (1, midiNote, (juce::uint8)100)
        : juce::MidiMessage::noteOff (1, midiNote);
    processor.getMidiMessageCollector().handleIncomingMidiMessage (nullptr, msg);
}

void PianoKeyboardComponent::mouseDown (const juce::MouseEvent& e)
{
    int note = noteAtMouse (e.x, e.y);
    if (note >= 0 && ! mouseHeld[note])
    {
        mouseHeld[note] = true;
        lastDragNote    = note;
        sendNote (note, true);
        repaint();
    }
}

void PianoKeyboardComponent::mouseUp (const juce::MouseEvent& e)
{
    juce::ignoreUnused (e);
    for (int i = 0; i < 128; ++i)
    {
        if (mouseHeld[i])
        {
            mouseHeld[i] = false;
            sendNote (i, false);
        }
    }
    lastDragNote = -1;
    repaint();
}

void PianoKeyboardComponent::mouseDrag (const juce::MouseEvent& e)
{
    int note = noteAtMouse (e.x, e.y);
    if (note == lastDragNote) return;

    if (lastDragNote >= 0 && mouseHeld[lastDragNote])
    {
        mouseHeld[lastDragNote] = false;
        sendNote (lastDragNote, false);
    }

    if (note >= 0)
    {
        mouseHeld[note] = true;
        sendNote (note, true);
    }
    lastDragNote = note;
    repaint();
}

void PianoKeyboardComponent::mouseWheelMove (const juce::MouseEvent& /*e*/,
                                              const juce::MouseWheelDetails& w)
{
    if (w.deltaY > 0) octaveOffset = juce::jlimit(0, 7, octaveOffset + 1);
    else if (w.deltaY < 0) octaveOffset = juce::jlimit(0, 7, octaveOffset - 1);
    repaint();
}

//=============================================================================
// HelpyCordsEditor
//=============================================================================
HelpyCordsEditor::HelpyCordsEditor (HelpyCordsPlugin& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setSize (1100, 700);
    setResizable (true, true);
    setResizeLimits (800, 550, 1800, 1000);

    //--- Title ----------------------------------------------------------------
    titleLabel.setText ("HelpyCords", juce::dontSendNotification);
    titleLabel.setFont (juce::Font ("Helvetica Neue", 30.0f, juce::Font::bold));
    titleLabel.setColour (juce::Label::textColourId, Colors::accent);
    addAndMakeVisible (titleLabel);

    authorLabel.setText ("v2.0  ·  by Agustin", juce::dontSendNotification);
    authorLabel.setFont (juce::Font (12.0f));
    authorLabel.setColour (juce::Label::textColourId, Colors::textDim);
    addAndMakeVisible (authorLabel);

    //--- Instrument -----------------------------------------------------------
    instrumentLabel.setText ("INSTRUMENT", juce::dontSendNotification);
    instrumentLabel.setFont (juce::Font (10.0f, juce::Font::bold));
    instrumentLabel.setColour (juce::Label::textColourId, Colors::textDim);
    addAndMakeVisible (instrumentLabel);

    for (int i = 0; i < NUM_INSTRUMENTS; ++i)
        instrumentBox.addItem (INSTRUMENT_PRESETS[i].name, i + 1);
    instrumentBox.setSelectedItemIndex (0);
    instrumentBox.addListener (this);
    addAndMakeVisible (instrumentBox);

    //--- Chord ----------------------------------------------------------------
    chordLabel.setText ("CHORD TYPE", juce::dontSendNotification);
    chordLabel.setFont (juce::Font (10.0f, juce::Font::bold));
    chordLabel.setColour (juce::Label::textColourId, Colors::textDim);
    addAndMakeVisible (chordLabel);

    for (int i = 0; i < NUM_CHORD_TYPES; ++i)
        chordBox.addItem (CHORD_TYPE_NAMES[i], i + 1);
    chordBox.setSelectedItemIndex (0);
    chordBox.addListener (this);
    addAndMakeVisible (chordBox);

    //--- Key ------------------------------------------------------------------
    keyLabel.setText ("KEY", juce::dontSendNotification);
    keyLabel.setFont (juce::Font (10.0f, juce::Font::bold));
    keyLabel.setColour (juce::Label::textColourId, Colors::textDim);
    addAndMakeVisible (keyLabel);

    for (int i = 0; i < 12; ++i)
        keyBox.addItem (KEY_NAMES[i], i + 1);
    keyBox.setSelectedItemIndex (0);
    keyBox.addListener (this);
    addAndMakeVisible (keyBox);

    //--- AutoCorrect toggle ---------------------------------------------------
    autoCorrectButton.setButtonText ("AUTO-CORRECT");
    autoCorrectButton.setToggleState (true, juce::dontSendNotification);
    autoCorrectButton.addListener (this);
    addAndMakeVisible (autoCorrectButton);

    autoCorrectLabel.setText ("Snap pressed notes to chord tones", juce::dontSendNotification);
    autoCorrectLabel.setFont (juce::Font (10.0f));
    autoCorrectLabel.setColour (juce::Label::textColourId, Colors::textDim);
    addAndMakeVisible (autoCorrectLabel);

    //--- Sliders --------------------------------------------------------------
    setupSlider (sustainRow, "SUSTAIN",  0.0,   2.0,  0.01, 1.0);
    setupSlider (decayRow,   "DECAY",    0.01,  2.0,  0.01, 0.5);
    setupSlider (reverbRow,  "REVERB",   0.0,   1.0,  0.01, 0.3);
    setupSlider (filterRow,  "FILTER",   0.0,   1.0,  0.01, 1.0);
    setupSlider (volumeRow,  "VOLUME",  -30.0,  0.0,  0.5, -6.0);

    setupSlider (pitchRow,       "PITCH",      -12.0, 12.0, 1.0,  0.0);
    setupSlider (speedRow,       "SPEED",       0.1,  3.0,  0.1,  1.0);
    setupSlider (toneRow,        "TONE",        0.0,  1.0,  0.01, 0.5);
    setupSlider (delayRow,       "DELAY",       0.0,  1.0,  0.01, 0.0);
    setupSlider (compressionRow, "COMPRESSION", 0.0,  1.0,  0.01, 0.0);

    //--- Chord info -----------------------------------------------------------
    chordInfoLabel.setText ("", juce::dontSendNotification);
    chordInfoLabel.setFont (juce::Font (12.0f, juce::Font::bold));
    chordInfoLabel.setColour (juce::Label::textColourId, Colors::accent2);
    chordInfoLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (chordInfoLabel);

    //--- Piano ----------------------------------------------------------------
    piano = std::make_unique<PianoKeyboardComponent> (p);
    addAndMakeVisible (piano.get());

    updateChordInfoLabel();
    startTimerHz (10);
}

HelpyCordsEditor::~HelpyCordsEditor()
{
    stopTimer();
}

void HelpyCordsEditor::setupSlider (SliderRow& row, const juce::String& name,
                                     double lo, double hi, double step, double val)
{
    auto& s = row.slider;
    auto& l = row.label;

    s.setSliderStyle (juce::Slider::LinearHorizontal);
    s.setTextBoxStyle (juce::Slider::TextBoxRight, false, 50, 16);
    s.setRange (lo, hi, step);
    s.setValue (val, juce::dontSendNotification);
    s.setColour (juce::Slider::thumbColourId,        Colors::accent);
    s.setColour (juce::Slider::trackColourId,        Colors::accent.withAlpha (0.4f));
    s.setColour (juce::Slider::backgroundColourId,   Colors::panelBorder);
    s.setColour (juce::Slider::textBoxTextColourId,  Colors::textDim);
    s.setColour (juce::Slider::textBoxOutlineColourId, juce::Colour (0));
    s.addListener (this);
    addAndMakeVisible (s);

    l.setText (name, juce::dontSendNotification);
    l.setFont (juce::Font (10.0f, juce::Font::bold));
    l.setColour (juce::Label::textColourId, Colors::textDim);
    addAndMakeVisible (l);
}

//------------------------------------------------------------------------------
void HelpyCordsEditor::paint (juce::Graphics& g)
{
    const int W = getWidth(), H = getHeight();

    // Background
    g.fillAll (Colors::bg);

    // Header gradient band
    juce::ColourGradient headerGrad (Colors::accent.withAlpha (0.12f), 0, 0,
                                     Colors::accent2.withAlpha (0.06f), (float)W, 70,
                                     false);
    g.setGradientFill (headerGrad);
    g.fillRect (0, 0, W, 70);

    g.setColour (Colors::panelBorder);
    g.drawHorizontalLine (70, 0, (float)W);

    // Controls panel
    g.setColour (Colors::panel);
    g.fillRoundedRectangle (8, 78, (float)(W - 16), 200, 6);
    g.setColour (Colors::panelBorder);
    g.drawRoundedRectangle (8, 78, (float)(W - 16), 200, 6, 1.0f);

    // Piano panel
    int pianoY = H - 200;
    g.setColour (Colors::panel);
    g.fillRoundedRectangle (8, (float)(pianoY - 8), (float)(W - 16), 208, 6);
    g.setColour (Colors::panelBorder);
    g.drawRoundedRectangle (8, (float)(pianoY - 8), (float)(W - 16), 208, 6, 1.0f);

    // Sliders label bar
    g.setColour (Colors::panelBorder.withAlpha (0.5f));
    g.fillRect (8, 195, W - 16, 1);
    g.fillRect (8, 240, W - 16, 1);
}

void HelpyCordsEditor::resized()
{
    const int W = getWidth(), H = getHeight();
    const int m = 16;  // margin

    // Header
    titleLabel.setBounds  (m, 10, 250, 36);
    authorLabel.setBounds (m, 46, 250, 18);

    // Controls row 1: instrument, chord, key, autocorrect
    int rowY = 88;
    int colW = 160, lblH = 14, boxH = 28;
    int col  = m;

    instrumentLabel.setBounds (col, rowY, colW, lblH);
    instrumentBox.setBounds   (col, rowY + lblH + 2, colW, boxH);
    col += colW + m;

    chordLabel.setBounds (col, rowY, colW, lblH);
    chordBox.setBounds   (col, rowY + lblH + 2, colW, boxH);
    col += colW + m;

    keyLabel.setBounds (col, rowY, 80, lblH);
    keyBox.setBounds   (col, rowY + lblH + 2, 80, boxH);
    col += 96;

    autoCorrectButton.setBounds (col, rowY + lblH + 2, 140, boxH);
    autoCorrectLabel.setBounds  (col, rowY, 250, lblH);
    col += 155;

    chordInfoLabel.setBounds (col, rowY, W - col - m, boxH + lblH);

    // Sliders row
    int sliderY  = rowY + boxH + lblH + 18;
    int sliderW  = (W - 2 * m - 4 * m) / 5;
    int sliderLH = 14;

    auto placeSlider = [&](SliderRow& row, int x, int y)
    {
        row.label.setBounds  (x, y, sliderW, sliderLH);
        row.slider.setBounds (x, y + sliderLH, sliderW, 26);
    };

    int sx = m;
    placeSlider (sustainRow, sx, sliderY); sx += sliderW + m;
    placeSlider (decayRow,   sx, sliderY); sx += sliderW + m;
    placeSlider (reverbRow,  sx, sliderY); sx += sliderW + m;
    placeSlider (filterRow,  sx, sliderY); sx += sliderW + m;
    placeSlider (volumeRow,  sx, sliderY);

    sx = m;
    int sliderY2 = sliderY + 45;
    placeSlider (pitchRow,       sx, sliderY2); sx += sliderW + m;
    placeSlider (speedRow,       sx, sliderY2); sx += sliderW + m;
    placeSlider (toneRow,        sx, sliderY2); sx += sliderW + m;
    placeSlider (delayRow,       sx, sliderY2); sx += sliderW + m;
    placeSlider (compressionRow, sx, sliderY2);

    // Piano keyboard
    int pianoY = H - 200;
    piano->setBounds (m, pianoY, W - 2 * m, 192);
}

//------------------------------------------------------------------------------
void HelpyCordsEditor::updateChordInfoLabel()
{
    int keyIdx   = processor.keyParam->getIndex();
    int chordIdx = processor.chordTypeParam->getIndex();
    juce::String txt = KEY_NAMES[keyIdx];
    txt += " " + juce::String (CHORD_TYPE_NAMES[chordIdx]) + "  —  ";

    const int* ivs = CHORD_INTERVALS[chordIdx];
    for (int i = 0; i < 4; ++i)
    {
        int note = (keyIdx + ivs[i]) % 12;
        txt += KEY_NAMES[note];
        if (i < 3) txt += " · ";
    }
    chordInfoLabel.setText (txt, juce::dontSendNotification);
}

//------------------------------------------------------------------------------
void HelpyCordsEditor::timerCallback()
{
    // Keep combo boxes in sync if host changes parameters
    int instIdx  = processor.instrumentParam->getIndex();
    int chordIdx = processor.chordTypeParam->getIndex();
    int keyIdx   = processor.keyParam->getIndex();

    if (instrumentBox.getSelectedItemIndex() != instIdx)
        instrumentBox.setSelectedItemIndex (instIdx, juce::dontSendNotification);
    if (chordBox.getSelectedItemIndex() != chordIdx)
        chordBox.setSelectedItemIndex (chordIdx, juce::dontSendNotification);
    if (keyBox.getSelectedItemIndex() != keyIdx)
        keyBox.setSelectedItemIndex (keyIdx, juce::dontSendNotification);
}

//------------------------------------------------------------------------------
void HelpyCordsEditor::sliderValueChanged (juce::Slider* slider)
{
    if      (slider == &sustainRow.slider) *processor.sustainParam = (float) slider->getValue();
    else if (slider == &decayRow.slider)   *processor.decayParam   = (float) slider->getValue();
    else if (slider == &reverbRow.slider)  *processor.reverbParam  = (float) slider->getValue();
    else if (slider == &filterRow.slider)  *processor.filterParam  = (float) slider->getValue();
    else if (slider == &volumeRow.slider)  *processor.volumeParam  = (float) slider->getValue();
    else if (slider == &pitchRow.slider)   *processor.pitchParam   = (float) slider->getValue();
    else if (slider == &speedRow.slider)   *processor.speedParam   = (float) slider->getValue();
    else if (slider == &toneRow.slider)    *processor.toneParam    = (float) slider->getValue();
    else if (slider == &delayRow.slider)   *processor.delayParam   = (float) slider->getValue();
    else if (slider == &compressionRow.slider) *processor.compressionParam = (float) slider->getValue();
}

void HelpyCordsEditor::comboBoxChanged (juce::ComboBox* comboBox)
{
    if (comboBox == &instrumentBox)
        processor.instrumentParam->setValueNotifyingHost (
            processor.instrumentParam->convertTo0to1 (instrumentBox.getSelectedItemIndex()));
    else if (comboBox == &chordBox)
        processor.chordTypeParam->setValueNotifyingHost (
            processor.chordTypeParam->convertTo0to1 (chordBox.getSelectedItemIndex()));
    else if (comboBox == &keyBox)
        processor.keyParam->setValueNotifyingHost (
            processor.keyParam->convertTo0to1 (keyBox.getSelectedItemIndex()));

    updateChordInfoLabel();
}

void HelpyCordsEditor::buttonClicked (juce::Button* button)
{
    if (button == &autoCorrectButton)
        *processor.autoCorrectParam = autoCorrectButton.getToggleState() ? 1.0f : 0.0f;
}
