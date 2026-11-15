#include "PluginProcessor.h"
#include <cmath>
#include <iostream>

namespace
{
void setParameter(LowEndPilotAudioProcessor& processor, const juce::String& id, float value)
{
    auto* parameter = processor.parameters.getParameter(id);
    const auto range = processor.parameters.getParameterRange(id);
    parameter->setValueNotifyingHost(range.convertTo0to1(value));
}

float renderRms(float frequency, float tightness, bool useGainMatch = false,
                bool useFactoryDefault = false)
{
    LowEndPilotAudioProcessor processor;
    setParameter(processor, "tight", tightness);
    if (!useFactoryDefault)
    {
        setParameter(processor, "punch", 0.0f);
        setParameter(processor, "enhancer", 0.0f);
        setParameter(processor, "autoPunch", 0.0f);
    }
    setParameter(processor, "gainMatch", useGainMatch ? 1.0f : 0.0f);
    processor.prepareToPlay(48000.0, 256);
    juce::AudioBuffer<float> buffer(2, 256);
    juce::MidiBuffer midi;
    double power = 0.0;
    int counted = 0;
    for (int block = 0; block < 400; ++block)
    {
        for (int sample = 0; sample < 256; ++sample)
        {
            const int position = block * 256 + sample;
            const float value = static_cast<float>(0.4 * std::sin(2.0 * juce::MathConstants<double>::pi
                                                  * frequency * position / 48000.0));
            buffer.setSample(0, sample, value);
            buffer.setSample(1, sample, value);
        }
        processor.processBlock(buffer, midi);
        if (block >= 300)
            for (int sample = 0; sample < 256; ++sample)
            {
                const double value = buffer.getSample(0, sample);
                power += value * value;
                ++counted;
            }
    }
    return static_cast<float>(std::sqrt(power / counted));
}
}

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;
    const float lowOpen = renderRms(80.0f, 0.0f);
    const float lowTight = renderRms(80.0f, 100.0f);
    const float highOpen = renderRms(2000.0f, 0.0f);
    const float highTight = renderRms(2000.0f, 100.0f);
    const float lowMatched = renderRms(80.0f, 100.0f, true);
    const float lowDefault = renderRms(80.0f, 72.0f, false, true);
    std::cout << "80 Hz RMS: " << lowOpen << " -> " << lowTight << '\n';
    std::cout << "2 kHz RMS: " << highOpen << " -> " << highTight << '\n';
    if (!(lowTight < lowOpen * 0.7f && highTight > highOpen * 0.9f)) return 1;
    std::cout << "Gain Match on at 80 Hz: " << lowTight << " -> " << lowMatched << '\n';
    if (!(lowMatched <= lowTight * 1.13f)) return 4;
    std::cout << "Default 80 Hz RMS: " << lowDefault << '\n';
    if (!(lowDefault < lowOpen * 0.85f)) return 8;

    LowEndPilotAudioProcessor processor;
    processor.prepareToPlay(48000.0, 64);
    setParameter(processor, "bypass", 1.0f);
    juce::AudioBuffer<float> buffer(1, 64);
    juce::MidiBuffer midi;
    for (int i = 0; i < 64; ++i) buffer.setSample(0, i, 0.2f * std::sin(i * 0.1f));
    juce::AudioBuffer<float> original(buffer);
    processor.processBlock(buffer, midi);
    for (int i = 0; i < 64; ++i)
        if (buffer.getSample(0, i) != original.getSample(0, i)) return 2;

    setParameter(processor, "tight", 73.0f);
    juce::MemoryBlock saved;
    processor.getStateInformation(saved);
    setParameter(processor, "tight", 0.0f);
    processor.setStateInformation(saved.getData(), static_cast<int>(saved.getSize()));
    if (std::abs(processor.parameters.getRawParameterValue("tight")->load() - 73.0f) > 0.01f) return 3;
    if (processor.getNumPrograms() < 6) return 5;
    if (std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
        editor->findChildWithID("factoryPresets") == nullptr
        || editor->findChildWithID("tight") == nullptr
        || editor->findChildWithID("punch") == nullptr
        || editor->findChildWithID("enhancer") == nullptr
        || editor->findChildWithID("mode") == nullptr
        || editor->findChildWithID("autoPunch") == nullptr) return 9;
    if (processor.parameters.getParameter("tight")->getName(100) != "Snugness"
        || processor.parameters.getParameter("punch")->getName(100) != "Thump Force"
        || processor.parameters.getParameter("crossover")->getName(100) != "Bass Boundary"
        || processor.parameters.getParameter("enhancer")->getName(100) != "Magic Sauce"
        || processor.parameters.getParameter("mode")->getName(100) != "Magic Mode"
        || processor.parameters.getParameter("autoPunch")->getName(100) != "Automatic Thumper") return 10;
    processor.setCurrentProgram(2);
    if (std::abs(processor.parameters.getRawParameterValue("crossover")->load() - 90.0f) > 0.01f) return 6;
    saved.reset();
    processor.getStateInformation(saved);
    processor.setCurrentProgram(1);
    processor.setStateInformation(saved.getData(), static_cast<int>(saved.getSize()));
    if (processor.getCurrentProgram() != 2) return 7;
    const auto userPreset = juce::File::getSpecialLocation(juce::File::tempDirectory)
        .getNonexistentChildFile("du-butt-kicker-smoke-preset", ".dubk");
    setParameter(processor, "tight", 61.0f);
    if (!processor.savePresetToFile(userPreset)) return 10;
    setParameter(processor, "tight", 0.0f);
    const bool loaded = processor.loadPresetFromFile(userPreset);
    userPreset.deleteFile();
    if (!loaded || std::abs(processor.parameters.getRawParameterValue("tight")->load() - 61.0f) > 0.01f)
        return 11;
    return 0;
}
