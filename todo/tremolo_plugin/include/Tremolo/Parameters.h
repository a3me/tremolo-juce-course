#pragma once

namespace tremolo {
struct Parameters {
  explicit Parameters(juce::AudioProcessor&);

  juce::AudioParameterFloat& rate;
};
}  // namespace tremolo
