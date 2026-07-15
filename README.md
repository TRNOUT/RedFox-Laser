# RedFox-Laser

An open-source laser show control application for Windows — a live-performance
tool and editor for driving ILDA laser projectors.

> **Status:** early development. The engine foundation is in place and tested;
> the user interface, vector editor, and hardware output backends are in
> progress.

## Design

RedFox-Laser is split into two processes for safety and responsiveness:

- **Engine** (`redfox_engine`) — a headless, high-priority process that owns the
  safety-critical logic: an arm/blank/emergency-stop state machine with a
  heartbeat and frame-stall watchdog, a rotating safety-event log, and the laser
  output backends. If the UI stops sending its heartbeat (crash or hang), the
  engine blanks all outputs automatically.
- **UI** (planned) — the editor, cue grid / timeline, and status panel, connected
  to the engine over local IPC (shared-memory telemetry + a named-pipe command
  channel, native Win32, no third-party IPC dependency).

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

## Running

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

## Tests

```
ctest --test-dir build
```

## License

MIT — see [LICENSE](LICENSE).
