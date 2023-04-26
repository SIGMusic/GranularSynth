#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent() : audioSetupComp (deviceManager, 0, 0, 0, 256,
                                                 true, // showMidiInputOptions must be true
                                                 true,
                                                 true,
                                                 false)
{
    // Some platforms require permissions to open input channels so request that here
    if (juce::RuntimePermissions::isRequired (juce::RuntimePermissions::recordAudio)
        && ! juce::RuntimePermissions::isGranted (juce::RuntimePermissions::recordAudio))
    {
        juce::RuntimePermissions::request (juce::RuntimePermissions::recordAudio,
                                           [&] (bool granted) { setAudioChannels (granted ? 2 : 0, 2); });
    }
    else
    {
        // Specify the number of input and output channels that we want to open
        setAudioChannels (2, 2);
    }

    deviceManager.addMidiInputDeviceCallback ({}, this);

    addAndMakeVisible(audioSetupComp);

    attack_.setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag);
    attack_.setRange(0.0, 5.0);
    attack_.setValue(0.1);
    attack_.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 50, 10);
    decay_.setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag);
    decay_.setRange(0.0, 5.0);
    decay_.setValue(0.1);
    decay_.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 50, 10);
    sustain_.setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag);
    sustain_.setRange(0.0, 1.0);
    sustain_.setValue(0.9);
    sustain_.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 50, 10);
    release_.setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag);
    release_.setRange(0.0, 5.0);
    release_.setValue(0.1);
    release_.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 50, 10);

    addAndMakeVisible(attack_);
    addAndMakeVisible(decay_);
    addAndMakeVisible(sustain_);
    addAndMakeVisible(release_);

    attack_.addListener(this);
    decay_.addListener(this);
    sustain_.addListener(this);
    release_.addListener(this);

    setupBuiltinGrains();

    // Make sure you set the size of the component after
    // you add any child components.
    setSize (kWindowWidth, 400 + kKeyboardHeight + kSliderHeight);
}

void MainComponent::setupBuiltinGrains()
{
    grain_dropdown_.addItem("Custom Grain", 1);
    grain_dropdown_.addSeparator();

    for (int idx = 0; idx < BinaryData::namedResourceListSize; ++idx)
    {
        const char* rname = BinaryData::namedResourceList[idx];
        std::string resource_name(rname);

        size_t first_plus_pos = resource_name.find("_");
        std::size_t second_plus_pos = resource_name.find("_", first_plus_pos + 1);

        std::string grain_name = resource_name.substr(0, first_plus_pos);
        std::string grain_freq_str = resource_name.substr(first_plus_pos + 1, second_plus_pos - first_plus_pos - 1);

        std::cout << grain_name << std::endl;
        std::cout << grain_freq_str << std::endl;

        float freq = 0.0f;
        try
        {
            freq = std::stof(grain_freq_str);  // parse substring2 as a float
        }
        catch (const std::invalid_argument e)
        {
            std::cerr << "Could not load grain: <" << rname << ">" << std::endl;
            std::cerr << e.what() << std::endl;
            continue;
        }
        catch (const std::out_of_range& e)
        {
            std::cerr << "Could not load grain: <" << rname << ">" << std::endl;
            std::cerr << e.what() << std::endl;
            continue;
        }

        builtin_grains_.push_back({rname, freq});
        grain_dropdown_.addItem(grain_name, idx + kBuiltinGrainIdOffset);
        fprintf(stderr, "HERE\n");
    }

    addAndMakeVisible(&grain_dropdown_);
    grain_dropdown_.addListener(this);
}

MainComponent::~MainComponent()
{
    deviceManager.removeMidiInputDeviceCallback ({}, this);
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    samples_per_block_ = samplesPerBlockExpected;
    sample_rate_ = sampleRate;
    if (synth_)
        synth_->prepareToPlay(samplesPerBlockExpected, sampleRate);
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    if (synth_)
        synth_->getNextAudioBlock(bufferToFill);
}

void MainComponent::releaseResources()
{
    if (synth_)
        synth_->releaseResources();
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void MainComponent::resized()
{
    auto local_bounds = getLocalBounds();
    if (synth_)
        synth_->setBounds(local_bounds.removeFromBottom(kKeyboardHeight));
    else
        (void) local_bounds.removeFromBottom(kKeyboardHeight);
    auto slider_bounds = local_bounds.removeFromBottom(kSliderHeight);
    for (auto* slider : {&attack_, &decay_, &sustain_, &release_})
    {
        slider->setBounds(slider_bounds.removeFromLeft(kSliderWidth));
    }
    auto dropdown_bounds = local_bounds.removeFromTop(kDropdownHeight);
    grain_dropdown_.setBounds(dropdown_bounds);
    audioSetupComp.setBounds(local_bounds);
}

void MainComponent::sliderValueChanged(juce::Slider* slider)
{
    if (synth_)
    {
        if (slider == &attack_)
        {
            for (auto* voice : synth_->getVoices())
            {
                voice->setAttack(attack_.getValue());
            }
        }
        else if (slider == &decay_)
        {
            for (auto* voice : synth_->getVoices())
            {
                voice->setDecay(decay_.getValue());
            }
        }
        else if (slider == &sustain_)
        {
            for (auto* voice : synth_->getVoices())
            {
                voice->setSustain(sustain_.getValue());
            }
        }
        else if (slider == &release_)
        {
            for (auto* voice : synth_->getVoices())
            {
                voice->setRelease(release_.getValue());
            }
        }
    }
}

bool MainComponent::isInterestedInFileDrag(const StringArray& files)
{
    return files.size() == 1;
}

void MainComponent::filesDropped(const StringArray& files, int /**/, int /**/)
{
    grain_filepath_.clear();
    grain_filepath_ = files[0];

   AlertWindow::showAsync (MessageBoxOptions()
                          .withIconType (MessageBoxIconType::QuestionIcon)
                          .withTitle ("Enter a grain frequency")
                          .withButton ("OK")
                          .withButton ("Cancel"), [this](int retval){ grainDropCallback(retval); });
}

void MainComponent::grainDropCallback(int retval)
{
    if (retval == 1)
    {
        // float grain_freq = std::stof(alert_window_.getTextEditorContents("Grain Frequency").toStdString());
        float grain_freq = 440.0;
        juce::File grain_file(grain_filepath_);
        if (!resetSynth(&grain_file, grain_freq))
        {
            juce::AlertWindow::showAsync(MessageBoxOptions()
                                            .withIconType(MessageBoxIconType::WarningIcon)
                                            .withTitle("Could not load grain")
                                            .withButton("OK"), nullptr);
            return;
        }
        grain_dropdown_.setSelectedId(kFileGrainId);
    }
}

void MainComponent::comboBoxChanged(ComboBox* comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged == &grain_dropdown_)
    {
        int selected_id = comboBoxThatHasChanged->getSelectedId();
        if (selected_id < kBuiltinGrainIdOffset)
            return;

        BuiltinGrain& sel_bg =
            builtin_grains_[selected_id - kBuiltinGrainIdOffset];
        resetSynth(sel_bg.rname, sel_bg.freq);
    }
}

bool MainComponent::resetSynth(juce::File* grain_file, float grain_freq)
{
    if (grain_freq < 1.0 || grain_freq > 20000.0)
        return false;

    // Create an AudioFormatReader object for the audio file
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    return resetSynth(formatManager.createReaderFor(*grain_file), grain_freq);
}

bool MainComponent::resetSynth(const char* resource_name, float grain_freq)
{
    if (grain_freq < 1.0 || grain_freq > 20000.0)
        return false;

    // Create an AudioFormatReader object for the audio file
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    juce::AudioFormatReader* raw_reader = nullptr;
    
    int grain_size;
    const char* grain_data = BinaryData::getNamedResource(resource_name, grain_size);
    auto instream = std::make_unique<MemoryInputStream>((const void*) grain_data,
                                                        static_cast<size_t>(grain_size),
                                                        false);
    return resetSynth(formatManager.createReaderFor(std::move(instream)),
                      grain_freq);
}

bool MainComponent::resetSynth(juce::AudioFormatReader* raw_reader, float grain_freq)
{
    if (grain_freq < 1.0 || grain_freq > 20000.0)
        return false;

    auto reader = std::unique_ptr<AudioFormatReader>(raw_reader);

    if (reader != nullptr)
    {
        auto buffer = std::make_unique<AudioSampleBuffer>(1, reader->lengthInSamples);
  
        reader->read(buffer.get(), 0, reader->lengthInSamples, 0, true, true);

        dsp::WindowingFunction<float> window(buffer->getNumSamples(), dsp::WindowingFunction<float>::hann);

        float* channelData = buffer->getWritePointer(0);
        window.multiplyWithWindowingTable(channelData, buffer->getNumSamples());

        synth_ = std::make_unique<SynthKeyboard>(*buffer, grain_freq);

        addAndMakeVisible(synth_.get());

        synth_->prepareToPlay(samples_per_block_, sample_rate_);
        resized();

        return true;
    }
    fprintf(stderr, "Failed to initialize synth");

    synth_ = nullptr;
    return false;
}
