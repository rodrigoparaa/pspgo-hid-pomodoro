# 🎮 PSP Go Modern Lab: USB HID Gamepad & Desk Clock Pomodoro

<div align="center">

**[🇬🇧 Read in English](README.md)** • [🇪🇸 Leer en Español](README_ES.md)

![Platform](https://img.shields.io/badge/Platform-Sony_PSP_Go_(N1000)-003791?style=for-the-badge&logo=playstation&logoColor=white)
![Host](https://img.shields.io/badge/Host-macOS_Apple_Silicon_(M1/M2/M3)-000000?style=for-the-badge&logo=apple&logoColor=white)
![Toolchain](https://img.shields.io/badge/Toolchain-GCC_15.2.0_MIPS-E65B00?style=for-the-badge&logo=gnu&logoColor=white)
![Language](https://img.shields.io/badge/Language-Pure_C99-00599C?style=for-the-badge&logo=c&logoColor=white)
![Framerate](https://img.shields.io/badge/Graphics-60_FPS_GU_Double--Buffer-2EA44F?style=for-the-badge)
![License](https://img.shields.io/badge/License-MIT-blue?style=for-the-badge)

<p align="center">
  <b>A comprehensive software engineering, hardware exploitation, and native development laboratory for the PlayStation Portable Go (PSP-N1000) on macOS Apple Silicon.</b>
</p>

</div>

---

## 📌 Project Overview

This repository documents and implements two complementary technical branches for the **Sony PSP Go**:

1. **Low-Latency USB HID Gamepad for macOS:** Plug-and-Play physical controller configuration for macOS, resolving compatibility hurdles with modern game engines (such as Unity in *Cult of the Lamb*) via Steam Input mediation.
2. **Native Desk Productivity Suite (Desk Clock & Pomodoro):** Pure C application compiled for MIPS architecture that turns the folded slider console into a minimalist desk clock and Pomodoro productivity timer, operated exclusively through the top physical **L** and **R** triggers with hardware double-buffering at a locked 60 FPS.
3. **Isolated Development Environment (`venv` style):** Local macOS toolchain environment enabling modern MIPS compilation (GCC 15.2.0) without polluting global system directories or conflicting with host libraries.

---

## 🏗️ Repository Architecture

```text
.
├── activate.sh                     # Virtual environment activation script (source activate.sh)
├── .gitignore                      # Version control exclusion rules
├── projects/
│   ├── 00_HelloWorld/              # Compiler validation and pad input testbed
│   ├── 00_Probe/                   # Hardware telemetry suite (Phase 0 hardware diagnostic)
│   └── 01_DeskClock_Pomodoro/      # Primary productivity and desk application
│       ├── core/                   # Decoupled pure C logic (Pomodoro, gestures, router)
│       ├── ui/                     # GU graphics pipeline, bitmap fonts, and screens
│       ├── platform/               # Synthesized procedural audio and battery telemetry
│       ├── tests/                  # Unit test suite executable natively on host with clang
│       ├── Makefile                # MIPS build rules and EBOOT.PBP packaging
│       ├── DESIGN.md               # Detailed functional specification
│       ├── INFORME_TECNICO.md      # Low-level engineering report and architecture
│       └── EBOOT.PBP               # Production-ready homebrew binary
├── README_ES.md                    # Spanish Documentation (Español)
└── README.md                       # Main Documentation (English)
```

---

## 🕹️ Branch 1: PSP Go as a macOS USB Gamepad

The PSP Go features USB 2.0 connectivity and an ultra-compact slider form factor. Leveraging the **FuSa GamePad v0.3** driver, the handheld enumerates as a standard USB HID input peripheral.

```mermaid
flowchart LR
    A["PSP Go (ef0:)"] -- "USB HID Driver (FuSa)" --> B["macOS Host (Apple Silicon)"]
    B -- "HID Raw Input" --> C["Steam Input Layer"]
    C -- "Virtual Gamepad" --> D["Unity / Native Games\n(e.g. Cult of the Lamb)"]
```

### Connection Protocol
1. **Startup:** Launch `FuSa GamePad` from the console's game menu **before** connecting the USB cable to avoid triggering USB Mass Storage mode.
2. **Link:** Connect the USB cable to your Mac. The handheld screen will reflect `:: CONNECTED ::`.
3. **Clean Disconnect:** Disconnect the USB cable, press the physical **`PS / HOME`** button, and confirm with **`X`** to safely return to the XMB.

### Compatibility with macOS & Modern Game Engines (Unity)
Modern macOS games relying on Apple's `GameController` framework often ignore generic, uncertified HID controllers. To resolve this:
* Enable **Steam Input for generic controllers** in Steam's global controller settings.
* Force **Enable Steam Input** in game properties.
* Provides immediate support for the analog thumbstick, D-pad, and action buttons with negligible input latency.

---

## ⏱️ Branch 2: Desk Clock & Pomodoro (`01_DeskClock_Pomodoro`)

A dedicated desk companion engineered around the unique mechanical design of the PSP Go. When the handheld rests folded on a desk or docking station, the top shoulder triggers (**L** and **R**) remain exposed and face upwards, enabling effortless one-hand Pomodoro operation without ever sliding open the screen.

```text
┌─────────────────────────────────────────────────────────────┐
│  DESK CLOCK  •  PSP GO                              90% [■] │  <- Subtle header bar
├─────────────────────────────────────────────────────────────┤
│                                                             │
│                    OCTOBER 15, 2026                         │
│                                                             │
│                    2 0 : 4 5 : 1 2                          │  <- Solid 7-segment digital time
│                                                             │
│       ┌─────────────────────────────────────────────┐       │
│       │  POMODORO IN PROGRESS    24:18    Active    │       │  <- Live status card
│       │  [■■■■■■■■■■■□□□□□□□□□□□□□□□□□□□□□□□□□□□□□]  │       │
│       └─────────────────────────────────────────────┘       │
│                                                             │
├─────────────────────────────────────────────────────────────┤
│  [L+R] Cycle Screen                        PSP Go Desk App  │  <- Bottom navigation bar
└─────────────────────────────────────────────────────────────┘
```

### Key Technical Features
* **Official Hardware Double-Buffering at 60 FPS (`libpspgu`):** Initializes Sony's Graphic Engine coprocessor (`sceGuInit`). Display and draw buffers (512×272 @ 32-bit RGBA) are swapped using `sceGuSwapBuffers()` strictly synchronized with the vertical refresh scanline (`sceDisplayWaitVblankStart`). Zero tearing, zero visual stutter, and zero frame drops.
* **Continuous Solid Digital Typography:** Segment joints interconnect seamlessly without gaps or broken pixels, maximizing clarity on the handheld's LCD panel.
* **Drift-Free Hardware RTC Engine:** Elapsed time is calculated using deltas from the hardware Real-Time Clock tick counter (`sceRtcGetCurrentTick`) in milliseconds, ensuring the timer remains mathematically precise regardless of loop execution time.
* **Procedural Synthesized Audio Chimes:** Real-time harmonic dual-tone chime generated purely via algorithmic sine wave synthesis through `libpspaudio`—zero external WAV/MP3 asset overhead.
* **Battery Telemetry & Hardware Filtering:** Direct querying of power subsystems via `libpsppower` displaying battery percentage, exact millivolt readings, and charging state, while sanitizing unsupported temperature registers on the PSP Go architecture.
* **System Keep-Awake:** Periodic background calls to `scePowerTick()` prevent the official firmware from dimming the backlight or entering standby while on desk display duty.

---

## 🎮 Control Scheme

The application includes a built-in **Quick Controls Guide (Cheatsheet)** accessible at any time by pressing **`SELECT`**.

| Action | Handheld Closed (Slider Folded) | Handheld Open (Expanded) |
| :--- | :--- | :--- |
| **Play / Pause Pomodoro** | Short press on **`L Trigger`** | **`L Trigger`** or **`Cross (X)`** |
| **Skip Phase (Work/Break)** | Short press on **`R Trigger`** | **`R Trigger`** |
| **Reset Pomodoro Cycle** | Hold **`R Trigger`** for 1.5 s | Hold **`R Trigger`** for 1.5 s |
| **Cycle Active Screen** | Hold **`L + R`** for 1.0 s | Hold **`L + R`** or **`D-Pad ◄ / ►`** |
| **Toggle Controls Cheatsheet** | — | **`SELECT`** button |
| **Return to Main Clock** | — | **`Circle (O)`** button |
| **Exit to PSP XMB Menu** | **`PS / HOME`** button | **`PS / HOME`** button |

> **Engineering Note:** The input engine implements software cancellation logic: after triggering the sustained `L + R` screen cycle combo, releasing the buttons inhibits false individual taps.

---

## 💻 Development & Build Environment

To eliminate dependency conflicts on macOS (Apple Silicon ARM64), the development workspace operates similarly to a Python virtual environment (`venv`).

### Host Prerequisites
* macOS 13+ on Apple Silicon (M1/M2/M3/M4).
* Homebrew package manager with auxiliary compiler math libraries:
  ```bash
  brew install libmpc mpfr gmp
  ```

### Activating the Environment
From the repository root:
```bash
# Activate the isolated PSPDEV environment
source activate.sh

# Terminal prompt will transform to:
# (psp-env) user@host:MandoPSP $
```

### Build & Verification

1. **Run Unit Tests on macOS Host:**
   Core logic (`core/`) contains zero PSP SDK dependencies and compiles natively with `clang` for rapid unit testing:
   ```bash
   cd projects/01_DeskClock_Pomodoro/tests
   clang -Wall -Wextra -O2 -I../core ../core/*.c test_core.c -o run_tests && ./run_tests
   ```

2. **Compile the Final PSP Go Executable:**
   ```bash
   cd projects/01_DeskClock_Pomodoro
   make clean && make
   ```
   *Generates the production-packaged `EBOOT.PBP` binary ready for hardware deployment.*

3. **Deactivate the Environment:**
   ```bash
   deactivate_psp
   ```

---

## 🚀 Deployment to PSP Go Hardware

1. Connect the PSP Go to your Mac via USB in *USB Connection* mode.
2. Create the destination directory in the internal flash storage (`ef0:`):
   ```bash
   mkdir -p "/Volumes/PSP GO/PSP/GAME/DeskClock"
   cp projects/01_DeskClock_Pomodoro/EBOOT.PBP "/Volumes/PSP GO/PSP/GAME/DeskClock/"
   ```
3. Purge macOS filesystem metadata (`._*` and `.DS_Store`) to avoid "Corrupted Data" icons on the XMB:
   ```bash
   dot_clean "/Volumes/PSP GO/PSP/GAME"
   sync
   ```
4. Safely eject the volume in Finder and unplug the cable. The app will be listed under **Game ➔ System Storage**.

---

## 📄 License & Credits

* **License:** MIT License. Open source for the retro gaming and homebrew development community.
* **Toolchain:** Official community preservation project [PSPDEV](https://github.com/pspdev).
* **USB Driver:** FuSa GamePad by Andy_maN.
