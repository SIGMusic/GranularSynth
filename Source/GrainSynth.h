#pragma once

#include <JuceHeader.h>

#include "CustomADSR.h"

//==============================================================================
/*
*/
class GrainSynth : public juce::ToneGeneratorAudioSource
{
public:
    GrainSynth(const juce::AudioSampleBuffer& grain,
                   float grain_freq,
                   float attack_time = 0.1,
                   float decay_time = 0.2,
                   float sustain_frac = 0.9,
                   float release_time = 0.1) :
        grain_(grain),
        grain_freq_(grain_freq),
        adsr_parameters_(CustomADSR::Parameters(attack_time, decay_time, sustain_frac, release_time, 256)),
        adsr_(CustomADSR(adsr_parameters_))
    {
        table_size_ = grain.getNumSamples();
    }

    ~GrainSynth() override
    { /* Nothing */ }

    void noteOn(float amp)
    {
        adsr_.noteOn();
        amp_ = amp;
    }

    void noteOff()
    {
        adsr_.noteOff();
    }

    void setFrequency(float frequency)
    {
        freq_ = frequency;
        prepareToPlay(0, sample_rate_);
    }

    void setAttack(float attack_time)
    {
        adsr_parameters_.attack = attack_time;
        adsr_parameters_.attackEnv = CustomADSR::Parameters::EXP_GRO_ENV<4, 1>;
        adsr_.setParameters(adsr_parameters_);
        adsr_.reset();
    }

    void setDecay(float decay_time)
    {
        adsr_parameters_.decay = decay_time;
        adsr_.setParameters(adsr_parameters_);
        adsr_parameters_.decayEnv = CustomADSR::Parameters::EXP_DEC_ENV<3, 1>;
        adsr_.reset();
    }
 
    void setSustain(float sustain_frac)
    {
        adsr_parameters_.sustain = sustain_frac;
        adsr_.setParameters(adsr_parameters_);
        adsr_.reset();
    }

    void setRelease(float release_time)
    {
        adsr_parameters_.release = release_time;
        adsr_parameters_.releaseEnv = CustomADSR::Parameters::EXP_DEC_ENV<3, 1>;
        adsr_.setParameters(adsr_parameters_);
        adsr_.reset();
    }

    bool isActive()
    {
        return adsr_.isActive();
    }

    /**
    Calculates the trigger frequency of the grain
    */
    virtual void prepareToPlay(
        int /* parameter not needed */, double sampleRate) override
    {
        sample_rate_ = sampleRate;
        trigger_samples_ = (float) table_size_ * grain_freq_ / (2 * freq_);
        adsr_.setSampleRate(sampleRate);
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
            buf0[idx] = adsr_.getNextSample() * getNextSample() * amp_;
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
        auto* grain_readptr = grain_.getReadPointer(0);

        if (accumulator_ > trigger_samples_ &&
            curr_num_grains_ < max_num_grains_)
        {
            ++curr_num_grains_;
            ++gidx_end_;
            gidx_end_ %= max_num_grains_;
            accumulator_ -= trigger_samples_;
            // ringbuf_hist_.push_back(std::vector<unsigned int>(grain_idx_ringbuf_, grain_idx_ringbuf_ + max_num_grains_)); // TODO
        }

        float outsample = 0.0f;
        int end = gidx_start_ + curr_num_grains_;
        for (int idx_idx = gidx_start_; idx_idx < end; ++idx_idx)
        {
            unsigned int grain_idx =
                grain_idx_ringbuf_[idx_idx % max_num_grains_]++;
            if (grain_idx >= table_size_)
            {
                grain_idx_ringbuf_[gidx_start_++] = 0;
                gidx_start_ %= max_num_grains_;
                --curr_num_grains_;
                // ringbuf_hist_.push_back(std::vector<unsigned int>(grain_idx_ringbuf_, grain_idx_ringbuf_ + max_num_grains_)); // TODO
                continue;
            }
            outsample += grain_readptr[grain_idx];
        }

        accumulator_ += 1.0f;
        return outsample;
    }

private:
    // Begin grain data
    juce::AudioSampleBuffer grain_;
    unsigned int table_size_;
    float grain_freq_;
    double sample_rate_ = 48000.0;
    float freq_ = 440.0f;

    float trigger_samples_ = 0.0f; // to be calculated
    float accumulator_ = 0.0f;
    static const int max_num_grains_ = 64;
    unsigned int grain_idx_ringbuf_[max_num_grains_] = {0};
    unsigned int gidx_start_ = 0;
    unsigned int gidx_end_ = 1;
    unsigned int curr_num_grains_ = 1;
    // End grain data

    CustomADSR::Parameters adsr_parameters_;
    CustomADSR adsr_;
    float amp_ = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GrainSynth)
};

