#include "PluginEditor.h"
#include "BinaryData.h"
#include <algorithm>

LowEndPilotEditor::LowEndPilotEditor(LowEndPilotAudioProcessor& owner)
    : AudioProcessorEditor(owner), processor(owner)
{
    background = juce::ImageCache::getFromMemory(BinaryData::dubackground_png,
                                                  BinaryData::dubackground_pngSize);
    title.setText("DU - BUTT-KICKER", juce::dontSendNotification);
    title.setFont(juce::FontOptions(21.0f, juce::Font::bold));
    title.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(title);

    presetSelector.setComponentID("factoryPresets");
    lastFactoryProgram = processor.getCurrentProgram();
    refreshPresets();
    presetSelector.setSelectedId(lastFactoryProgram + 1, juce::dontSendNotification);
    presetSelector.onChange = [this]
    {
        const int selected = presetSelector.getSelectedId();
        if (selected >= 1 && selected <= processor.getNumPrograms())
        {
            lastFactoryProgram = selected - 1;
            processor.setCurrentProgram(lastFactoryProgram);
            presetName.clear();
        }
        else if (selected >= 1000)
        {
            const int index = selected - 1000;
            if (juce::isPositiveAndBelow(index, static_cast<int>(userPresets.size())))
            {
                if (processor.loadPresetFromFile(userPresets[static_cast<size_t>(index)]))
                {
                    lastFactoryProgram = processor.getCurrentProgram();
                    presetName.setText(userPresets[static_cast<size_t>(index)].getFileNameWithoutExtension(),
                                       juce::dontSendNotification);
                }
                else
                    juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
                        "Preset could not be loaded", "This preset file is invalid or could not be read.");
            }
        }
    };
    presetSelector.setColour(juce::ComboBox::backgroundColourId, juce::Colour::fromRGB(25, 19, 30));
    presetSelector.setColour(juce::ComboBox::textColourId, juce::Colours::white);
    presetSelector.setColour(juce::ComboBox::outlineColourId, juce::Colour::fromRGB(123, 77, 123));
    addAndMakeVisible(presetSelector);

    presetName.setTextToShowWhenEmpty("Name your preset", juce::Colours::grey);
    presetName.setComponentID("userPresetName");
    presetName.setColour(juce::TextEditor::backgroundColourId, juce::Colour::fromRGB(25, 19, 30));
    presetName.setColour(juce::TextEditor::textColourId, juce::Colours::white);
    presetName.setColour(juce::TextEditor::outlineColourId, juce::Colour::fromRGB(123, 77, 123));
    addAndMakeVisible(presetName);
    saveButton.setComponentID("savePreset");
    saveButton.onClick = [this] { savePreset(); };
    saveButton.setColour(juce::TextButton::buttonColourId, juce::Colour::fromRGB(111, 55, 111));
    saveButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    addAndMakeVisible(saveButton);

    constexpr std::array<std::pair<const char*, const char*>, 8> knobDefinitions {{
        {"tight", "Snugness"}, {"punch", "Thump Force"},
        {"crossover", "Bass Boundary"}, {"release", "Release ms"},
        {"enhancer", "Magic Sauce"}, {"input", "Input dB"},
        {"output", "Output dB"}, {"mix", "Mix"}
    }};
    for (size_t index = 0; index < knobs.size(); ++index)
    {
        auto& knob = knobs[index];
        const auto [parameterID, caption] = knobDefinitions[index];
        knob.label.setText(caption, juce::dontSendNotification);
        knob.label.setJustificationType(juce::Justification::centred);
        knob.label.setColour(juce::Label::textColourId, juce::Colours::white);
        knob.label.setFont(juce::FontOptions(14.0f, juce::Font::bold));
        addAndMakeVisible(knob.label);
        knob.slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        knob.slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 84, 24);
        knob.slider.setComponentID(parameterID);
        knob.slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour::fromRGB(232, 129, 193));
        knob.slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour::fromRGB(70, 57, 76));
        knob.slider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
        knob.slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour::fromRGB(22, 18, 28));
        knob.slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        addAndMakeVisible(knob.slider);
        knob.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.parameters, parameterID, knob.slider);
    }

    magicModeLabel.setText("Magic Mode", juce::dontSendNotification);
    magicModeLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    magicModeLabel.setFont(juce::FontOptions(14.0f, juce::Font::bold));
    addAndMakeVisible(magicModeLabel);
    magicModeSelector.addItem("Clean", 1);
    magicModeSelector.addItem("Color", 2);
    magicModeSelector.setComponentID("mode");
    magicModeSelector.setColour(juce::ComboBox::backgroundColourId, juce::Colour::fromRGB(25, 19, 30));
    magicModeSelector.setColour(juce::ComboBox::textColourId, juce::Colours::white);
    magicModeSelector.setColour(juce::ComboBox::outlineColourId, juce::Colour::fromRGB(123, 77, 123));
    addAndMakeVisible(magicModeSelector);
    magicModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.parameters, "mode", magicModeSelector);

    for (auto* button : {&automaticThumper, &gainMatchToggle, &bypassToggle})
    {
        button->setColour(juce::ToggleButton::textColourId, juce::Colours::white);
        button->setColour(juce::ToggleButton::tickColourId, juce::Colour::fromRGB(232, 129, 193));
        addAndMakeVisible(*button);
    }
    automaticThumper.setComponentID("autoPunch");
    gainMatchToggle.setComponentID("gainMatch");
    bypassToggle.setComponentID("bypass");
    automaticThumperAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processor.parameters, "autoPunch", automaticThumper);
    gainMatchAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processor.parameters, "gainMatch", gainMatchToggle);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processor.parameters, "bypass", bypassToggle);

    setSize(900, 650);
    startTimerHz(4);
}

void LowEndPilotEditor::paint(juce::Graphics& graphics)
{
    graphics.fillAll(juce::Colour::fromRGB(10, 9, 13));
    if (background.isValid())
    {
        const auto width = static_cast<float>(background.getWidth());
        const auto height = static_cast<float>(background.getHeight());
        const auto targetAspect = static_cast<float>(getWidth()) / static_cast<float>(getHeight());
        const auto cropHeight = std::min(height, width / targetAspect);
        const int sourceY = static_cast<int>((height - cropHeight) * 0.5f);
        graphics.setOpacity(0.55f);
        graphics.drawImage(background, 0, 0, getWidth(), getHeight(),
                           0, sourceY, background.getWidth(), static_cast<int>(cropHeight));
        graphics.setOpacity(1.0f);
    }
    graphics.setColour(juce::Colours::black.withAlpha(0.35f));
    graphics.fillRect(getLocalBounds());

    auto area = getLocalBounds().reduced(16);
    const auto header = area.removeFromTop(96);
    graphics.setColour(juce::Colour::fromRGB(14, 12, 19).withAlpha(0.82f));
    graphics.fillRoundedRectangle(header.toFloat(), 10.0f);
    auto grid = area.removeFromTop(410);
    constexpr int gap = 12;
    const int cellWidth = (grid.getWidth() - 3 * gap) / 4;
    const int cellHeight = (grid.getHeight() - gap) / 2;
    graphics.setColour(juce::Colour::fromRGB(16, 13, 21).withAlpha(0.76f));
    for (int index = 0; index < 8; ++index)
    {
        const int column = index % 4;
        const int row = index / 4;
        const juce::Rectangle<int> cell(grid.getX() + column * (cellWidth + gap),
                                         grid.getY() + row * (cellHeight + gap), cellWidth, cellHeight);
        graphics.fillRoundedRectangle(cell.toFloat(), 10.0f);
    }
    graphics.setColour(juce::Colour::fromRGB(14, 12, 19).withAlpha(0.82f));
    graphics.fillRoundedRectangle(area.toFloat(), 10.0f);
}

void LowEndPilotEditor::resized()
{
    auto area = getLocalBounds().reduced(16);
    auto header = area.removeFromTop(96).reduced(12, 8);
    auto firstRow = header.removeFromTop(36);
    presetSelector.setBounds(firstRow.removeFromRight(240));
    title.setBounds(firstRow);
    header.removeFromTop(6);
    saveButton.setBounds(header.removeFromRight(124));
    header.removeFromRight(8);
    presetName.setBounds(header);

    auto grid = area.removeFromTop(410);
    constexpr int gap = 12;
    const int cellWidth = (grid.getWidth() - 3 * gap) / 4;
    const int cellHeight = (grid.getHeight() - gap) / 2;
    for (int index = 0; index < 8; ++index)
    {
        const int column = index % 4;
        const int row = index / 4;
        auto cell = juce::Rectangle<int>(grid.getX() + column * (cellWidth + gap),
                                         grid.getY() + row * (cellHeight + gap), cellWidth, cellHeight);
        cell = cell.reduced(8);
        knobs[static_cast<size_t>(index)].label.setBounds(cell.removeFromTop(27));
        knobs[static_cast<size_t>(index)].slider.setBounds(cell);
    }
    auto options = area.reduced(14, 8);
    magicModeLabel.setBounds(options.removeFromLeft(95));
    magicModeSelector.setBounds(options.removeFromLeft(125).reduced(0, 16));
    options.removeFromLeft(16);
    automaticThumper.setBounds(options.removeFromLeft(190));
    gainMatchToggle.setBounds(options.removeFromLeft(135));
    bypassToggle.setBounds(options.removeFromLeft(100));
}

void LowEndPilotEditor::timerCallback()
{
    const int current = processor.getCurrentProgram();
    if (current != lastFactoryProgram)
    {
        lastFactoryProgram = current;
        presetSelector.setSelectedId(current + 1, juce::dontSendNotification);
    }
}

juce::File LowEndPilotEditor::presetDirectory()
{
    return juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
        .getChildFile("DU - Butt-Kicker").getChildFile("Presets");
}

void LowEndPilotEditor::refreshPresets()
{
    presetSelector.clear(juce::dontSendNotification);
    for (int index = 0; index < processor.getNumPrograms(); ++index)
        presetSelector.addItem(processor.getProgramName(index), index + 1);

    userPresets.clear();
    const auto directory = presetDirectory();
    if (!directory.isDirectory()) return;
    auto files = directory.findChildFiles(juce::File::findFiles, false, "*.dubk");
    std::sort(files.begin(), files.end(), [](const juce::File& a, const juce::File& b)
    {
        return a.getFileName().compareIgnoreCase(b.getFileName()) < 0;
    });
    if (!files.isEmpty()) presetSelector.addSeparator();
    for (const auto& file : files)
    {
        userPresets.push_back(file);
        presetSelector.addItem("User: " + file.getFileNameWithoutExtension(),
                               1000 + static_cast<int>(userPresets.size()) - 1);
    }
}

void LowEndPilotEditor::savePreset()
{
    const auto name = juce::File::createLegalFileName(presetName.getText().trim());
    if (name.isEmpty())
    {
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon,
            "Preset name needed", "Type a preset name, then click Save Preset.");
        return;
    }
    const auto directory = presetDirectory();
    if (directory.createDirectory().failed())
    {
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
            "Preset could not be saved", "The preset folder could not be created.");
        return;
    }
    auto file = directory.getChildFile(name + ".dubk");
    for (int suffix = 2; file.existsAsFile(); ++suffix)
        file = directory.getChildFile(name + " (" + juce::String(suffix) + ").dubk");
    if (!processor.savePresetToFile(file))
    {
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
            "Preset could not be saved", "The preset file could not be written.");
        return;
    }
    refreshPresets();
    for (size_t index = 0; index < userPresets.size(); ++index)
        if (userPresets[index] == file)
        {
            presetSelector.setSelectedId(1000 + static_cast<int>(index), juce::dontSendNotification);
            break;
        }
}
