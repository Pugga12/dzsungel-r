# Introduction
Dzsungel is a cpp synthesizer project. It supports playing midi files, either rendering them into a .wav file, or outputting them live over speakers

# Usage
`dzsmf SUBCOMMAND INFILE OPTIONS`

- `SUBCOMMAND`: 
    - `play`: Plays the midi file to the default audio device using the computer's audio server, or if specified with `-d`, another device
    - `render`: Renders to a wave file offline. This is the de-facto reference for what the audio should sound like, as it touches no external components like the sound server that can affect the output, has no multithreading or synchronization, etc. Everything runs in the main thread.
    - Use `-h/--help` or `-H/--help-all` to see options. Specify a subcommand to filter for that option
- `INFILE`: /path/to/file.mid

## Subcommand `play`
- `-d/--device`: Specify a numerical device ID for playback. Detects the default device from your audio server by default and uses that if not set 
    - `-L/--list-devices`: Lists connected audio devices. You should get something like this
        ```
            Available output devices: 
            (129) 2 ch out: Speakers (Qualcomm(R) Aqstic(TM) Audio Adapter Device) (default)
        ```
    Note the device id in parenthesis, and that `(default)` will be placed on the end of the line for the default device

## Subcommand `render`
Use `-o/--output` to set the output file. Uses `out.wav` by default. Will overwrite

# Project Structure
- `/apps`: Contains main files. Currently there is only one, `SmfApp.cpp`. These all link against the core library, `dzsungel-core`
- `/bin`: Binary directory. May contain subdirectories for different build types depending on platform (e.g. `Release`, `Debug` for MSVC)
- `/include/dzsungel`: Project headers. Most of these have an implementation file in `/src`, so any complicated functions as well as helpers should go in there
    - `/include/dzsungel/core`: Core synth functionality. ADSR, oscillators, state, etc.
    - `/include/dzsungel/io/Wav.hpp`: Header for `dr_wav` implementation
    - `/include/dzsungel/io/Smf.hpp`: `IOSmf` class contains everything related to reading standard midi files and preparing them in a way the synth can understand
    - `/include/dzsungel/resources`: Management for patches and Wavetables
    - Top level:
        - `Constants.hpp`: Numerical constants
        - `RampingValue.hpp`: Manages numerical values that ramp up over time
        - `Types.hpp`: Core struct types.
- `libs`: Small libraries, things that can't be installed with `vcpkg` or *nix package managers
    - `dr_wav`: Header only library for wav reading and writing. Implemented at `src/io/Wav.cpp`
    - `midifile`: Handles reading smf files
- `/src`: Library source files
- `/tests`: Unit tests
- `CMakeLists.txt`: Top-level cmake lists
- `CMakePresets.json`: Build presets for Windows and Linux
- `vcpkg.json`: Vcpkg manifest file. Used on windows only with MSVC

# Setup
## Windows
- Make sure you are running in a Developer powershell environment with Windows SDK stuff on the path (e.g. msvc)
- Simply run `cmake --preset msbuild-aarch64`. I don't have an x86 preset yet
- To build `cmake --build out/build/msbuild-aarch64`. Files will be in the `/bin` under `/Debug` by default.
## Linux
- Install development packages system package manager
    - `Boost`: At the very least, install `boost::lockfree`
    - `CLI11`
    - `RtAudio`
    - `spdlog`
    - `magic_enum`

These are **not exact names** and may be written differently based on the package manager, `dnf` vs `apt`, etc
- Simply run `cmake --preset clang-fedora-linux`
- To build `cmake --build out/build/clang-fedora-linux`.

# Unit Tests
Enable by cmake flag `DZSUNGEL_BUILD_TESTS`. Uses gtest

# Tokens
This project relies on many external libraries. While it may be your default to go spelunking through the vcpkg install in `/out` or system headers, don't. That bloats the context window and uses way too many tokens
- Try and use your web search tool to find any documentation (e.g. doxygen, api references) before searching for code
- Don't just use a recursive tree command. A vcpkg install has thousands of files, limit it to one or two levels or use the provided project structure

# Comments 
Write comments, but not overly specific. If there's some reasoning someone **needs** to absolutely understand, do that in a few words. Otherwise, just label sections.

# Code Style
I try and split things out into private or static functions when a function is doing too many things. See rtaudio stuff in `apps/SmfApp.cpp` or `src/core/AudioEngine.cpp` if you are working with those and need an example.

# Git
Short commit mesages. Check commit diffs if you want to see anything I've done historically