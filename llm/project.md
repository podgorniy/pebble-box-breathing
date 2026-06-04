# Box Breathing — Pebble Watchapp

A native C watchapp for Pebble / Rebble smartwatches that facilitates **Box Breathing**: it guides the user through a 16-second four-Phase Cycle with synchronized Vibes, a live Cycle Counter against a Target Cycles goal, and a per-Cycle HR Graph.

---

## 1. Glossary

### 1.1 Terms (bare list, grouped)

**Breathing model**
Box Breathing · Cycle · Phase · Inhale Phase · Hold Full Phase · Exhale Phase · Hold Empty Phase · Phase Boundary · Cycle Wrap

**Session & counters**
Session · Session Elapsed · Completed Cycles · Cycle Counter · Target Cycles · Reset

**Pause / playback**
Pause State · Pause Indicator

**Breathing visual**
Breathing Circle · Filler Circle · Exhaled Reference Outline · Inhaled Reference Outline · Phase Fill

**Vibe (haptic feedback)**
Vibe Mode · Vibe Every Second · Vibe Phase Only · Vibe Off · Short Vibe · Long Vibe · Session-Complete Vibe

**Heart rate**
HR Sample · HR Sample Buffer · HR Write Index · HR Graph · HR Min Label · HR Max Label · Current HR

**Status Strip**
Status Strip · Vibe Indicator · Backlight Indicator · Pause Indicator

**Backlight**
Backlight Always-On

**Top container**
Clock · Session Elapsed Display · Cycle Counter Display · Current HR Display

**Engine / architecture**
Tick Handler · Animation Timer · Cycle Elapsed Sec · Anim Sub Ms · Vibe Mode Persist Key · Backlight Persist Key · Layout Slot

**Controls**
Top Button · Middle Button · Bottom Button

### 1.2 Terms with descriptions

**Box Breathing** — the 4-Phase, equal-duration breathing technique this app facilitates.
**Cycle** — one full 16-second Box Breathing iteration (4 Phases × 4 s). Unit counted by Cycle Counter.
**Phase** — one 4-second segment of a Cycle; exactly four exist (Inhale, Hold Full, Exhale, Hold Empty).
**Inhale Phase** — Cycle seconds 0–4; Filler Circle grows from Exhaled Reference Outline toward Inhaled Reference Outline.
**Hold Full Phase** — Cycle seconds 4–8; Filler Circle held at Inhaled Reference Outline.
**Exhale Phase** — Cycle seconds 8–12; Filler Circle shrinks from Inhaled Reference Outline toward Exhaled Reference Outline.
**Hold Empty Phase** — Cycle seconds 12–16; Filler Circle held at Exhaled Reference Outline.
**Phase Boundary** — instant (Cycle seconds 0, 4, 8, 12) where one Phase ends and the next begins. Drives Long Vibe in Vibe Every Second and the only Vibe in Vibe Phase Only.
**Cycle Wrap** — instant Cycle Elapsed Sec rolls 15 → 0 (= end of Hold Empty Phase → start of next Inhale Phase). Triggers HR Sample capture and Completed Cycles increment.

**Session** — user-initiated run from app launch / last Reset until app close or next Reset.
**Session Elapsed** — wall time since Session started, shown as MM:SS in Session Elapsed Display.
**Completed Cycles** — total Cycles finished in the current Session (monotonic).
**Cycle Counter** — value `Completed Cycles mod Target Cycles`, rendered in Cycle Counter Display as `X/20`.
**Target Cycles** — per-batch goal of 20 Cycles; every multiple reached fires Session-Complete Vibe.
**Reset** — clears Session Elapsed, Completed Cycles, Cycle Elapsed Sec, Anim Sub Ms, HR Sample Buffer; then re-captures an initial HR Sample.

**Pause State** — app frozen: Tick Handler skips SECOND_UNIT work; Animation Timer keeps firing but skips Phase Fill updates and emits no Vibes.
**Pause Indicator** — two-bar icon shown in Status Strip only while Pause State is true.

**Breathing Circle** — concentric composition in the breathing Layout Slot: Filler Circle + Exhaled Reference Outline + Inhaled Reference Outline.
**Filler Circle** — solid cyan animating disc whose radius interpolates each Phase, driven by Phase Fill.
**Exhaled Reference Outline** — inner thin Cobalt-Blue outline at minimum radius; marks the Hold Empty Phase / Exhale Phase endpoint.
**Inhaled Reference Outline** — outer thin Cobalt-Blue outline at maximum radius; marks the Hold Full Phase / Inhale Phase endpoint.
**Phase Fill** — float `0.0–1.0` derived from current Phase + Anim Sub Ms; sets Filler Circle radius between Exhaled and Inhaled Reference Outlines.

**Vibe Mode** — user-cycled, persisted haptic setting; one of Vibe Every Second / Vibe Phase Only / Vibe Off.
**Vibe Every Second** — Short Vibe each non-Boundary second of a Cycle + Long Vibe on each Phase Boundary.
**Vibe Phase Only** — Short Vibe on each Phase Boundary only; no per-second pulses.
**Vibe Off** — no per-second / per-Boundary Vibes are fired by Tick Handler. Session-Complete Vibe still fires regardless (intentional).
**Short Vibe** — 50 ms pulse.
**Long Vibe** — 250 ms pulse.
**Session-Complete Vibe** — 3-pulse pattern (400/200/400/200/400 ms) fired at every multiple of Target Cycles in Completed Cycles. Fires regardless of Vibe Mode.

**HR Sample** — single BPM reading captured at Cycle Wrap (= end of Hold Empty Phase), validated `30 < bpm < 250`.
**HR Sample Buffer** — 20-entry circular buffer of HR Samples (one slot per Cycle).
**HR Write Index** — next write position in HR Sample Buffer.
**HR Graph** — line plot of HR Sample Buffer in the graph Layout Slot (Brilliant Rose line + 2 px dots per sample).
**HR Min Label** — bottom-left numeric label = min of HR Sample Buffer.
**HR Max Label** — top-left numeric label = max of HR Sample Buffer.
**Current HR** — most recent HR Sample, shown in Current HR Display.

**Status Strip** — horizontal row of indicator icons centered in the top container; auto-recenters when Pause Indicator appears / disappears.
**Vibe Indicator** — red Folly circle in Status Strip: filled = Vibe Every Second, outlined = Vibe Phase Only, crossed-outlined = Vibe Off.
**Backlight Indicator** — icon in Status Strip: 4-point star = Backlight Always-On enabled, outline circle = disabled.

**Backlight Always-On** — user toggle forcing the Pebble backlight on for the duration of the Session; persisted across launches.

**Clock** — current wall time, top-left of top container, 24h or 12h per system setting.
**Session Elapsed Display** — top-right cyan MM:SS rendering of Session Elapsed.
**Cycle Counter Display** — line beneath Session Elapsed Display, renders Cycle Counter as `X/20`.
**Current HR Display** — line beneath Cycle Counter Display, renders Current HR (or `--` until first HR Sample arrives).

**Tick Handler** — SECOND_UNIT subscriber; the **sole Vibe trigger** and the only writer of Cycle Elapsed Sec / Session Elapsed / Completed Cycles.
**Animation Timer** — 100 ms repeating `AppTimer`; only advances Anim Sub Ms and redraws Filler Circle. Never fires Vibes.
**Cycle Elapsed Sec** — `0–15`, owned by Tick Handler.
**Anim Sub Ms** — `0–999`, owned by Animation Timer, reset to 0 each Tick.
**Vibe Mode Persist Key** — persist key `0`; stores Vibe Mode across launches.
**Backlight Persist Key** — persist key `1`; stores Backlight Always-On across launches.
**Layout Slot** — proportional vertical slice of the window. The app uses 4 Layout Slots with weights `31 / 41 / 28 / 0`.

**Top Button** (UP) — short press = toggle Pause State; long press = Reset.
**Middle Button** (SELECT) — advance Vibe Mode (Every Second → Phase Only → Off → …).
**Bottom Button** (DOWN) — toggle Backlight Always-On.

---

## 2. Application overview

The app runs a continuous Box Breathing loop driven by two clocks (Tick Handler + Animation Timer), renders a Breathing Circle plus Status Strip plus HR Graph, persists Vibe Mode and Backlight Always-On between launches, and reads heart rate once per Cycle.

The user controls only three things: Pause State (via Top Button short), Reset (via Top Button long), Vibe Mode (via Middle Button), Backlight Always-On (via Bottom Button).

---

## 3. Entities (persistent / runtime state)

### 3.1 AppState (runtime singleton)
Single global `g_state` aggregating all runtime values:

- `paused` → Pause State
- `backlight_always_on` → Backlight Always-On
- `vibration_mode` → Vibe Mode
- `session_elapsed_ms` → Session Elapsed (ms)
- `cycle_elapsed_sec` → Cycle Elapsed Sec (`0–15`)
- `anim_sub_ms` → Anim Sub Ms (`0–999`)
- `current_phase` → current Phase enum value
- `completed_cycles` → Completed Cycles
- `hr_samples[20]` → HR Sample Buffer
- `hr_write_idx` → HR Write Index
- `hr_sample_count` → number of HR Samples currently held (0…20)
- `current_time` → wall clock seconds used by Clock

### 3.2 Persisted state
- Vibe Mode Persist Key (key `0`, int) — survives app close.
- Backlight Persist Key (key `1`, bool) — survives app close.
All other AppState fields reset on each launch.

---

## 4. Behaviours (event flows)

### 4.1 Cycle progression (Tick Handler, 1 Hz)
On each SECOND_UNIT tick while Pause State is false:
1. Increment Session Elapsed by 1000 ms.
2. Advance Cycle Elapsed Sec by 1 (mod 16); reset Anim Sub Ms to 0; update current Phase. Return `wrapped == true` on Cycle Wrap.
3. Fire per-second Vibe via Vibe Mode dispatch (see 4.3).
4. If Cycle Wrap:
   a. Capture an HR Sample (if sensor returned a valid BPM) and push into HR Sample Buffer.
   b. Increment Completed Cycles.
   c. If Completed Cycles is a multiple of Target Cycles, fire Session-Complete Vibe.
   d. Mark HR Graph + Status Strip for redraw.
5. Mark Clock for redraw (so wall time and Session Elapsed Display refresh).

### 4.2 Breathing animation (Animation Timer, 10 Hz)
Re-registers itself every 100 ms. While Pause State is false:
1. Advance Anim Sub Ms by 100 (capped at 999).
2. Recompute current Phase from `cycle_elapsed_sec * 1000 + anim_sub_ms`.
3. Mark Filler Circle for redraw. Phase Fill is recomputed on draw.

Animation Timer **never** fires Vibes — Vibe firing is exclusive to Tick Handler so it stays aligned with the watch's seconds tick.

### 4.3 Vibe Mode dispatch (per-second)
Called from Tick Handler with current Cycle Elapsed Sec:
- Vibe Off → no-op.
- Vibe Phase Only → Short Vibe iff `cycle_sec % 4 == 0` (Phase Boundary).
- Vibe Every Second → Long Vibe on Phase Boundary, otherwise Short Vibe.

Session-Complete Vibe is *not* gated by Vibe Mode (intentional).

### 4.4 Phase Fill computation
Given current Phase and Cycle Elapsed Sec + Anim Sub Ms:
- Inhale Phase: `Phase Fill = phase_elapsed_ms / 4000` (ramps 0 → 1).
- Hold Full Phase: `Phase Fill = 1.0`.
- Exhale Phase: `Phase Fill = 1 - phase_elapsed_ms / 4000` (ramps 1 → 0).
- Hold Empty Phase: `Phase Fill = 0.0`.

Filler Circle radius = `min_r + (max_r - min_r) * Phase Fill`.

### 4.5 HR sampling
- An initial HR Sample is captured synchronously at app launch and on Reset, so Current HR Display and HR Graph aren't blank if the sensor already has a reading.
- During runtime, HR Samples are captured at Cycle Wrap only — exactly one per Cycle.
- Sensor sampling cadence is hinted via `health_service_set_heart_rate_sample_period(16)` at init (and `0` at deinit). On non-HR platforms this is a compile-time no-op.
- Each new HR Sample overwrites the slot at HR Write Index, then HR Write Index advances `(idx + 1) % 20`. `hr_sample_count` grows up to 20 then stays saturated (circular fill).

### 4.6 Pause / Resume
Top Button short toggles Pause State.
- Tick Handler keeps running every second (so Clock keeps ticking), but skips Cycle work and Vibe firing when paused.
- Animation Timer keeps re-registering but skips Filler Circle updates.
- Pause Indicator appears in Status Strip; the strip recenters to keep all visible indicators horizontally centered.

### 4.7 Reset
Top Button long re-initializes Session Elapsed, Completed Cycles, Cycle Elapsed Sec, Anim Sub Ms, HR Sample Buffer (count + index), and current Phase back to Inhale Phase. Immediately afterwards a fresh initial HR Sample is captured.

### 4.8 Vibe Mode cycling
Middle Button advances Vibe Mode through Every Second → Phase Only → Off → Every Second … and persists to Vibe Mode Persist Key. Status Strip redraws so the Vibe Indicator reflects the new mode.

### 4.9 Backlight Always-On toggle
Bottom Button flips Backlight Always-On, persists to Backlight Persist Key, calls `light_enable(...)`, and redraws Status Strip so the Backlight Indicator reflects the new state.

### 4.10 Init / deinit lifecycle
- `init`: read Vibe Mode + Backlight Always-On from persist; Reset; enable backlight per setting; request HR sampling period; capture initial HR Sample; build UI; subscribe Tick Handler; arm Animation Timer.
- `deinit`: zero HR sampling period; destroy UI.

---

## 5. UI surfaces (Layout Slots)

The window uses a vertical 4-Layout-Slot layout, weights `31 / 41 / 28 / 0`. Background is black.

### 5.1 Slot 0 — Top container (31%)
Single container Layer hosting all top-row elements:
- **Clock** — left half, GOTHIC_24_BOLD, white.
- **Session Elapsed Display** — right half row 1, GOTHIC_24_BOLD, cyan.
- **Cycle Counter Display** — right half row 2, GOTHIC_18_BOLD, white, format `X/20`.
- **Current HR Display** — right half row 3, GOTHIC_18_BOLD, white, format `NN` or `--`.
- **Status Strip** — centered horizontally near the top: Pause Indicator (when visible), Vibe Indicator, Backlight Indicator. Indicators are spaced by 18 px; the entire block recenters when Pause Indicator toggles.
- Round-watch safe-area margins via `PBL_IF_ROUND_ELSE`.

### 5.2 Slot 1 — Breathing Circle (41%)
- **Filler Circle**: cyan (white fallback on mono) solid disc, centered in slot. Radius animates between min radius (24 px) and max radius (slot half-extent) per Phase Fill. No outline.
- **Exhaled Reference Outline**: Cobalt-Blue 1 px circle at min radius.
- **Inhaled Reference Outline**: Cobalt-Blue 1 px circle at max radius.
- (Phase label text is currently commented out — kept in source as a hidden option.)

### 5.3 Slot 2 — HR Graph (28%)
- **HR Min Label** + **HR Max Label**: left margin, GOTHIC_18_BOLD white text with 8-direction black outline; positioned at top-left (max) and bottom-left (min) of the plot area.
- **HR Graph line**: Brilliant Rose, 2 px wide, connecting HR Samples in chronological order with a 2 px filled dot at each point.
- Plot is blank until first HR Sample exists. Y-axis auto-scales to current `[min, max]` of HR Sample Buffer (min span of 5 BPM enforced).

### 5.4 Slot 3 — unused (0%)
Reserved status Layer kept for future use; weight 0 so it occupies no vertical space.

---

## 6. Controls

| Button | Action | Glossary term |
|---|---|---|
| UP short | Toggle Pause State | Top Button |
| UP long  | Reset             | Top Button |
| SELECT short | Advance Vibe Mode | Middle Button |
| DOWN short | Toggle Backlight Always-On | Bottom Button |

---

## 7. Architecture & files

- `src/c/main.c` — app lifecycle (init / deinit), Tick Handler (Cycle progression, HR Sample at Cycle Wrap, Session-Complete Vibe, Completed Cycles increment), Animation Timer registration, all four Button click handlers, Vibe Mode + Backlight Always-On persistence, initial HR Sample capture, Reset.
- `src/c/app_state.h` — `AppState` struct, `BreathingPhase` enum (Inhale / Hold Full / Exhale / Hold Empty Phase), `VibrationMode` enum (Vibe Every Second / Phase Only / Off), `HR_SAMPLE_BUFFER` = 20 = Target Cycles.
- `src/c/breathing.c` / `.h` — Phase determination from elapsed ms, `breathing_update` (Animation Timer hook, advances Anim Sub Ms), `breathing_tick` (Tick Handler hook, advances Cycle Elapsed Sec, returns Cycle Wrap), `breathing_get_fill` (Phase Fill computation).
- `src/c/vibration.c` / `.h` — Short Vibe / Long Vibe / Session-Complete Vibe pattern definitions; `vibration_trigger_for_second` (Vibe Mode dispatcher); `vibration_trigger_session_complete`.
- `src/c/ui.c` / `.h` — all rendering: Breathing Circle (Filler Circle + Exhaled / Inhaled Reference Outlines), Status Strip (Pause / Vibe / Backlight Indicators), top container text layers (Clock, Session Elapsed Display, Cycle Counter Display, Current HR Display), HR Graph (line, dots, HR Min/Max Labels).
- `src/c/layout.c` / `.h` — proportional vertical Layout Slot manager.

### 7.1 Vibe sync invariant
Cycle Elapsed Sec is owned by Tick Handler. Anim Sub Ms is owned by Animation Timer and reset to 0 by every Tick. **All Vibe firing happens inside Tick Handler.** This guarantees Vibes always land precisely on the watch's second boundaries.

### 7.2 Layout safety
`layout_create` zero-allocates both layer and weight arrays so the weight-sum guard in `layout_add_layer_with_params` is reliable.

### 7.3 Build / packaging
- `package.json` includes `"sources": ["src/c"]` so the Rebble cloud build picks up the C sources (local builds use the `wscript` glob).
- Targets: `aplite`, `basalt`, `chalk`, `diorite`, `emery`, `flint`, `gabbro`. Color features gated with `COLOR_FALLBACK` / `#ifdef PBL_COLOR`.

---

## 8. Glossary bookmarks in source

Every Layer in this app maps to one or more glossary terms. Source files carry `// glossary: <term_snake_case>[, <term_snake_case>...]` comments next to the relevant code so the codebase is grep-able by glossary term. Examples:

- `// glossary: cycle_wrap, hr_sample, completed_cycles, session_complete_vibe`
- `// glossary: filler_circle, exhaled_reference_outline, inhaled_reference_outline`
- `// glossary: vibe_mode_dispatch, short_vibe, long_vibe, phase_boundary`

To find code for a glossary term: `grep -rn "glossary:.*<term_snake_case>" src/`.
