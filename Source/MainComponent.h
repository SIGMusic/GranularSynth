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
                       public juce::FileDragAndDropTarget,
                       public juce::ComboBox::Listener,
                       public juce::ChangeListener
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
    virtual void comboBoxChanged(ComboBox* comboBoxThatHasChanged) override;
    virtual void changeListenerCallback(ChangeBroadcaster* source) override;
    void grainDropCallback(int retval);
private:
    void setupBuiltinGrains();
    bool resetSynth(juce::AudioFormatReader* raw_reader, float grain_freq);
    bool resetSynth(juce::File* grain_file, float grain_freq);
    bool resetSynth(const char* resource_name, float grain_freq);
    //==============================================================================

    static const int kWindowWidth = 800;
    static const int kKeyboardHeight = 100; // pixels
    static const int kSliderHeight = 300; // pixels
    static const int kSliderWidth = kWindowWidth / 4; // pixels
    static const int kDropdownHeight = 30;
    static const int kCutoffHeight = 40;
    static const int kDefaultCutoff = 1000.0f;
    std::unique_ptr<SynthKeyboard> synth_ = nullptr;
    juce::AudioDeviceSelectorComponent audioSetupComp;

    juce::Slider attack_;
    juce::Slider decay_;
    juce::Slider sustain_;
    juce::Slider release_;
    juce::Slider cutoff_;

    juce::ComboBox grain_dropdown_;
    static const int kFileGrainId = 1;
    static const int kBuiltinGrainIdOffset = 2;
    typedef struct
    {
        const char* rname;
        float freq;
    } BuiltinGrain;
    std::vector<BuiltinGrain> builtin_grains_;

    int samples_per_block_;
    double sample_rate_;
    juce::BigInteger num_chans = 2;

    juce::String grain_filepath_;

    std::vector<juce::IIRFilter> lpfs_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
