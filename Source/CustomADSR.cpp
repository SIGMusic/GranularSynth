/*
  ==============================================================================

    CustomADSR.cpp
    Created: 27 Apr 2023 2:28:57pm
    Author:  Andrew Orals

  ==============================================================================
*/

#include "CustomADSR.h"

using namespace juce;

CustomADSR::CustomADSR(const CustomADSR::Parameters& parameters)
  : parameters_(parameters)
{
    recalculateRates();
  reset();
}

void CustomADSR::setParameters(const CustomADSR::Parameters& newParameters)
{
  parameters_ = newParameters;
    recalculateRates();
  reset();
}

void CustomADSR::setParameters(const juce::ADSR::Parameters& p)
{
  parameters_ = CustomADSR::Parameters(p.attack, p.decay, p.sustain, p.release);
    recalculateRates();
  reset();
}

const CustomADSR::Parameters& CustomADSR::getParameters() const noexcept
{
  return parameters_;
}

bool CustomADSR::isActive() const noexcept
{
  return adsr_state_ != Idle;
}

void CustomADSR::setSampleRate(double newSampleRate) noexcept
{
  jassert(newSampleRate > 0.0);
  sample_rate_ = newSampleRate;
  recalculateRates();
    reset();

    // TODO
    for (auto& t : {attack_env_table_, decay_env_table_, release_env_table_})
    {
        for (auto& e : t)
        {
            std::cerr << std::setw(10) << std::to_string(e);
        }
        std::cerr << std::endl;
    }
}

void CustomADSR::reset() noexcept
{
  gotoState(Idle);
  attack_samples_ = 0;
  decay_samples_ = 0;
  release_samples_ = 0;
  attack_idx_ = 0;
  decay_idx_ = 0;
  release_idx_ = 0;
  curr_attack_rate_ = 0.0f;
  curr_decay_rate_ = 0.0f;
  curr_release_rate_ = 0.0f;
  release_start_amp_ = parameters_.maxAmp;
}

void CustomADSR::noteOn() noexcept
{
  if (parameters_.attack > 0.0f)
  {
    gotoState(Attack);
  }
  else if (parameters_.decay > 0.0f)
  {
    gotoState(Decay);
  }
  else
  {
    gotoState(Sustain);
  }
}

void CustomADSR::noteOff() noexcept
{
  if (adsr_state_ != Idle)
  {
    if (parameters_.release > 0.0f)
    {
      gotoState(Release);
    }
    else
    {
      gotoState(Idle);
    }
  }
}

float CustomADSR::getNextSample() noexcept
{
  switch (adsr_state_)
  {
    case Idle:
      curr_amplitude_ = 0.0f;
      break;

    case Attack:
      // std::cerr << std::setw(10) << std::to_string(curr_amplitude_) << std::endl; // TODO
      // std::cerr << std::setw(10) << std::to_string(curr_attack_rate_) << std::endl; // TODO
      // std::cerr << std::setw(10) << std::to_string(attack_idx_) << std::endl;
      if (curr_amplitude_ >= parameters_.maxAmp)
      {
        gotoState(Decay);
        break;
      }

      calcRate(Attack);
      curr_amplitude_ += curr_attack_rate_;
      break;

    case Decay:
      if (curr_amplitude_ <= parameters_.sustain)
      {
        gotoState(Sustain);
        break;
      }

      calcRate(Decay);
      curr_amplitude_ += curr_decay_rate_;
      break;

    case Sustain:
      curr_amplitude_ = parameters_.sustain;
      break;

    case Release:
      if (curr_amplitude_ <= 0.0f)
      {
        gotoState(Idle);
        break;
      }

      calcRate(Release);
      curr_amplitude_ += curr_release_rate_;
      break;
  }

  return curr_amplitude_;
}

template <typename FloatType>
void CustomADSR::applyEnvelopeToBuffer(AudioBuffer<FloatType>& buffer, int startSample, int numSamples)
{
  jassert (startSample + numSamples <= buffer.getNumSamples());

  if (adsr_state_ == Idle)
  {
    buffer.clear (startSample, numSamples);
    return;
  }

  if (adsr_state_ == Sustain)
  {
    buffer.applyGain (startSample, numSamples, parameters_.sustain);
    return;
  }

  auto numChannels = buffer.getNumChannels();

  while (--numSamples >= 0)
  {
    auto env = getNextSample();

    for (int i = 0; i < numChannels; ++i)
      buffer.getWritePointer (i)[startSample] *= env;

    ++startSample;
  }
}

void CustomADSR::gotoState(State s) noexcept
{
  float from_amp = curr_amplitude_; // TODO
  switch (s)
  {
    case Idle:
      adsr_state_ = Idle;
      curr_amplitude_ = 0.0f;
      break;
    case Attack:
      adsr_state_ = Attack;
      attack_samples_ = 0;
      attack_idx_ = 0;
      curr_attack_rate_ = (attack_env_table_[1] - attack_env_table_[0]) *
          parameters_.maxAmp;
      break;
    case Decay:
      adsr_state_ = Decay;
      decay_samples_ = 0;
      decay_idx_ = 0;
      curr_amplitude_ = fmin(parameters_.maxAmp, curr_amplitude_);
      curr_decay_rate_ = (decay_env_table_[1] - decay_env_table_[0]) *
          (curr_amplitude_ - parameters_.sustain);
      break;
    case Sustain:
      adsr_state_ = Sustain;
      curr_amplitude_ = parameters_.sustain;
      break;
    case Release:
      adsr_state_ = Release;
      release_start_amp_ = curr_amplitude_;
      release_samples_ = 0;
      release_idx_ = 0;
      curr_amplitude_ = fmin(parameters_.maxAmp, curr_amplitude_);
      curr_release_rate_ = (release_env_table_[1] - release_env_table_[0]) *
          curr_amplitude_;
      break;
  }

  std::cerr << "from amp: " << std::setw(10) << std::to_string(from_amp); // TODO
  std::cerr << "to amp: " << std::setw(10) << std::to_string(curr_amplitude_) << std::endl; // TODO
  std::cerr << "from: " << std::to_string(s) << " to: " << std::to_string(adsr_state_) << std::endl; // TODO
}

void CustomADSR::calcRate(State s) noexcept
{
  int *tr, *es;
  unsigned int *i;
  float *cr;
  std::vector<float> *table;
  float scale;

  switch (s)
  {
    case Idle:
      return;
    case Attack:
      tr = &attack_table_rate_;
      es = &attack_samples_;
      i = &attack_idx_;
      cr = &curr_attack_rate_;
      table = &attack_env_table_;
      scale = parameters_.maxAmp;
      break;
    case Decay:
      tr = &decay_table_rate_;
      es = &decay_samples_;
      i = &decay_idx_;
      cr = &curr_decay_rate_;
      table = &decay_env_table_;
      scale = parameters_.maxAmp - parameters_.sustain;
      break;
    case Sustain:
      return;
    case Release:
      tr = &release_table_rate_;
      es = &release_samples_;
      i = &release_idx_;
      cr = &curr_release_rate_;
      table = &release_env_table_;
      scale = release_start_amp_;
      break;
  }

  ++(*es);
  if (*es >= *tr)
  {
    *es = 0;
    (*i) = (*i) < parameters_.env_resolution - 1 ? (*i) + 1 : (*i);
    *cr = ((*table)[(*i) + 1] - (*table)[(*i)]) * scale;
  }
}

void CustomADSR::recalculateRates() noexcept
{
  tabulateEnvelopes();

  auto calc_table_rate = [this](float sec)
  {
    return (fmax(fmax(sample_rate_ * sec, 1.0f) / parameters_.env_resolution, 1.0f));
  };

  attack_table_rate_ = static_cast<int>(calc_table_rate(parameters_.attack));
  decay_table_rate_ = static_cast<int>(calc_table_rate(parameters_.decay));
  release_table_rate_ = static_cast<int>(calc_table_rate(parameters_.release));

  // get rate of change per sample increment
  for (int i = 0; i <= parameters_.env_resolution; ++i)
  {
    attack_env_table_[i] /= calc_table_rate(parameters_.attack);
    decay_env_table_[i] /= calc_table_rate(parameters_.decay);
    release_env_table_[i] /= calc_table_rate(parameters_.release);
  }
}

void CustomADSR::tabulateEnvelopes()
{
  attack_env_table_.resize(parameters_.env_resolution + 1);
  decay_env_table_.resize(parameters_.env_resolution + 1);
  release_env_table_.resize(parameters_.env_resolution + 1);

  for (int i = 0; i <= parameters_.env_resolution; ++i)
  {
    attack_env_table_[i] = parameters_.attackEnv((float) i / parameters_.env_resolution);
    decay_env_table_[i] = parameters_.decayEnv((float) i / parameters_.env_resolution);
    release_env_table_[i] = parameters_.releaseEnv((float) i / parameters_.env_resolution);
  }
}

