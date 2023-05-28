# GranularSynth

An implementation of pitch-synchronous overlap add granular synthesis.

## Background

Given grains with a known approximate period, the grains can be streamed at a rate proportional to the period of each grain to created a pitched tone with timbre reminiscient of the original sample. (See here for more details)[https://www.jstor.org/stable/3679554]

## Running the project

- ((Install JUCE)[https://juce.com/download/])
- Clone the repository
- Open the .jucer file in the projucer
- Select your build system of choice and open your IDE using the projucer (or just save the .jucer file if using a makefile)
- Compile and run

### Using different grains

Currently, a few grains are compiled into the executable for use with the synthesizer. You can make other grains yourself or using the script in the `grain-extractor` directory.


