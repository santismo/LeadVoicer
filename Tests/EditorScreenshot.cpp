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
    if (auto* source = processor.getValueTreeState().getParameter (ParameterIDs::sourceMode))
        source->setValueNotifyingHost (source->convertTo0to1 (1.0f));

    processor.setRateAndBufferSizeDetails (1000.0, 64);
    processor.prepareToPlay (1000.0, 64);
    const auto captureChord = [&processor] (std::initializer_list<int> notes)
    {
        juce::AudioBuffer<float> audio (1, 64);
        juce::MidiBuffer midi;
        auto offset = 0;
        for (const auto note : notes)
        {
            midi.addEvent (juce::MidiMessage::noteOn (1, note, static_cast<juce::uint8> (100)), offset);
            offset += 4;
        }
        for (const auto note : notes)
        {
            midi.addEvent (juce::MidiMessage::noteOff (1, note), offset);
            offset += 4;
        }
        processor.processBlock (audio, midi);
        for (int block = 0; block < 4; ++block)
        {
            juce::MidiBuffer silence;
            processor.processBlock (audio, silence);
        }
    };
    captureChord ({ 60, 64, 67, 71 });
    captureChord ({ 62, 65, 69, 72 });
    captureChord ({ 55, 59, 62, 65 });
    processor.setChordBankCardProbability (0, 0.82f);
    processor.setChordBankCardProbability (1, 0.48f);
    processor.setChordBankCardProbability (2, 0.68f);
    processor.setChordBankListening (false);

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
