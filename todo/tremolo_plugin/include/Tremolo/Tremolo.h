#pragma once

namespace tremolo {
class Tremolo {
public:
  enum class LfoWaveform : size_t { sine = 0, triangle = 1 };

  Tremolo() {
    for (auto& lfo : lfos) {
      lfo.setFrequency(5.0f, true);  // Set the LFO frequency to 5 Hz
    }
  }

  void prepare(double sampleRate, int expectedMaxFramesPerBlock) {
    const juce::dsp::ProcessSpec processSpec{
        .sampleRate = sampleRate,
        .maximumBlockSize =
            static_cast<juce::uint32>(expectedMaxFramesPerBlock),
        .numChannels = 1u};

    for (auto& lfo : lfos) {
      lfo.prepare(processSpec);
    }

    lfoSmoother.reset(sampleRate, 0.025f /* 25ms*/);
  }

  void setLfoWaveform(LfoWaveform waveform) noexcept {
    jassert(waveform == LfoWaveform::sine || waveform == LfoWaveform::triangle);
    lfoToSet = waveform;
  }

  void process(juce::AudioBuffer<float>& buffer) noexcept {
    // update the LFO waveform if it has changed
    updateLfoWaveform();

    // for each frame
    for (const auto frameIndex : std::views::iota(0, buffer.getNumSamples())) {
      // get the LFO value for this frame
      const auto lfoValue = getNextLfoValue();

      // calculate the modulation value
      constexpr auto modulationDepth = 0.4f;
      const auto modulationValue = modulationDepth * lfoValue + 1.f;

      // for each channel sample in the frame
      for (const auto channelIndex :
           std::views::iota(0, buffer.getNumChannels())) {
        // get the input sample
        const auto inputSample = buffer.getSample(channelIndex, frameIndex);

        // modulate the sample
        const auto outputSample = inputSample * modulationValue;

        // set the output sample
        buffer.setSample(channelIndex, frameIndex, outputSample);
      }
    }
  }

  void reset() noexcept {
    for (auto& lfo : lfos) {
      lfo.reset();
    }
  }

private:
  static float sine(float phase) { return std::sin(phase); }

  static float triangle(float phase) {
    return std::abs(2 * phase / juce::MathConstants<float>::pi) - 1.f;
  }

  std::array<juce::dsp::Oscillator<float>, 2u> lfos{
      juce::dsp::Oscillator<float>(sine),
      juce::dsp::Oscillator<float>(triangle)};

  LfoWaveform currentLfoWaveform = LfoWaveform::sine;
  LfoWaveform lfoToSet = currentLfoWaveform;

  // use smooth value to avoid clicks when switching LFO waveforms
  juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> lfoSmoother{
      0.f};

  void updateLfoWaveform() noexcept {
    // if lfo waveform has changed, update the smoother and set the new waveform
    if (lfoToSet != currentLfoWaveform) {
      // update smoother to current lfo value
      lfoSmoother.setCurrentAndTargetValue(getNextLfoValue());

      // set new waveform
      currentLfoWaveform = lfoToSet;

      // update smoother to new lfo value
      lfoSmoother.setTargetValue(getNextLfoValue());
    }
  }

  float getNextLfoValue() {
    if (lfoSmoother.isSmoothing()) {
      return lfoSmoother.getNextValue();
    }
    return lfos[juce::toUnderlyingType(currentLfoWaveform)].processSample(0.f);
  }
};  // namespace tremolo
}  // namespace tremolo