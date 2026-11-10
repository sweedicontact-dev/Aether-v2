# AETHER — Ambient Texture Engine (VST3)

Procesador de texturas para Ambient, Drone, Cinematic y Soundscapes. Efecto estéreo (se pone en una pista de Ableton, Reaper, FL Studio, Cubase, etc.).

## Qué hace

| Sección | Controles |
|---|---|
| **Espacio** | Size, Decay (0,3 – 60 s), Damping, Pre-Delay |
| **Movimiento** | Mod Depth, Mod Rate (el tanque "respira"), Tape (wow/flutter + saturación + oscurece) |
| **Shimmer** | Cantidad + intervalo (+12, +7, −12, +19) |
| **Tono / salida** | Low Cut, High Cut, Width, Mix, Output |
| **Freeze** | Congela la cola de reverb para hacer drones infinitos |

Motor: reverb FDN de 8 líneas (matriz Hadamard) con líneas moduladas, difusión de entrada, pitch shifter granular en el lazo de feedback, etapa de cinta y filtros de 12 dB/oct.

## 15 presets de fábrica

Catedral Infinita, Shimmer Celestial, Quintas del Cielo, Abismo Oscuro, Cinta Vieja, Drone Congelado, Aurora Boreal, Pad Etéreo, Cuarto Oscuro, Sueño Lento, Piano en la Niebla, Cristal Metálico, Océano Profundo, Mínimo Espacio, Init. Se cambian desde el menú del plugin (o con `<` `>`), y también aparecen como "programas" en el host.

## Compilar

### Opción A (la más fácil): GitHub Actions
1. Creá un repo en GitHub y subí esta carpeta.
2. Pestaña **Actions** → corre solo (o "Run workflow").
3. Cuando termine, bajá el artefacto `Aether-VST3-Windows` o `Aether-VST3-macOS`.

### Opción B: local
Requisitos: CMake ≥ 3.22, Git y un compilador (Visual Studio 2022 en Windows / Xcode en Mac).

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```
(la primera vez descarga JUCE 8.0.4 automáticamente). El resultado queda en `build/Aether_artefacts/Release/VST3/Aether.vst3`.

## Instalar
- **Windows:** copiá `Aether.vst3` a `C:\Program Files\Common Files\VST3\`
- **macOS:** copiá a `/Library/Audio/Plug-Ins/VST3/` y, si lo bajaste de internet, ejecutá `xattr -cr Aether.vst3` y `codesign --force --deep -s - Aether.vst3`

En **Ableton Live**: Preferences → Plug-ins → activá "Use VST3 Plug-in System Folders" → Rescan.

## Personalizar
- Nombre de empresa/códigos: `CMakeLists.txt` (`COMPANY_NAME`, `PLUGIN_MANUFACTURER_CODE`, `PLUGIN_CODE`).
- Presets: `Source/Presets.h` (cada preset es una lista corta de `{ "parametro", valor }`).
- DSP: `Source/Dsp.h` (sin dependencias de JUCE).
