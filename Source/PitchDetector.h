/*
  ==============================================================================

    PitchDetector.h
    Created: 12 Apr 2023 6:54:19pm
    Author:  Andrew Orals

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
*/
class PitchDetector  : public juce::Component
{
public:
    PitchDetector();
    ~PitchDetector() override;
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PitchDetector)
};
