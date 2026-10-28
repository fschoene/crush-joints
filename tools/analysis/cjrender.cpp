// Offline renderer for the CrushJoints DSP chain, for A/B tests against the original plugin.
//   cjrender in.f32 out.f32 [sampleRate] [blockSize] key=value ...
// Raw interleaved stereo float32. Keys use CrushJoints units: tube mech tube_on cutoff(Hz) res
// lpf_on amount phat comp_on boom volume(dB) mix master_on.
// The chain matches PluginProcessor::processBlock.
#include <juce_dsp/juce_dsp.h>

#include "DSP/Compressor.h"
#include "DSP/Distortion.h"
#include "DSP/LadderFilter.h"
#include "DSP/Master.h"

#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        std::fprintf(stderr, "usage: cjrender in.f32 out.f32 [sr] [block] key=value...\n");
        return 1;
    }

    double sampleRate = 44100.0;
    int blockSize = 512;
    int next = 3;

    if (argc > next && std::string(argv[next]).find('=') == std::string::npos)
        sampleRate = std::atof(argv[next++]);

    if (argc > next && std::string(argv[next]).find('=') == std::string::npos)
        blockSize = std::atoi(argv[next++]);

    std::map<std::string, float> p {
        { "tube", 0.0f }, { "mech", 0.0f }, { "tube_on", 1.0f }, { "cutoff", 16000.0f }, { "res", 0.0f },
        { "lpf_on", 0.0f }, { "amount", 0.0f }, { "phat", 0.0f }, { "comp_on", 1.0f }, { "boom", 0.0f },
        { "volume", 0.0f }, { "mix", 1.0f }, { "master_on", 1.0f },
    };

    for (int i = next; i < argc; ++i)
    {
        const std::string arg = argv[i];
        const auto eq = arg.find('=');
        if (eq != std::string::npos)
            p[arg.substr(0, eq)] = static_cast<float>(std::atof(arg.substr(eq + 1).c_str()));
    }

    std::FILE* f = std::fopen(argv[1], "rb");
    if (f == nullptr)
        return 1;

    std::fseek(f, 0, SEEK_END);
    const long bytes = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    std::vector<float> data(static_cast<size_t>(bytes) / sizeof(float));
    if (std::fread(data.data(), sizeof(float), data.size(), f) != data.size())
        return 1;
    std::fclose(f);

    const int frames = static_cast<int>(data.size() / 2);

    CrushJoints::Distortion distortion;
    CrushJoints::LadderFilter filter;
    CrushJoints::Compressor compressor;
    CrushJoints::Master master;

    distortion.prepareToPlay(sampleRate);
    filter.prepareToPlay(sampleRate, blockSize, 2);
    compressor.prepareToPlay(sampleRate, blockSize, 2);
    master.prepareToPlay(sampleRate, blockSize, 2);

    distortion.setTubeAmount(p["tube"]);
    distortion.setMechAmount(p["mech"]);
    distortion.setTubeOn(p["tube_on"] > 0.5f);
    filter.setCutoff(p["cutoff"]);
    filter.setResonance(p["res"]);
    filter.setFilterOn(p["lpf_on"] > 0.5f);
    compressor.setAmount(p["amount"]);
    compressor.setPhatMode(p["phat"] > 0.5f);
    compressor.setCompressorOn(p["comp_on"] > 0.5f);
    compressor.setBoomMode(p["boom"] > 0.5f);
    compressor.reset();
    master.setVolume(p["volume"]);
    master.setMix(p["mix"]);
    master.setMasterOn(p["master_on"] > 0.5f);

    for (int offset = 0; offset < frames; offset += blockSize)
    {
        const int count = std::min(blockSize, frames - offset);
        juce::AudioBuffer<float> block(2, count);
        juce::AudioBuffer<float> dry(2, count);

        for (int i = 0; i < count; ++i)
            for (int ch = 0; ch < 2; ++ch)
                block.setSample(ch, i, data[static_cast<size_t>(2 * (offset + i) + ch)]);

        dry.makeCopyOf(block);

        distortion.processMech(block);
        filter.processBlock(block);
        distortion.processTube(block);
        compressor.processBlock(block);
        master.processBlock(block, &dry);

        for (int i = 0; i < count; ++i)
            for (int ch = 0; ch < 2; ++ch)
                data[static_cast<size_t>(2 * (offset + i) + ch)] = block.getSample(ch, i);
    }

    f = std::fopen(argv[2], "wb");
    std::fwrite(data.data(), sizeof(float), data.size(), f);
    std::fclose(f);
    return 0;
}
