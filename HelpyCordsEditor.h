#pragma once

#include <JuceHeader.h>
#include "HelpyCordsPlugin.h"

class HelpyCordsEditor : public juce::AudioProcessorEditor,
                         public juce::Slider::Listener,
                         public juce::ComboBox::Listener,
                         public juce::Button::Listener
{
public:
    HelpyCordsEditor(HelpyCordsPlugin& p);
    ~HelpyCordsEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void sliderValueChanged(juce::Slider* slider) override;
    void comboBoxChanged(juce::ComboBox* comboBox) override;
    void buttonClicked(juce::Button* button) override;

private:
    HelpyCordsPlugin& processor;

    juce::Label titleLabel;
    juce::Label authorLabel;

    juce::ComboBox instrumentBox;
    juce::Label    instrumentLabel;

    juce::ComboBox chordTypeBox;
    juce::ComboBox keyBox;
    juce::Label    chordTypeLabel;
    juce::Label    keyLabel;

    juce::Slider sustainSlider;
    juce::Slider decaySlider;
    juce::Slider reverbSlider;
    juce::Slider filterSlider;
    juce::Slider volumeSlider;
    juce::Slider zoomSlider;

    juce::Label sustainLabel;
    juce::Label decayLabel;
    juce::Label reverbLabel;
    juce::Label filterLabel;
    juce::Label volumeLabel;
    juce::Label zoomLabel;

    juce::ToggleButton autoCorrectButton;

    // FIX: PianoKeyboard inherits mouseWheelMove from juce::Component directly.
    //      No extra "MouseWheelListener" base class needed (it doesn't exist in JUCE).
    class PianoKeyboard : public juce::Component
    {
    public:
        explicit PianoKeyboard(HelpyCordsPlugin& p);
        void paint(juce::Graphics& g) override;
        void mouseDown(const juce::MouseEvent& e) override;
        void mouseUp(const juce::MouseEvent& e) override;
        void mouseWheelMove(const juce::MouseEvent& e,
                            const juce::MouseWheelDetails& wheel) override;

        void setZoom(float z) { zoom = z; repaint(); }

    private:
        HelpyCordsPlugin&     processor;
        std::array<bool, 128> keyPressed;
        float zoom         = 1.0f;
        int   scrollOffset = 0;

        void sendMidiNote(int midiNote, bool noteOn);
    };

    std::unique_ptr<PianoKeyboard> keyboard;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HelpyCordsEditor)
};
