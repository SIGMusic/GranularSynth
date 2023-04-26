#pragma once

#include <JuceHeader.h>

#include "SynthKeyboard.h"
#include "GrainSynth.h"

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent  : public juce::AudioAppComponent,
                       public juce::MidiInputCallback,
                       public juce::Slider::Listener,
                       public juce::FileDragAndDropTarget
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    //==============================================================================
    void paint (juce::Graphics& g) override;
    void resized() override;

    //==============================================================================
    void handleIncomingMidiMessage (juce::MidiInput* /*source*/,
                                    const juce::MidiMessage& message) override
    {
        if (synth_)
            synth_->processMIDIMessage(message);
    }

    virtual void sliderValueChanged(juce::Slider* slider) override;

    virtual bool isInterestedInFileDrag(const StringArray& files) override;
    virtual void filesDropped(const StringArray& files, int x, int y) override;
    void grainDropCallback(int retval);
private:
    bool resetSynth(juce::File* grain_file, float grain_freq);
    //==============================================================================

    static const int kDefaultGrainFreq = 220.0;
    static const int kWindowWidth = 800;
    static const int kKeyboardHeight = 100; // pixels
    static const int kSliderHeight = 300; // pixels
    static const int kSliderWidth = kWindowWidth / 4; // pixels
    std::unique_ptr<SynthKeyboard> synth_ = nullptr;
    juce::AudioDeviceSelectorComponent audioSetupComp;

    juce::Slider attack_;
    juce::Slider decay_;
    juce::Slider sustain_;
    juce::Slider release_;

    int samples_per_block_;
    double sample_rate_;

    juce::String grain_filepath_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
