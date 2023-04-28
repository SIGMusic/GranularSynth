#pragma once

#include <JuceHeader.h>
#include "GrainSynth.h"

#include <map>

using std::unordered_map;

//==============================================================================
/*
*/
class SynthKeyboard  : public juce::Component,
                       public juce::MidiKeyboardState::Listener,
                       public juce::AudioSource
{
public:
    SynthKeyboard(const juce::AudioSampleBuffer& grain,
                  float grain_freq)
    {
        midi_keyboard_state_.addListener(this);
        midi_keyboard_.reset(new juce::MidiKeyboardComponent(midi_keyboard_state_,
                            juce::KeyboardComponentBase::Orientation::horizontalKeyboard));
        addAndMakeVisible(midi_keyboard_.get());

        for (int voice_idx = 0; voice_idx < max_voices_; ++voice_idx)
        {
            auto* synth = new GrainSynth(grain, grain_freq); // TODO
            mixer_.addInputSource(synth, false);
            free_voices_[num_free_voices_++] = synth;
            voices_.add(synth);
        }
    }

    virtual ~SynthKeyboard() = default;

    //==========================================================================
    // External MIDI

    void processMIDIMessage(const juce::MidiMessage& message)
    {
        if (message.isNoteOn())
        {
            handleNoteOn(nullptr,
                         message.getChannel(),
                         message.getNoteNumber(),
                         message.getVelocity());
        }
        else if (message.isNoteOff())
        {
            handleNoteOff(nullptr,
                          message.getChannel(),
                          message.getNoteNumber(),
                          message.getVelocity());
        }
    }
 

    //==========================================================================
    // AudioSource
 
    virtual void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override
    {
        mixer_.prepareToPlay(samplesPerBlockExpected, sampleRate);
    }
 
    virtual void releaseResources() override
    {
        mixer_.releaseResources();
    }

    virtual void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override
    {
        mixer_.getNextAudioBlock(bufferToFill);
    }

    //==========================================================================
    // MidiKeyboardState::Listener

    static constexpr inline float midiToFreq(juce::uint8 midi_note)
    {
        return 440.0 * std::pow(2.0, (midi_note - 69) / 12.0);
    }

    virtual void handleNoteOn(juce::MidiKeyboardState *source,
                              int midiChannel,
                              int midiNoteNumber,
                              float velocity) override
    {
        if (voice_mapping_.find(midiNoteNumber) != voice_mapping_.end())
        {
            voice_mapping_[midiNoteNumber]->setFrequency(
                (juce::uint8) midiToFreq(midiNoteNumber));
        }
        else if (num_free_voices_ > 0)
        {
            auto* voice = free_voices_[num_free_voices_ - 1];
            voice_mapping_[midiNoteNumber] = voice;
            auto freq = midiToFreq((juce::uint8) midiNoteNumber);
            voice->setFrequency(freq);
            voice->noteOn(0.5 / max_voices_);
            free_voices_[num_free_voices_ - 1] = nullptr;
            --num_free_voices_;
        }
    }

    virtual void handleNoteOff(juce::MidiKeyboardState *source,
                               int midiChannel,
                               int midiNoteNumber,
                               float velocity) override
    {
        std::unordered_map<int, GrainSynth*>::iterator iter;
        if ((iter = voice_mapping_.find(midiNoteNumber)) != voice_mapping_.end())
        {
            auto* voice = voice_mapping_[midiNoteNumber];
            voice->noteOff();
            voice_mapping_.erase(iter);
            free_voices_[num_free_voices_++] = voice;
        }
    }

    //==========================================================================
    // Component

    void paint (juce::Graphics& g) override { /* Nothing */ }

    void resized() override
    {
        midi_keyboard_->setBounds(getLocalBounds());
    }

    juce::OwnedArray<GrainSynth>& getVoices()
    {
        return voices_; // sorry, this is a hack
    }

private:
    // TODO
    static const int max_voices_ = 8;
    juce::OwnedArray<GrainSynth> voices_;
    juce::MixerAudioSource mixer_;

    int num_free_voices_ = 0;
    GrainSynth* free_voices_[max_voices_];
    unordered_map<int, GrainSynth*> voice_mapping_;

    juce::MidiKeyboardState midi_keyboard_state_;
    std::unique_ptr<juce::MidiKeyboardComponent> midi_keyboard_;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SynthKeyboard)
};
