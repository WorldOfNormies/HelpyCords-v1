#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_events/juce_events.h>
#include "HelpyCordsPlugin.h"

//==============================================================================
// PianoKeyboard component
//==============================================================================
class PianoKeyboardComponent : public juce::Component,
                               public juce::Timer
{
public:
    explicit PianoKeyboardComponent (HelpyCordsPlugin& p);
    ~PianoKeyboardComponent() override;

    void paint (juce::Graphics& g) override;
    void mouseDown  (const juce::MouseEvent& e) override;
    void mouseUp    (const juce::MouseEvent& e) override;
    void mouseDrag  (const juce::MouseEvent& e) override;
    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override;
    void timerCallback() override;

    void setZoom (float z);
    float getZoom() const { return zoom; }

private:
    HelpyCordsPlugin& processor;

    float zoom         = 1.0f;
    int   scrollOffset = 0;        // pixel scroll (horizontal)
    int   startNote    = 36;       // C2
    int   octaveOffset = 2;        // Default to C2

    // Which notes are currently held by mouse
    bool mouseHeld[128] = {};
    int  lastDragNote   = -1;

    // Cached layout (recalculated in paint)
    int whiteKeyW = 44;
    int blackKeyW = 26;
    int keyAreaX  = 0;

    int noteAtMouse (int x, int y) const;
    void sendNote (int midiNote, bool on);
    bool isBlackKey (int noteInOct) const;

    static const int WHITE_KEY_NOTES[7];
    static const int BLACK_KEY_NOTES[5];
    static const float BLACK_KEY_OFFSETS[5];
    static const char* NOTE_NAMES[12];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PianoKeyboardComponent)
};

//==============================================================================
// Main editor
//==============================================================================
class HelpyCordsEditor : public juce::AudioProcessorEditor,
                         public juce::Slider::Listener,
                         public juce::ComboBox::Listener,
                         public juce::Button::Listener,
                         public juce::Timer
{
public:
    explicit HelpyCordsEditor (HelpyCordsPlugin& p);
    ~HelpyCordsEditor() override;

    void paint   (juce::Graphics& g) override;
    void resized () override;

    void sliderValueChanged (juce::Slider*   s) override;
    void comboBoxChanged    (juce::ComboBox* c) override;
    void buttonClicked      (juce::Button*   b) override;
    void timerCallback      ()                  override;

private:
    HelpyCordsPlugin& processor;

    //--- Header ---------------------------------------------------------------
    juce::Label  titleLabel, authorLabel;

    //--- Controls panel -------------------------------------------------------
    juce::Label    instrumentLabel, chordLabel, keyLabel;
    juce::ComboBox instrumentBox, chordBox, keyBox;

    juce::ToggleButton autoCorrectButton;
    juce::Label        autoCorrectLabel;

    //--- Sliders --------------------------------------------------------------
    struct SliderRow
    {
        juce::Slider slider;
        juce::Label  label;
    };
    SliderRow sustainRow, decayRow, reverbRow, filterRow, volumeRow;
    SliderRow pitchRow, speedRow, toneRow, delayRow, compressionRow;

    void setupSlider (SliderRow& row, const juce::String& name,
                      double lo, double hi, double step, double val);

    //--- Chord info display ---------------------------------------------------
    juce::Label chordInfoLabel;

    //--- Piano keyboard -------------------------------------------------------
    std::unique_ptr<PianoKeyboardComponent> piano;

    void updateChordInfoLabel();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HelpyCordsEditor)
};
