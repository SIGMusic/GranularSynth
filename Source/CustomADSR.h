/*
  ==============================================================================

    CustomADSR.h
    Created: 27 Apr 2023 2:28:57pm
    Author:  Andrew Orals

  ==============================================================================
*/

#pragma once

#include "JuceHeader.h"

class CustomADSR
{
public:
  typedef std::function<float(float)> Envelope;

  struct Parameters : public juce::ADSR::Parameters
  {
    Parameters() = default;

    Parameters(float a, float d, float s, float r)
      : juce::ADSR::Parameters(a, d, s, r)
    { /* Nothing */ }

    Parameters(float attackTimeSeconds,
               float decayTimeSeconds,
               float sustainTimeSeconds,
               float releaseTimeSeconds,
               float maxAmplitude,
               const Envelope& attackEnvFunc,
               const Envelope& decayEnvFunc,
               const Envelope& releaseEnvFunc,
               size_t env_resolution=256)
      : juce::ADSR::Parameters(attackTimeSeconds,
                               decayTimeSeconds,
                               sustainTimeSeconds,
                               releaseTimeSeconds),
        maxAmp(maxAmplitude),
        attackEnv(attackEnvFunc),
        decayEnv(decayEnvFunc),
        releaseEnv(releaseEnvFunc),
        env_resolution(env_resolution)
    {
      jassert(env_resolution >= 2 && env_resolution <= 4096 /* arbitrary */);
    }

    float maxAmp = 1.0f;
    Envelope attackEnv = [](float x) { return x; },
             decayEnv = [](float x) { return -x; },
             releaseEnv = [](float x) { return -x; };
    size_t env_resolution = 2; // number of tabulations for each envelope
  };

  CustomADSR(const Parameters& newParameters);

  void setParameters (const Parameters& newParameters);

  void setParameters (const juce::ADSR::Parameters& newParameters);

  const Parameters& getParameters() const noexcept;

  bool isActive() const noexcept;

  void setSampleRate (double newSampleRate) noexcept;

  void reset() noexcept;

  void noteOn() noexcept;

  void noteOff() noexcept;

  float getNextSample() noexcept;

  template <typename FloatType>
  void applyEnvelopeToBuffer(juce::AudioBuffer<FloatType>& buffer,
                             int startSample,
                             int numSamples);

private:
  typedef enum
  {
    Idle,
    Attack,
    Decay,
    Sustain,
    Release
  } State;

  void gotoState(State s) noexcept;

  void calcRate(State s) noexcept;

  void recalculateRates() noexcept;

  void tabulateEnvelopes();

  State adsr_state_ = Idle;
  Parameters parameters_;
  double sample_rate_ = 44100.0;
  float curr_amplitude_ = 0.0f;

  std::vector<float> attack_env_table_, // tabulations of the envelope
                                        // functions
                     decay_env_table_,
                     release_env_table_;

  int attack_table_rate_ = 1, // after how many samples to move to
                              // the next rate
      decay_table_rate_ = 1,
      release_table_rate_ = 1;

  int attack_samples_ = 0, // number of samples elapsed for each
                           // state; reset on state change
      decay_samples_ = 0,
      release_samples_ = 0;

  unsigned int attack_idx_ = 0, // index into the respective tables
               decay_idx_ = 0,
               release_idx_ = 0;

  float curr_attack_rate_ = 0.0f,
        curr_decay_rate_ = 0.0f,
        curr_release_rate_ = 0.0f;

  float release_start_amp_ = 1.0f; // the amplitude at which release starts
};

