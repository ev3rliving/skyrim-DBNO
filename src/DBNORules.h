#pragma once
#include <cmath>
#include <string_view>

namespace DBNORules
{
    inline float FilterHealthRegen(float value, bool enabled, bool downed,
        bool player, bool loading, bool healthRegenValue)
    {
        return enabled && downed && player && !loading && healthRegenValue ? 0.0f : value;
    }

    inline float DownedHealthRegenDelay(float existing, float delta)
    {
        // Leave a full second after this update, including unusually long frames.
        const float minimum = (std::isfinite(delta) && delta > 0.0f ? delta : 0.0f) + 1.0f;
        return std::isfinite(existing) && existing > minimum ? existing : minimum;
    }

    inline bool InstantRagdollOnDeath(bool original, bool enabled, bool option, bool downed, bool player, bool killmove = false)
    {
        return original || (enabled && option && downed && player && !killmove);
    }

    inline bool CanControlAnimation(bool deadOrDying, bool killmove, bool deathTransition, float health)
    {
        return !deadOrDying && !killmove && !deathTransition && std::isfinite(health) && health > 0.0f;
    }

    inline bool ShouldSuppressStagger(bool enabled, bool option, bool downed, bool player, std::string_view event)
    {
        if (!enabled || !option || !downed || !player) return false;
        constexpr std::string_view start = "staggerstart";
        if (event.size() != start.size()) return false;
        for (std::size_t i = 0; i < start.size(); ++i) {
            char c = event[i];
            if (c >= 'A' && c <= 'Z') c = static_cast<char>(c + ('a' - 'A'));
            if (c != start[i]) return false;
        }
        return true;
    }

    inline bool ShouldEnter(float before, float after, float maximum, float threshold)
    {
        return std::isfinite(before) && std::isfinite(after) && std::isfinite(maximum) &&
            std::isfinite(threshold) && maximum > 0.0f && after > 0.0f && after < before &&
            after <= maximum * threshold / 100.0f;
    }
    inline float ClampSetting(float value, float low, float high, float fallback)
    {
        if (!std::isfinite(value)) return fallback;
        return value < low ? low : value > high ? high : value;
    }

    // Keep a baseline across engine updates, independent of damage callback timing.
    struct HealthObserver
    {
        float previous{ 0.0f };
        bool valid{ false };
        void Reset() { valid = false; }
        bool Sample(float health, float maximum, float threshold)
        {
            const bool enter = valid && ShouldEnter(previous, health, maximum, threshold);
            previous = health;
            valid = std::isfinite(health) && health > 0.0f;
            return enter;
        }
    };
}
