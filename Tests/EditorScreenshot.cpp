#include <JuceHeader.h>
#include "PluginEditor.h"
#include "PluginProcessor.h"

#include <iostream>

int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI initialiseJuce;
    SoliVoicerAudioProcessor processor;

    if (auto* skin = processor.getValueTreeState().getParameter (ParameterIDs::skin))
        skin->setValueNotifyingHost (skin->convertTo0to1 (7.0f));

    std::unique_ptr<juce::AudioProcessorEditor> editor (processor.createEditor());
    editor->setSize (1120, 880);
    constexpr auto scale = 1.5f;
    juce::Image image (juce::Image::ARGB, juce::roundToInt (editor->getWidth() * scale),
                       juce::roundToInt (editor->getHeight() * scale), true);
    {
        juce::Graphics graphics (image);
        graphics.addTransform (juce::AffineTransform::scale (scale));
        editor->paintEntireComponent (graphics, true);
    }
    const auto output = argc > 1 ? juce::File (argv[1])
                                 : juce::File::getCurrentWorkingDirectory().getChildFile ("voicizer-editor.png");

    output.getParentDirectory().createDirectory();
    output.deleteFile();
    auto stream = output.createOutputStream();
    juce::PNGImageFormat png;
    if (stream == nullptr || ! png.writeImageToStream (image, *stream))
    {
        std::cerr << "Could not write " << output.getFullPathName() << '\n';
        return 1;
    }

    std::cout << output.getFullPathName() << '\n';
    return 0;
}
