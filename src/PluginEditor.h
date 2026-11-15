#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <array>
#include <vector>

class LowEndPilotEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit LowEndPilotEditor(LowEndPilotAudioProcessor& processor);
    void paint(juce::Graphics& graphics) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    LowEndPilotAudioProcessor& processor;
    juce::Image background;
    juce::Label title;
    juce::ComboBox presetSelector;
    juce::TextEditor presetName;
    juce::TextButton saveButton { "Save Preset" };
    std::array<Knob, 8> knobs;
    juce::Label magicModeLabel;
    juce::ComboBox magicModeSelector;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> magicModeAttachment;
    juce::ToggleButton automaticThumper { "Automatic Thumper" };
    juce::ToggleButton gainMatchToggle { "Gain Match" };
    juce::ToggleButton bypassToggle { "Bypass" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> automaticThumperAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> gainMatchAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
    std::vector<juce::File> userPresets;
    int lastFactoryProgram = 0;
    void refreshPresets();
    void savePreset();
    static juce::File presetDirectory();
    void timerCallback() override;
};
