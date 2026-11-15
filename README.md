# DU - Butt-Kicker

An independent VST3 for tightening and shaping low frequencies. It is a new implementation, with its own plugin ID and no dependency on the supplied commercial VST3.

## Background research

[Streaky Audio's product page](https://streakyaudio.com/products/tight-arse) describes the job as controlling excess bass, choosing between cleaner and more colored low-end enhancement, restoring punch, compensating level as bass control changes, and matching output level for a fair comparison. The ten factory presets extracted from the local binary also show controls for crossover, release, input/output gain, gain matching, and oversampling. The decompiled runtime image shows a low-band crossover, sample-level envelope tracking, gain calculation, filtering, and output mixing. These observations inform this plugin's workflow; its DSP code is newly written.

[JUCE's Linkwitz-Riley filter](https://docs.juce.com/develop/classjuce_1_1dsp_1_1LinkwitzRileyFilter.html) provides a fourth-order low/high split with a flat summed magnitude response. [JUCE's AudioProcessor](https://docs.juce.com/master/classjuce_1_1AudioProcessor.html) defines the host's block-processing interface, and [AudioProcessorValueTreeState](https://docs.juce.com/master/classjuce_1_1AudioProcessorValueTreeState.html) stores automatable parameters and preset state.

## Processing

Input gain → stereo-linked 120 Hz default crossover → low-band RMS compression and transient lift → clean sub-rumble trim or mild color shaping → high-band recombination → wet/dry mix → optional long-term gain match → output gain.

The dry side of the Mix control uses the same crossover's recombined low/high signal, keeping its phase aligned with the processed side. Bypass passes the raw input. The Clean/Color mode blends over 30 ms to avoid an abrupt switch. No oversampling or lookahead is used, so the plugin reports zero latency. The Color mode is intentionally mild; high drive settings can still produce some aliasing.

## Controls

| Control | Purpose |
| --- | --- |
| Snugness | Sets low-band compression threshold and ratio together. |
| Thump Force | Adds short transient lift to the low band. |
| Bass Boundary | Sets the boundary between processed lows and untouched highs. |
| Release ms | Controls low-band compressor recovery. |
| Magic Sauce | Amount of sub-rumble trim in Clean, or soft color in Color. |
| Magic Mode | Clean or Color. |
| Automatic Thumper | Applies a small makeup gain as Snugness increases. |
| Gain Match | Slowly moves toward the dry RMS level, with upward correction limited to about 1 dB. Its switch fades over 250 ms. |
| Input dB / Output dB | Gain before and after processing. |
| Mix | Blends aligned dry and processed signals. |
| Bypass | Passes the original input. |

The plugin window has a preset selector with seven factory starting points: Punchy Start, Gentle Glue, Kick Focus, Bass Tamer, Sub Clean, Low Heat, and Mix Bus. These presets are compiled into the plugin and are also exposed through the host program interface. Punchy Start is the default. Existing DAW sessions may restore their saved parameter values; select Punchy Start in the plugin to hear the default settings.

To save your own preset, type a name in the field above the controls and click **Save Preset**. The plugin saves a `.dubk` file in your Documents folder under `DU - Butt-Kicker/Presets`. User presets appear in the same selector with a `User:` prefix. Saving the same name again creates a numbered copy, preserving the earlier file. The host can also save the plugin state as part of a project.

## Build

The project uses the JUCE source already present in the sibling `My-808` project. To use another JUCE checkout, pass `-DJUCE_DIR=<path>`.

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target LowEndPilot_VST3
cmake --build build --config Release --target LowEndPilot_Standalone LowEndPilotSmoke
./build/Release/LowEndPilotSmoke.exe
```

The VST3 bundle is `build/LowEndPilot_artefacts/Release/VST3/DU - Butt-Kicker.vst3`. The standalone app is `build/LowEndPilot_artefacts/Release/Standalone/DU - Butt-Kicker.exe`. The former `Low End Pilot.vst3` bundle was moved to `previous-build/` so the DAW scan folder contains one plugin with this ID. The custom editor embeds `assets/du-background.png` and displays it behind the controls. Parameter IDs remain unchanged so existing projects and presets can restore their settings.

The smoke test passed at 48 kHz: an 80 Hz tone fell from 0.283 to 0.044 RMS as Tight went from 0 to 100, while a 2 kHz tone remained at 0.283 RMS. Punchy Start measured 0.068 RMS at 80 Hz. Gain Match at maximum Tight raised the low-band RMS from 0.044 to 0.049, within the 1 dB limit. It also checked raw bypass, the preset selector, parameter/program-state restoration, and saving and loading a user preset file. A fresh DAW listening test of this revision has not yet been performed.

## macOS VST3 build

On a Mac with Xcode and CMake installed, extract `dist/DU-Butt-Kicker-macOS-source.zip` and run `sh build-macos.sh` from its `DU-Butt-Kicker` folder. The script builds a universal Intel and Apple Silicon VST3, runs the smoke test, and archives the bundle as `dist/DU - Butt-Kicker-macOS-universal.vst3.zip`. The package includes JUCE 8.0.12 source. The resulting plugin is unsigned. To install it locally, extract the VST3 archive and put the `.vst3` bundle in `~/Library/Audio/Plug-Ins/VST3/`, then rescan plugins in the DAW.

Without a Mac, extract that source ZIP on Windows, create a new GitHub repository from the extracted `DU-Butt-Kicker` folder, and run the included **Build macOS VST3** workflow from the repository's Actions tab. Download the `DU-Butt-Kicker-macOS-universal-VST3` artifact after it succeeds. The workflow runs the same build script on a GitHub-hosted Mac.
