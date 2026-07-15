# RedFox-Laser

An open-source laser show control application for Windows — a live-performance
tool and editor for driving ILDA laser projectors.

> **Status:** early but functional. Working today: the safety engine with
> end-to-end cue playback, a shared core library (ILDA read/write, show data
> model, MIDI parsing/mapping, a binary show format), MIDI-controller input
> (libremidi), and a Qt UI with engine controls, a cue grid, and a vector
> editor. Real ShowNET hardware output is pending an SDK integration.

## Design

RedFox-Laser is split into two processes for safety and responsiveness:

- **Engine** (`redfox_engine`) — a headless, high-priority process that owns the
  safety-critical logic: an arm/blank/emergency-stop state machine with a
  heartbeat and frame-stall watchdog, a rotating safety-event log, and the laser
  output backends. If the UI stops sending its heartbeat (crash or hang), the
  engine blanks all outputs automatically.
- **UI** (`redfox_ui`, Qt) — connects to the engine over local IPC (shared-memory
  telemetry + a named-pipe command channel, native Win32, no third-party IPC
  dependency). The initial window shows the live safety state and offers
  Arm / Emergency-Stop / Clear controls; the editor, cue grid and timeline build
  on top of this. Optional at build time (only built when Qt6 is found).

The shared, hardware-independent `core` library holds the file formats (an ILDA
`.ild` reader/writer covering all standard formats), the show/cue data model,
and MIDI parsing + mapping.

Laser output goes through a small `LaserOutput` interface with pluggable
backends. Today there is a `MockLaserOutput` (for automated tests) and a backend
built on the open-source [libera-laser](https://github.com/sebleedelisle/libera-laser)
library (Ether Dream, Helios, IDN, LaserCube, …).

### Hardware support

- **IDN (ILDA Digital Network)** and other open DAC protocols via libera-laser.
- **Laserworld ShowNET** support is planned via Laserworld's official ShowNET
  API/SDK.

## Prerequisites

- Windows 10+, Visual Studio 2022 (or another CMake-compatible MSVC toolchain)
- CMake 3.15+
- Git (with submodule support)

## Building

```
git submodule update --init --recursive
cmake -S . -B build
cmake --build build
```

The Qt UI is optional. To include it, install Qt 6 and point CMake at it:

```
cmake -S . -B build -DCMAKE_PREFIX_PATH=C:/path/to/Qt/6.x.x/msvc2022_64
cmake --build build
```

## Running

The UI (start the engine too, see below):

```
build\ui\Debug\redfox_ui.exe
```

The engine (mock output):

```
build\engine\Debug\redfox_engine.exe
```

A console client that stands in for the UI (heartbeat + arm/estop/status/…):

```
build\tools\engine_console\Debug\engine_console.exe
```

Probe whether a laser DAC on your network speaks IDN (reads laser-safety notes in
the source before using the light-emitting option):

```
build\tools\idn_probe\Debug\idn_probe.exe
```

MIDI: if a MIDI input device is present, the engine opens it on startup and maps
pad notes 36–43 to cues 0–7 (note 44 arms, note 45 blackouts). The vector editor
opens from the UI's "Open Vector Editor" button.

## Tests

```
ctest --test-dir build
```

## License

MIT — see [LICENSE](LICENSE).
