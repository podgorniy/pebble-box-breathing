# Box Breathing Pebble Watchapp

## Overview
A native C watchapp for Pebble/Rebble smartwatches that guides users through a 16-second box breathing exercise while simultaneously monitoring and graphing their heart rate.

## Key Features
- **Breathing Animation**: A smooth, animated 4-section lung-volume bar that visually represents the breathing phases (Inhale, Hold Full, Exhale, Hold Empty). The animation utilizes a `smoothstep` easing function and updates every 100ms.
- **Heart Rate Integration**: Connects to the Pebble `HealthService` to display the current heart rate. It also renders a scrolling history graph at the bottom of the screen (1 pixel width per second, vertically scaled dynamically between 40 and 160 BPM).
- **Vibration Modes**: Configurable haptic feedback to guide breathing without looking at the screen:
  - **Every Second**: Short pulses (50ms) every second, long pulses (250ms) on phase boundaries.
  - **Phase Only**: Long pulses (250ms) on phase boundaries only.
  - **Off**: No vibrations.
- **Controls**:
  - **Top Button**: Pause/Resume the breathing cycle, animation, timer, and heart rate graph.
  - **Middle Button**: Cycle through the vibration modes.
  - **Bottom Button**: Reset the session timer, cycle phase, and clear the heart rate graph.
- **UI Design**: A minimal, high-contrast layout. Includes a 24-hour clock, session duration timer, numeric BPM display, and compact unicode state indicators for vibration mode and pause state. Supports color (Pebble Time series) and monochrome (Pebble Classic, Pebble 2) displays via fallback colors.
- **Compatibility**: Targets `aplite`, `basalt`, `chalk`, `diorite`, and `emery` platforms.

## Core Project Structure
- `src/c/main.c`: Application lifecycle, button subscriptions, and main 1-second/100ms tick loops.
- `src/c/app_state.h`: Global application state definition (timers, phase, BPM history buffer, mode).
- `src/c/breathing.c`: Breathing cycle timing, phase calculation, and animation fill math.
- `src/c/ui.c`: UI layer management, custom drawing routines for the breathing bar and text rendering.
- `src/c/graph.c`: Custom drawing logic for the scrolling heart rate history graph.
- `src/c/heart_rate.c`: Pebble HealthService subscription and buffering BPM data.
- `src/c/vibration.c`: Haptic pattern definitions and vibration triggering logic based on the breathing cycle.
