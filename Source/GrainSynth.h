#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
*/
class GrainSynth : public juce::ToneGeneratorAudioSource
{
public:
    static const int TABLE_SIZE = 4096;

    GrainSynth(const juce::AudioSampleBuffer& grain,
                   float grain_freq,
                   float attack_time = 0.1,
                   float decay_time = 0.2,
                   float sustain_frac = 0.9,
                   float release_time = 0.1) :
        grain_(grain),
        grain_freq_(grain_freq),
        attack_time_(attack_time),
        decay_time_(decay_time),
        sustain_frac_(sustain_frac),
        release_time_(release_time)
    {
        jassert(grain.getNumSamples() == TABLE_SIZE);
    }

    ~GrainSynth() override { /* Nothing */ }

    void noteOn(float amp)
    {
        if (attack_time_ == 0.0f)
            adsr_state_ = Sustain;
        else
            adsr_state_ = Attack;
        max_amplitude_ = amp;
    }

    void noteOff()
    {
        adsr_state_ = Release;
    }

    void setFrequency(float frequency)
    {
        freq_ = frequency;
        prepareToPlay(0, sample_rate_);
    }

    void setAttack(float attack_time)
    {
        attack_time_ = attack_time;
    }

    void setDecay(float decay_time)
    {
        decay_time_ = decay_time;
    }
 
    void setSustain(float sustain_frac)
    {
        sustain_frac_ = sustain_frac;
    }

    void setRelease(float release_time)
    {
        release_time_ = release_time;
    }

    /**
    Calculates the trigger frequency of the grain
    */
    virtual void prepareToPlay(
        int /* parameter not needed */, double sampleRate) override
    {
        sample_rate_ = sampleRate;
        trigger_samples_ = (float) TABLE_SIZE * grain_freq_ / (2 * freq_);
    }

    /**
    This fills the audio buffer with the samples that the synth generates.
    */
    virtual void getNextAudioBlock(
        const juce::AudioSourceChannelInfo &bufferToFill) override
    {
        bufferToFill.clearActiveBufferRegion();
        auto* buf0 = bufferToFill.buffer->getWritePointer(0);

        for (unsigned int idx = 0; idx < bufferToFill.numSamples; ++idx)
        {
            buf0[idx] = calcAmplitude() * getNextSample();
        }

        // Duplicate signal across all channels
        for (unsigned int chan_idx = 1;
             chan_idx < bufferToFill.buffer->getNumChannels();
             ++chan_idx)
        {
            auto* buf = bufferToFill.buffer->getWritePointer(chan_idx);
            for (unsigned int idx = 0; idx < bufferToFill.numSamples; ++idx)
            {
                buf[idx] = buf0[idx];
            }
        }
    }

    /**
    Get the next sample
    */
    forcedinline float getNextSample() noexcept
    {
        jassert(curr_num_grains_ < max_num_grains_);
        auto* grain_readptr = grain_.getReadPointer(1);

        if (accumulator_ > trigger_samples_ &&
            curr_num_grains_ < max_num_grains_)
        {
            ++curr_num_grains_;
            grain_idx_ringbuf_[gidx_end_++] = 0;
            gidx_end_ %= max_num_grains_;
            accumulator_ -= trigger_samples_;
        }

        float outsample = 0.0f;
        for (int idx_idx = gidx_start_;
             idx_idx < gidx_start_ + curr_num_grains_;
             ++idx_idx)
        {
            unsigned int grain_idx =
                grain_idx_ringbuf_[idx_idx % max_num_grains_]++;
            if (grain_idx >= TABLE_SIZE)
            {
                ++gidx_start_;
                gidx_start_ %= max_num_grains_;
                --curr_num_grains_;
                continue;
            }
            outsample += grain_readptr[grain_idx];
        }

        accumulator_ += 1.0f;
        return outsample;
    }

private:

    float calcAmplitude()
    {
        // all in units of amplitude per sample
        int attack_samples = fmax(sample_rate_ * attack_time_, 1);
        int decay_samples = fmax(sample_rate_ * decay_time_, 1);
        int release_samples = fmax(sample_rate_ * release_time_, 1);

        float attack_rate = max_amplitude_ / attack_samples;
        float decay_rate = (max_amplitude_ * (1 - sustain_frac_)) / decay_samples;
        float release_rate = (max_amplitude_ * sustain_frac_) / release_samples;

        switch (adsr_state_)
        {
            case Attack:
                if (curr_amplitude_ >= max_amplitude_)
                {
                    curr_amplitude_ = max_amplitude_;
                    adsr_state_ = Decay;
                    break;
                }
                curr_amplitude_ += attack_rate;
                break;
            case Decay:
                if (curr_amplitude_ <= sustain_frac_ * max_amplitude_)
                {
                    curr_amplitude_ = sustain_frac_ * max_amplitude_;
                    adsr_state_ = Sustain;
                    break;
                }
                curr_amplitude_ -= decay_rate;
                break;
            case Sustain:
                curr_amplitude_ = sustain_frac_ * max_amplitude_;
                break;
            case Release:
                if (curr_amplitude_ <= 0)
                {
                    curr_amplitude_ = 0;
                    break;
                }
                curr_amplitude_ -= release_rate;
                break;
        }

        return curr_amplitude_;
    }
 

    // Begin grain data
    juce::AudioSampleBuffer grain_;
    float grain_freq_;
    double sample_rate_ = 48000.0;
    float freq_ = 440.0f;

    float trigger_samples_ = 0.0f; // to be calculated
    float accumulator_ = 0.0f;
    static const int max_num_grains_ = 128;
    unsigned int grain_idx_ringbuf_[max_num_grains_] = {0};
    unsigned int gidx_start_ = 0;
    unsigned int gidx_end_ = 1;
    unsigned int curr_num_grains_ = 1;
    // End grain data

    // Begin ADSR data
    typedef enum
    {
        Attack,
        Decay,
        Sustain,
        Release
    } State;
    State adsr_state_;
    float max_amplitude_ = 0.0f;
    float curr_amplitude_ = 0.0f;
    float attack_time_;
    float decay_time_;
    float sustain_frac_;         // fraction of maximum amplitude to sustain
    float release_time_;
    // End ADSR data

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GrainSynth)
};

//========================Misc Graphics Stuff=================================//
