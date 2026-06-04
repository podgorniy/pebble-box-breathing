# Box Breathing Pebble Watchapp

## Overview
A native C watchapp for Pebble/Rebble smartwatches that guides users through a 16-second box breathing exercise.

## Key Features
- **Breathing Animation**: A linearly animated, continuous lung-volume bar centered on the screen that visually represents the breathing phases. High contrast text labels ("In", "Out", "Hold") indicate the current phase, with text color dynamically adapting to the fill state for readability. Updates every 100ms.
- **Vibration Modes**: Configurable haptic feedback to guide breathing without looking at the screen. The user's selection is saved across sessions.
  - **Every Second**: Short pulses (50ms) every second, long pulses (250ms) on phase boundaries.
  - **Phase Only**: Short pulses (50ms) on phase boundaries only.
  - **Off**: No vibrations.
- **Always-On Backlight**: Can be manually toggled to keep the screen on, saving the preference across sessions.
- **Controls**:
  - **Top Button**: Pause/Resume the breathing cycle (short press), or Reset the session (long press).
  - **Middle Button**: Cycle through the vibration modes.
  - **Bottom Button**: Toggle the always-on backlight.
- **UI Design**: A minimal, high-contrast layout driven by a vertical layout manager. Includes a 24-hour clock, session duration timer, and native vector-drawn state indicators for vibration mode, pause state, and backlight status. Supports color (Pebble Time series) and monochrome (Pebble Classic, Pebble 2) displays via fallback colors.
- **Compatibility**: Targets `aplite`, `basalt`, `chalk`, `diorite`, and `emery` platforms.

## Core Project Structure
- `src/c/main.c`: Application lifecycle, settings persistence, button subscriptions, and main 1-second/100ms tick loops.
- `src/c/app_state.h`: Global application state definition (timers, phase, mode).
- `src/c/breathing.c`: Breathing cycle timing, phase calculation, and linear animation fill math.
- `src/c/ui.c`: UI layer management, integrating the vertical layout, custom drawing routines for the breathing bar with text rendering, and native geometric icon drawing.
- `src/c/layout.c` & `src/c/layout.h`: A simple layout library for managing proportional screen space without fixed coordinates.
- `src/c/vibration.c`: Haptic pattern definitions and vibration triggering logic based on the breathing cycle.