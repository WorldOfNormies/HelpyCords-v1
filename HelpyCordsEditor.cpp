#include "HelpyCordsEditor.h"

HelpyCordsEditor::HelpyCordsEditor(HelpyCordsPlugin& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(1200, 750);

    titleLabel.setText("HelpyCords", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(32.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xFF0066FF));
    addAndMakeVisible(titleLabel);

    authorLabel.setText("-by Agustin", juce::dontSendNotification);
    authorLabel.setFont(juce::Font(10.0f));
    authorLabel.setColour(juce::Label::textColourId, juce::Colour(0xFF0066FF).withAlpha(0.7f));
    addAndMakeVisible(authorLabel);

    instrumentBox.addItemList(juce::StringArray(
        "Grand Piano", "Electric Piano", "Acoustic Guitar", "Electric Guitar",
        "Classical Guitar", "Violin", "Cello", "Trumpet", "Saxophone", "Flute", "Bass", "Harpsichord"), 1);
    instrumentBox.setSelectedItemIndex(0);
    instrumentBox.addListener(this);
    addAndMakeVisible(instrumentBox);

    instrumentLabel.setText("Instrument:", juce::dontSendNotification);
    instrumentLabel.setFont(juce::Font(14.0f, juce::Font::bold));
    instrumentLabel.setColour(juce::Label::textColourId, juce::Colour(0xFF333333));
    addAndMakeVisible(instrumentLabel);

    chordTypeBox.addItemList(juce::StringArray("Major", "Minor", "Dominant 7", "Major 7", "Minor 7", "Sus2", "Sus4"), 1);
    chordTypeBox.setSelectedItemIndex(0);
    chordTypeBox.addListener(this);
    addAndMakeVisible(chordTypeBox);

    chordTypeLabel.setText("Chord:", juce::dontSendNotification);
    chordTypeLabel.setFont(juce::Font(12.0f, juce::Font::bold));
    chordTypeLabel.setColour(juce::Label::textColourId, juce::Colour(0xFF333333));
    addAndMakeVisible(chordTypeLabel);

    keyBox.addItemList(juce::StringArray("C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"), 1);
    keyBox.setSelectedItemIndex(0);
    keyBox.addListener(this);
    addAndMakeVisible(keyBox);

    keyLabel.setText("Key:", juce::dontSendNotification);
    keyLabel.setFont(juce::Font(12.0f, juce::Font::bold));
    keyLabel.setColour(juce::Label::textColourId, juce::Colour(0xFF333333));
    addAndMakeVisible(keyLabel);

    autoCorrectButton.setButtonText("Auto Correct");
    autoCorrectButton.setToggleState(true, juce::dontSendNotification);
    autoCorrectButton.setColour(juce::ToggleButton::tickColourId, juce::Colour(0xFF0066FF));
    autoCorrectButton.addListener(this);
    addAndMakeVisible(autoCorrectButton);

    auto setupSlider = [&](juce::Slider& s, juce::Label& l, const juce::String& name,
                           double lo, double hi, double step, double val)
    {
        s.setSliderStyle(juce::Slider::LinearHorizontal);
        s.setRange(lo, hi, step);
        s.setValue(val);
        s.setColour(juce::Slider::thumbColourId, juce::Colour(0xFF0066FF));
        s.setColour(juce::Slider::trackColourId, juce::Colour(0xFFE6B8FF).withAlpha(0.5f));
        s.addListener(this);
        addAndMakeVisible(s);
        l.setText(name, juce::dontSendNotification);
        l.setFont(juce::Font(11.0f, juce::Font::bold));
        l.setColour(juce::Label::textColourId, juce::Colour(0xFF333333));
        addAndMakeVisible(l);
    };

    setupSlider(sustainSlider, sustainLabel, "Sustain", 0.0,  2.0,  0.05, 1.0);
    setupSlider(decaySlider,   decayLabel,   "Decay",   0.01, 2.0,  0.05, 0.5);
    setupSlider(reverbSlider,  reverbLabel,  "Reverb",  0.0,  1.0,  0.05, 0.3);
    setupSlider(filterSlider,  filterLabel,  "Filter",  0.0,  1.0,  0.05, 1.0);
    setupSlider(volumeSlider,  volumeLabel,  "Volume",  -30.0, 0.0, 1.0, -6.0);
    setupSlider(zoomSlider,    zoomLabel,    "Zoom",    0.5,  2.0,  0.1,  1.0);

    keyboard = std::make_unique<PianoKeyboard>(processor);
    addAndMakeVisible(keyboard.get());
}

HelpyCordsEditor::~HelpyCordsEditor()
{
}

void HelpyCordsEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xFFFFF5F8));

    g.setColour(juce::Colour(0xFFFFE8F0).withAlpha(0.8f));
    g.fillRect(0, 0, getWidth(), 80);

    g.setColour(juce::Colour(0xFF0066FF).withAlpha(0.2f));
    g.drawLine(0, 80, (float)getWidth(), 80, 1.0f);

    g.setColour(juce::Colour(0xFFFFFFFF).withAlpha(0.9f));
    g.fillRoundedRectangle(10, 100, (float)(getWidth() - 20), 180, 8);
    g.setColour(juce::Colour(0xFF0066FF).withAlpha(0.15f));
    g.drawRoundedRectangle(10, 100, (float)(getWidth() - 20), 180, 8, 1.0f);
}

void HelpyCordsEditor::resized()
{
    auto bounds = getLocalBounds();

    titleLabel.setBounds(20, 15, 300, 40);
    authorLabel.setBounds(320, 45, 150, 20);

    int controlY = 110;
    int margin   = 15;

    instrumentLabel.setBounds(margin, controlY, 80, 20);
    instrumentBox.setBounds(margin, controlY + 20, 150, 30);

    chordTypeLabel.setBounds(margin + 170, controlY, 80, 20);
    chordTypeBox.setBounds(margin + 170, controlY + 20, 130, 30);

    keyLabel.setBounds(margin + 320, controlY, 50, 20);
    keyBox.setBounds(margin + 320, controlY + 20, 90, 30);

    autoCorrectButton.setBounds(margin + 430, controlY + 20, 150, 30);

    int sliderY   = controlY + 70;
    int sliderW   = 140;
    int sliderGap = 10;

    sustainLabel.setBounds(margin, sliderY, 80, 15);
    sustainSlider.setBounds(margin, sliderY + 15, sliderW, 20);

    decayLabel.setBounds(margin + (sliderW + sliderGap) + 40, sliderY, 80, 15);
    decaySlider.setBounds(margin + (sliderW + sliderGap) + 40, sliderY + 15, sliderW, 20);

    reverbLabel.setBounds(margin + (sliderW + sliderGap) * 2 + 40, sliderY, 80, 15);
    reverbSlider.setBounds(margin + (sliderW + sliderGap) * 2 + 40, sliderY + 15, sliderW, 20);

    filterLabel.setBounds(margin + (sliderW + sliderGap) * 3 + 40, sliderY, 80, 15);
    filterSlider.setBounds(margin + (sliderW + sliderGap) * 3 + 40, sliderY + 15, sliderW, 20);

    volumeLabel.setBounds(margin + (sliderW + sliderGap) * 4 + 40, sliderY, 80, 15);
    volumeSlider.setBounds(margin + (sliderW + sliderGap) * 4 + 40, sliderY + 15, sliderW, 20);

    zoomLabel.setBounds(margin + (sliderW + sliderGap) * 5 + 40, sliderY, 80, 15);
    zoomSlider.setBounds(margin + (sliderW + sliderGap) * 5 + 40, sliderY + 15, sliderW, 20);

    auto keyboardBounds = bounds.removeFromBottom(bounds.getHeight() - 300);
    keyboard->setBounds(keyboardBounds);
}

void HelpyCordsEditor::sliderValueChanged(juce::Slider* slider)
{
    if (slider == &sustainSlider)
        *processor.sustainParam = (float)sustainSlider.getValue();
    else if (slider == &decaySlider)
        *processor.decayParam = (float)decaySlider.getValue();
    else if (slider == &reverbSlider)
        *processor.reverbParam = (float)reverbSlider.getValue();
    else if (slider == &filterSlider)
        *processor.filterParam = (float)filterSlider.getValue();
    else if (slider == &volumeSlider)
        *processor.volumeParam = (float)volumeSlider.getValue();
    else if (slider == &zoomSlider)
        keyboard->setZoom((float)zoomSlider.getValue());
}

void HelpyCordsEditor::comboBoxChanged(juce::ComboBox* comboBox)
{
    if (comboBox == &instrumentBox)
        processor.instrumentParam->setValueNotifyingHost(
            (float)instrumentBox.getSelectedItemIndex() / 11.0f);
    else if (comboBox == &chordTypeBox)
        processor.chordTypeParam->setValueNotifyingHost(
            (float)chordTypeBox.getSelectedItemIndex() / 6.0f);
    else if (comboBox == &keyBox)
        processor.keyParam->setValueNotifyingHost(
            (float)keyBox.getSelectedItemIndex() / 11.0f);
}

void HelpyCordsEditor::buttonClicked(juce::Button* button)
{
    if (button == &autoCorrectButton)
        *processor.autoCorrectParam = autoCorrectButton.getToggleState();
}

// ===========================================================================
// PianoKeyboard
// ===========================================================================

HelpyCordsEditor::PianoKeyboard::PianoKeyboard(HelpyCordsPlugin& p)
    : processor(p)
{
    keyPressed.fill(false);
    setWantsKeyboardFocus(true);
}

void HelpyCordsEditor::PianoKeyboard::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xFF1A1A2E).withAlpha(0.95f));

    const int startX        = 20;
    const int whiteKeyWidth = static_cast<int>(50 * zoom);
    const int blackKeyWidth = static_cast<int>(30 * zoom);
    const int keyHeight     = getHeight() - 40;
    const int startNote     = 36; // C2

    // How many white keys can we fit?
    const int totalWhiteKeys = (getWidth() - startX) / whiteKeyWidth;

    // --- Pass 1: White keys ---
    // white key index -> midi note mapping
    // White keys in an octave: C=0, D=2, E=4, F=5, G=7, A=9, B=11
    static const int whiteKeyNotes[] = { 0, 2, 4, 5, 7, 9, 11 };

    for (int wk = 0; wk < totalWhiteKeys; ++wk)
    {
        int octave      = wk / 7;
        int noteInOct   = whiteKeyNotes[wk % 7];
        int midiNote    = startNote + octave * 12 + noteInOct;
        if (midiNote >= 128) break;

        int xPos = startX + wk * whiteKeyWidth;

        juce::Colour keyColour = keyPressed[midiNote]
                                 ? juce::Colour(0xFF00FFFF)
                                 : juce::Colour(0xFFFFFFFF);
        g.setColour(keyColour);
        g.fillRoundedRectangle((float)xPos, 15.0f, (float)(whiteKeyWidth - 2), (float)keyHeight, 4.0f);

        g.setColour(juce::Colour(0xFF0066FF).withAlpha(0.3f));
        g.drawRoundedRectangle((float)xPos, 15.0f, (float)(whiteKeyWidth - 2), (float)keyHeight, 4.0f, 1.5f);

        static const char* noteNames[] = {"C","","D","","E","F","","G","","A","","B"};
        g.setColour(juce::Colour(0xFF333333));
        g.setFont(juce::Font(10.0f, juce::Font::bold));
        g.drawText(noteNames[noteInOct], xPos, getHeight() - 25, whiteKeyWidth - 2, 20,
                   juce::Justification::centred, true);
    }

    // --- Pass 2: Black keys (drawn on top) ---
    // Black keys in octave: C#=1, D#=3, F#=6, G#=8, A#=10
    // Their position offset from the start of the octave in white-key units:
    // C# is after C (white key 0), offset = 0.7
    // D# is after D (white key 1), offset = 1.7
    // F# is after F (white key 3), offset = 3.7
    // G# is after G (white key 4), offset = 4.7
    // A# is after A (white key 5), offset = 5.7
    static const float blackKeyOffsets[] = { 0.7f, 1.7f, 3.7f, 4.7f, 5.7f };
    static const int   blackKeyNotes[]   = { 1,    3,    6,    8,    10   };

    for (int wk = 0; wk < totalWhiteKeys; wk += 7)
    {
        int octave = wk / 7;
        for (int b = 0; b < 5; ++b)
        {
            int midiNote = startNote + octave * 12 + blackKeyNotes[b];
            if (midiNote >= 128) break;

            int bx = startX + static_cast<int>((wk + blackKeyOffsets[b]) * whiteKeyWidth) - blackKeyWidth / 2;
            if (bx + blackKeyWidth > getWidth()) break;

            juce::Colour keyColour = keyPressed[midiNote]
                                     ? juce::Colour(0xFF00FFFF)
                                     : juce::Colour(0xFF0066FF);
            g.setColour(keyColour);
            g.fillRoundedRectangle((float)bx, 15.0f, (float)(blackKeyWidth - 2), (float)(keyHeight - 40), 3.0f);

            g.setColour(juce::Colour(0xFF000000).withAlpha(0.3f));
            g.drawRoundedRectangle((float)bx, 15.0f, (float)(blackKeyWidth - 2), (float)(keyHeight - 40), 3.0f, 1.0f);
        }
    }
}

void HelpyCordsEditor::PianoKeyboard::mouseDown(const juce::MouseEvent& e)
{
    const int startX        = 20;
    const int whiteKeyWidth = static_cast<int>(50 * zoom);
    const int adjustedX     = e.x - startX;

    if (adjustedX < 0) return;

    // FIX: Map white-key index back to MIDI note correctly
    static const int whiteKeyNotes[] = { 0, 2, 4, 5, 7, 9, 11 };
    int wkIndex  = adjustedX / whiteKeyWidth;
    int octave   = wkIndex / 7;
    int noteInOct = whiteKeyNotes[wkIndex % 7];
    int midiNote  = 36 + octave * 12 + noteInOct;

    if (midiNote >= 0 && midiNote < 128)
    {
        keyPressed[midiNote] = true;
        sendMidiNote(midiNote, true);
        repaint();
    }
}

void HelpyCordsEditor::PianoKeyboard::mouseUp(const juce::MouseEvent& e)
{
    const int startX        = 20;
    const int whiteKeyWidth = static_cast<int>(50 * zoom);
    const int adjustedX     = e.x - startX;

    if (adjustedX < 0) return;

    static const int whiteKeyNotes[] = { 0, 2, 4, 5, 7, 9, 11 };
    int wkIndex   = adjustedX / whiteKeyWidth;
    int octave    = wkIndex / 7;
    int noteInOct = whiteKeyNotes[wkIndex % 7];
    int midiNote  = 36 + octave * 12 + noteInOct;

    if (midiNote >= 0 && midiNote < 128)
    {
        keyPressed[midiNote] = false;
        sendMidiNote(midiNote, false);
        repaint();
    }
}

void HelpyCordsEditor::PianoKeyboard::mouseWheelMove(const juce::MouseEvent& /*e*/,
                                                      const juce::MouseWheelDetails& wheel)
{
    scrollOffset -= static_cast<int>(wheel.deltaY * 10);
    repaint();
}

void HelpyCordsEditor::PianoKeyboard::sendMidiNote(int midiNote, bool noteOn)
{
    juce::MidiMessage msg = noteOn
        ? juce::MidiMessage::noteOn(1, midiNote, static_cast<juce::uint8>(100))
        : juce::MidiMessage::noteOff(1, midiNote);

    processor.getMidiMessageCollector().handleIncomingMidiMessage(nullptr, msg);
}
