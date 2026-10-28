#pragma once

namespace CrushJoints
{
    /**
        Knob law of the original CamelCrusher cutoff control.

        The original maps its normalised cutoff knob (0..1) to 80 Hz .. 16 kHz
        with a custom curve that is not exponential (see docs/MEASUREMENTS.md).
        The curve was read from the plugin's own parameter display at 1001 knob
        positions (1 Hz resolution) and is interpolated linearly. The measured
        filter response confirms that the displayed Hz is the real cutoff.
    */
    namespace CutoffCurve
    {
        constexpr float kMinHz = 80.0f;
        constexpr float kMaxHz = 16000.0f;

        // Normalised knob (0..1) to Hz.
        float toHz(float normalised) noexcept;

        // Hz to normalised knob (0..1); inverse of toHz, clamped to the range.
        float toNormalised(float hz) noexcept;
    }
}
