#pragma once
#include <utility>
#include <vector>

// Cada preset solo lista los parametros que cambian; el resto vuelve a su valor por defecto.
// IDs: mix, size, decay (s), damp (Hz), predelay (ms), shimmer, shimint (0:+12 1:+7 2:-12 3:+19),
//      moddepth, modrate (Hz), tape, lowcut (Hz), highcut (Hz), width, out (dB), freeze (0/1)
struct Preset
{
    const char* name;
    std::vector<std::pair<const char*, float>> values;
};

inline const std::vector<Preset>& getPresets()
{
    static const std::vector<Preset> presets = {
        { "01 Init - Sala Suave", {
            { "mix", 0.30f }, { "size", 0.50f }, { "decay", 3.5f }, { "damp", 7000.0f }, { "predelay", 15.0f } } },

        { "02 Catedral Infinita", {
            { "mix", 0.45f }, { "size", 0.90f }, { "decay", 28.0f }, { "damp", 5000.0f }, { "predelay", 40.0f },
            { "moddepth", 0.30f }, { "modrate", 0.25f }, { "width", 1.5f }, { "lowcut", 100.0f } } },

        { "03 Shimmer Celestial", {
            { "mix", 0.50f }, { "size", 0.75f }, { "decay", 14.0f }, { "damp", 9000.0f }, { "predelay", 25.0f },
            { "shimmer", 0.65f }, { "shimint", 0.0f }, { "moddepth", 0.45f }, { "modrate", 0.30f },
            { "width", 1.6f }, { "highcut", 16000.0f }, { "lowcut", 150.0f } } },

        { "04 Quintas del Cielo", {
            { "mix", 0.50f }, { "size", 0.80f }, { "decay", 16.0f }, { "damp", 8000.0f }, { "predelay", 30.0f },
            { "shimmer", 0.55f }, { "shimint", 1.0f }, { "moddepth", 0.40f }, { "modrate", 0.20f },
            { "width", 1.7f }, { "lowcut", 120.0f } } },

        { "05 Abismo Oscuro", {
            { "mix", 0.55f }, { "size", 0.85f }, { "decay", 24.0f }, { "damp", 2500.0f }, { "predelay", 20.0f },
            { "shimmer", 0.60f }, { "shimint", 2.0f }, { "moddepth", 0.35f }, { "modrate", 0.15f },
            { "tape", 0.20f }, { "lowcut", 60.0f }, { "highcut", 7000.0f }, { "width", 1.4f } } },

        { "06 Cinta Vieja", {
            { "mix", 0.40f }, { "size", 0.50f }, { "decay", 5.0f }, { "damp", 4500.0f }, { "predelay", 10.0f },
            { "moddepth", 0.50f }, { "modrate", 0.30f }, { "tape", 0.80f }, { "highcut", 8000.0f },
            { "lowcut", 90.0f } } },

        { "07 Drone Congelado", {
            { "mix", 0.70f }, { "size", 0.80f }, { "decay", 20.0f }, { "damp", 6000.0f }, { "predelay", 0.0f },
            { "moddepth", 0.50f }, { "modrate", 0.12f }, { "tape", 0.25f }, { "width", 1.8f },
            { "freeze", 1.0f } } },

        { "08 Aurora Boreal", {
            { "mix", 0.50f }, { "size", 0.80f }, { "decay", 18.0f }, { "damp", 10000.0f }, { "predelay", 35.0f },
            { "shimmer", 0.50f }, { "shimint", 3.0f }, { "moddepth", 0.60f }, { "modrate", 0.15f },
            { "width", 1.9f }, { "highcut", 17000.0f }, { "lowcut", 180.0f } } },

        { "09 Pad Eterico", {
            { "mix", 0.40f }, { "size", 0.65f }, { "decay", 9.0f }, { "damp", 7000.0f }, { "predelay", 15.0f },
            { "moddepth", 0.35f }, { "modrate", 0.35f }, { "width", 1.4f }, { "lowcut", 150.0f } } },

        { "10 Cuarto Oscuro", {
            { "mix", 0.30f }, { "size", 0.25f }, { "decay", 1.8f }, { "damp", 3000.0f }, { "predelay", 5.0f },
            { "moddepth", 0.15f }, { "width", 1.0f }, { "lowcut", 120.0f }, { "highcut", 9000.0f } } },

        { "11 Sueno Lento", {
            { "mix", 0.60f }, { "size", 1.00f }, { "decay", 22.0f }, { "damp", 4000.0f }, { "predelay", 50.0f },
            { "moddepth", 0.80f }, { "modrate", 0.08f }, { "tape", 0.35f }, { "width", 1.6f },
            { "highcut", 10000.0f } } },

        { "12 Piano en la Niebla", {
            { "mix", 0.35f }, { "size", 0.70f }, { "decay", 7.0f }, { "damp", 6500.0f }, { "predelay", 30.0f },
            { "shimmer", 0.20f }, { "shimint", 0.0f }, { "moddepth", 0.30f }, { "tape", 0.15f },
            { "lowcut", 120.0f }, { "highcut", 11000.0f }, { "width", 1.3f } } },

        { "13 Cristal Metalico", {
            { "mix", 0.45f }, { "size", 0.45f }, { "decay", 10.0f }, { "damp", 16000.0f }, { "predelay", 5.0f },
            { "shimmer", 0.50f }, { "shimint", 3.0f }, { "moddepth", 0.90f }, { "modrate", 1.5f },
            { "highcut", 19000.0f }, { "lowcut", 250.0f }, { "width", 1.5f } } },

        { "14 Oceano Profundo", {
            { "mix", 0.60f }, { "size", 1.00f }, { "decay", 40.0f }, { "damp", 3000.0f }, { "predelay", 60.0f },
            { "shimmer", 0.30f }, { "shimint", 2.0f }, { "moddepth", 0.50f }, { "modrate", 0.10f },
            { "width", 2.0f }, { "lowcut", 40.0f }, { "highcut", 8000.0f } } },

        { "15 Minimo Espacio", {
            { "mix", 0.20f }, { "size", 0.35f }, { "decay", 2.0f }, { "damp", 8000.0f }, { "predelay", 8.0f },
            { "moddepth", 0.20f }, { "width", 1.1f } } },
    };
    return presets;
}
