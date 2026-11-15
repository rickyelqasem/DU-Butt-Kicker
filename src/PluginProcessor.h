#pragma once

#include <JuceHeader.h>

class LowEndPilotAudioProcessor final : public juce::AudioProcessor
{
public:
    LowEndPilotAudioProcessor();
    ~LowEndPilotAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int) override;
    const juce::String getProgramName(int) override;
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    bool savePresetToFile(const juce::File& file);
    bool loadPresetFromFile(const juce::File& file);

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState parameters;

private:
    juce::dsp::LinkwitzRileyFilter<float> crossover;
    double currentSampleRate = 44100.0;
    float subLowpass[2] = {};
    float fastEnvelope = 0.0f;
    float slowEnvelope = 0.0f;
    float levelSquared = 0.0f;
    float compressionGain = 1.0f;
    float inputPower = 0.0f;
    float outputPower = 0.0f;
    float matchedGain = 1.0f;
    juce::SmoothedValue<float> modeBlend;
    juce::SmoothedValue<float> gainMatchBlend;
    std::atomic<int> currentProgram { 0 };

    std::atomic<float>* tight = nullptr;
    std::atomic<float>* punch = nullptr;
    std::atomic<float>* crossoverHz = nullptr;
    std::atomic<float>* releaseMs = nullptr;
    std::atomic<float>* enhancer = nullptr;
    std::atomic<float>* mode = nullptr;
    std::atomic<float>* autoPunch = nullptr;
    std::atomic<float>* gainMatch = nullptr;
    std::atomic<float>* inputDb = nullptr;
    std::atomic<float>* outputDb = nullptr;
    std::atomic<float>* mix = nullptr;
    std::atomic<float>* bypass = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LowEndPilotAudioProcessor)
};
