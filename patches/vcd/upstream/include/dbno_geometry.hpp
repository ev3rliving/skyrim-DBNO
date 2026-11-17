#pragma once

namespace DBNO {
    // Keep the bottom of the original bumper on the same plane when shortening
    // it. VCD normally centers presets, which would leave this capsule floating.
    constexpr float BottomEndpoint(float originalCenter, float originalHalfSegment,
        float originalRadius, float newRadius)
    {
        return originalCenter - originalHalfSegment - originalRadius + newRadius;
    }
}
