#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>
#include <cmath>

namespace
{
constexpr float pi = juce::MathConstants<float>::pi;

struct FactoryPreset
{
    const char* name;
    float tight, punch, crossover, release, enhancer, mode, autoPunch, mix;
};

constexpr FactoryPreset factoryPresets[] = {
    {"Punchy Start", 72.0f, 55.0f, 130.0f, 160.0f, 35.0f, 1.0f, 1.0f, 100.0f},
    {"Gentle Glue", 38.0f, 25.0f, 100.0f, 280.0f, 10.0f, 0.0f, 1.0f, 85.0f},
    {"Kick Focus", 78.0f, 70.0f, 90.0f, 90.0f, 15.0f, 0.0f, 1.0f, 100.0f},
    {"Bass Tamer", 88.0f, 20.0f, 160.0f, 240.0f, 20.0f, 0.0f, 0.0f, 100.0f},
    {"Sub Clean", 45.0f, 20.0f, 115.0f, 200.0f, 80.0f, 0.0f, 0.0f, 100.0f},
    {"Low Heat", 60.0f, 45.0f, 180.0f, 170.0f, 75.0f, 1.0f, 1.0f, 100.0f},
    {"Mix Bus", 55.0f, 35.0f, 120.0f, 300.0f, 25.0f, 1.0f, 1.0f, 70.0f},
};

float coefficient(float milliseconds, double sampleRate)
{
    return std::exp(-1.0f / (0.001f * milliseconds * static_cast<float>(sampleRate)));
}

float clampFinite(float value)
{
    return std::isfinite(value) ? value : 0.0f;
}
}

LowEndPilotAudioProcessor::LowEndPilotAudioProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "LowEndPilotState", createParameterLayout())
{
    tight       = parameters.getRawParameterValue("tight");
    punch       = parameters.getRawParameterValue("punch");
    crossoverHz = parameters.getRawParameterValue("crossover");
    releaseMs   = parameters.getRawParameterValue("release");
    enhancer    = parameters.getRawParameterValue("enhancer");
    mode        = parameters.getRawParameterValue("mode");
    autoPunch   = parameters.getRawParameterValue("autoPunch");
    gainMatch   = parameters.getRawParameterValue("gainMatch");
    inputDb     = parameters.getRawParameterValue("input");
    outputDb    = parameters.getRawParameterValue("output");
    mix         = parameters.getRawParameterValue("mix");
    bypass      = parameters.getRawParameterValue("bypass");
}

juce::AudioProcessorValueTreeState::ParameterLayout LowEndPilotAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    auto percent = juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f);
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"tight", 1}, "Snugness", percent, 72.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"punch", 1}, "Thump Force", percent, 55.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"crossover", 1}, "Bass Boundary",
        juce::NormalisableRange<float>(45.0f, 240.0f, 1.0f, 0.5f), 130.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"release", 1}, "Release ms",
        juce::NormalisableRange<float>(50.0f, 600.0f, 1.0f, 0.5f), 160.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"enhancer", 1}, "Magic Sauce", percent, 35.0f));
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"mode", 1}, "Magic Mode",
        juce::StringArray{"Clean", "Color"}, 1));
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"autoPunch", 1}, "Automatic Thumper", true));
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"gainMatch", 1}, "Gain Match", false));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"input", 1}, "Input dB",
        juce::NormalisableRange<float>(-18.0f, 18.0f, 0.1f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"output", 1}, "Output dB",
        juce::NormalisableRange<float>(-18.0f, 18.0f, 0.1f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"mix", 1}, "Mix", percent, 100.0f));
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"bypass", 1}, "Bypass", false));
    return layout;
}

bool LowEndPilotAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto input = layouts.getMainInputChannelSet();
    return (input == juce::AudioChannelSet::mono() || input == juce::AudioChannelSet::stereo())
        && input == layouts.getMainOutputChannelSet();
}

void LowEndPilotAudioProcessor::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock)
{
    currentSampleRate = std::max(8000.0, sampleRate);
    crossover.prepare({currentSampleRate, static_cast<juce::uint32>(std::max(1, maximumExpectedSamplesPerBlock)), 2});
    crossover.setCutoffFrequency(crossoverHz->load());
    crossover.reset();
    std::fill(std::begin(subLowpass), std::end(subLowpass), 0.0f);
    fastEnvelope = slowEnvelope = levelSquared = inputPower = outputPower = 0.0f;
    compressionGain = matchedGain = 1.0f;
    modeBlend.reset(currentSampleRate, 0.03);
    modeBlend.setCurrentAndTargetValue(mode->load());
    gainMatchBlend.reset(currentSampleRate, 0.25);
    gainMatchBlend.setCurrentAndTargetValue(gainMatch->load());
}

void LowEndPilotAudioProcessor::releaseResources() {}

void LowEndPilotAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int channels = std::min(2, buffer.getNumChannels());
    const int samples = buffer.getNumSamples();
    if (channels == 0 || samples == 0) return;

    const float tightAmount = tight->load() * 0.01f;
    const float punchAmount = punch->load() * 0.01f;
    const float enhanceAmount = enhancer->load() * 0.01f;
    const float wetAmount = mix->load() * 0.01f;
    const float inputGain = juce::Decibels::decibelsToGain(inputDb->load());
    const float outputGain = juce::Decibels::decibelsToGain(outputDb->load());
    const bool useAutoPunch = autoPunch->load() > 0.5f;
    const bool useGainMatch = gainMatch->load() > 0.5f;
    const bool isBypassed = bypass->load() > 0.5f;
    modeBlend.setTargetValue(mode->load());
    gainMatchBlend.setTargetValue(useGainMatch ? 1.0f : 0.0f);
    crossover.setCutoffFrequency(std::min(crossoverHz->load(), static_cast<float>(currentSampleRate * 0.4)));

    const float attack = coefficient(12.0f, currentSampleRate);
    const float release = coefficient(releaseMs->load(), currentSampleRate);
    const float fast = coefficient(4.0f, currentSampleRate);
    const float slow = coefficient(85.0f, currentSampleRate);
    const float matchCoeff = coefficient(700.0f, currentSampleRate);
    const float subCoeff = std::exp(-2.0f * pi * 28.0f / static_cast<float>(currentSampleRate));
    const float thresholdDb = -14.0f - 28.0f * tightAmount;
    const float ratio = 1.0f + 10.0f * tightAmount;
    const float makeup = useAutoPunch ? juce::Decibels::decibelsToGain(2.0f * tightAmount) : 1.0f;

    auto* left = buffer.getWritePointer(0);
    auto* right = channels == 2 ? buffer.getWritePointer(1) : nullptr;

    for (int sample = 0; sample < samples; ++sample)
    {
        const float original[2] = {clampFinite(left[sample]), right != nullptr ? clampFinite(right[sample]) : 0.0f};
        float low[2] = {}, high[2] = {}, dry[2] = {};
        for (int channel = 0; channel < channels; ++channel)
        {
            const float driven = original[channel] * inputGain;
            crossover.processSample(channel, driven, low[channel], high[channel]);
            dry[channel] = low[channel] + high[channel];
        }

        float power = 0.0f, absolute = 0.0f;
        for (int channel = 0; channel < channels; ++channel)
        {
            power += low[channel] * low[channel];
            absolute += std::abs(low[channel]);
        }
        power /= static_cast<float>(channels);
        absolute /= static_cast<float>(channels);
        levelSquared = (power > levelSquared ? attack : release) * levelSquared
                     + (1.0f - (power > levelSquared ? attack : release)) * power;
        fastEnvelope = fast * fastEnvelope + (1.0f - fast) * absolute;
        slowEnvelope = slow * slowEnvelope + (1.0f - slow) * absolute;

        const float levelDb = 10.0f * std::log10(std::max(levelSquared, 1.0e-12f));
        const float reductionDb = std::max(0.0f, levelDb - thresholdDb) * (1.0f - 1.0f / ratio);
        const float targetGain = juce::Decibels::decibelsToGain(-reductionDb);
        const float gainCoeff = targetGain < compressionGain ? attack : release;
        compressionGain = gainCoeff * compressionGain + (1.0f - gainCoeff) * targetGain;
        const float transient = std::clamp((fastEnvelope - slowEnvelope) / (slowEnvelope + 0.04f), 0.0f, 1.0f);
        const float transientGain = 1.0f + punchAmount * 1.3f * transient;
        const float colorBlend = modeBlend.getNextValue();

        float wet[2] = {};
        float dryPower = 0.0f, wetPower = 0.0f;
        for (int channel = 0; channel < channels; ++channel)
        {
            subLowpass[channel] = subCoeff * subLowpass[channel] + (1.0f - subCoeff) * low[channel];
            const float clean = low[channel] - enhanceAmount * subLowpass[channel];
            const float drive = 1.0f + 2.0f * enhanceAmount;
            const float colored = (1.0f - enhanceAmount) * low[channel]
                                + enhanceAmount * std::tanh(low[channel] * drive) / std::tanh(drive);
            const float shaped = clean + colorBlend * (colored - clean);
            wet[channel] = high[channel] + shaped * compressionGain * transientGain * makeup;
            dryPower += dry[channel] * dry[channel];
            wetPower += wet[channel] * wet[channel];
        }
        dryPower /= static_cast<float>(channels);
        wetPower /= static_cast<float>(channels);
        inputPower = matchCoeff * inputPower + (1.0f - matchCoeff) * dryPower;
        outputPower = matchCoeff * outputPower + (1.0f - matchCoeff) * wetPower;
        const float targetMatch = std::clamp(std::sqrt((inputPower + 1.0e-9f) / (outputPower + 1.0e-9f)),
                                            0.5f, 1.122f);
        matchedGain = matchCoeff * matchedGain + (1.0f - matchCoeff) * targetMatch;
        const float matchAmount = gainMatchBlend.getNextValue();

        for (int channel = 0; channel < channels; ++channel)
        {
            const float leveledWet = wet[channel] * (1.0f + matchAmount * (matchedGain - 1.0f));
            const float processed = (dry[channel] + wetAmount * (leveledWet - dry[channel])) * outputGain;
            const float result = isBypassed ? original[channel] : processed;
            if (channel == 0) left[sample] = result;
            else right[sample] = result;
        }
    }
    crossover.snapToZero();
}

juce::AudioProcessorEditor* LowEndPilotAudioProcessor::createEditor()
{
    return new LowEndPilotEditor(*this);
}

int LowEndPilotAudioProcessor::getNumPrograms() { return static_cast<int>(std::size(factoryPresets)); }
int LowEndPilotAudioProcessor::getCurrentProgram() { return currentProgram.load(); }

const juce::String LowEndPilotAudioProcessor::getProgramName(int index)
{
    return juce::isPositiveAndBelow(index, getNumPrograms()) ? factoryPresets[index].name : juce::String{};
}

void LowEndPilotAudioProcessor::setCurrentProgram(int index)
{
    if (!juce::isPositiveAndBelow(index, getNumPrograms())) return;
    const auto& preset = factoryPresets[index];
    auto set = [this](const char* id, float value)
    {
        if (auto* parameter = parameters.getParameter(id))
            parameter->setValueNotifyingHost(parameters.getParameterRange(id).convertTo0to1(value));
    };
    set("tight", preset.tight);
    set("punch", preset.punch);
    set("crossover", preset.crossover);
    set("release", preset.release);
    set("enhancer", preset.enhancer);
    set("mode", preset.mode);
    set("autoPunch", preset.autoPunch);
    set("gainMatch", 0.0f);
    set("input", 0.0f);
    set("output", 0.0f);
    set("mix", preset.mix);
    set("bypass", 0.0f);
    currentProgram.store(index);
}

void LowEndPilotAudioProcessor::getStateInformation(juce::MemoryBlock& destination)
{
    auto state = parameters.copyState();
    state.setProperty("factoryProgram", currentProgram.load(), nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary(*xml, destination);
}

void LowEndPilotAudioProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(parameters.state.getType()))
        {
            auto state = juce::ValueTree::fromXml(*xml);
            currentProgram.store(std::clamp(static_cast<int>(state.getProperty("factoryProgram", 0)),
                                            0, getNumPrograms() - 1));
            parameters.replaceState(state);
        }
}

bool LowEndPilotAudioProcessor::savePresetToFile(const juce::File& file)
{
    juce::MemoryBlock data;
    getStateInformation(data);
    return data.getSize() > 0 && file.replaceWithData(data.getData(), data.getSize());
}

bool LowEndPilotAudioProcessor::loadPresetFromFile(const juce::File& file)
{
    if (!file.existsAsFile() || file.getSize() < 1 || file.getSize() > 1024 * 1024) return false;
    juce::MemoryBlock data;
    if (!file.loadFileAsData(data)) return false;
    auto xml = getXmlFromBinary(data.getData(), static_cast<int>(data.getSize()));
    if (xml == nullptr || !xml->hasTagName(parameters.state.getType())) return false;
    setStateInformation(data.getData(), static_cast<int>(data.getSize()));
    return true;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LowEndPilotAudioProcessor();
}
