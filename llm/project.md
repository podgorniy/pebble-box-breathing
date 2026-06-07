# Box Breathing — Pebble Watchapp

A native C watchapp for Pebble / Rebble smartwatches that facilitates **Box Breathing** and related paced-breathing techniques: it guides the user through a configurable Breathing Technique (Cycle Length varies per technique) with synchronized Vibes, a live Cycle Counter against a Target Cycles goal, and a per-Cycle HR Graph.

---

## 1. Glossary

### 1.1 Terms (bare list, grouped)

**Breathing model**
Box Breathing · Cycle · Phase · Inhale Phase · Hold Full Phase · Exhale Phase · Hold Empty Phase · Phase Boundary · Cycle Wrap

**Breathing technique**
Breathing Technique · Technique Preset · Phase Duration · Phase Duration Table · Cycle Length · Technique Display · Phase Dot Indicator · Technique Persist Key · Classic Technique · Beginner Technique · Extended Technique · Meditation Technique · Advanced Technique · Extended-Exhale Technique · 4-7-8 Technique

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

**Display mode**
Display Mode · Display Default · Display Backlight · Display Minimal · Backlight Always-On · Minimal Mode

**Top container**
Clock · Session Elapsed Display · Cycle Counter Display · Current HR Display

**Engine / architecture**
Tick Handler · Animation Timer · Cycle Elapsed Sec · Anim Sub Ms · Vibe Mode Persist Key · Backlight Persist Key · Technique Persist Key · Layout Slot

**Controls**
Top Button · Middle Button · Middle Button Long · Bottom Button

### 1.2 Terms with descriptions

**Box Breathing** — the equal-duration 4-Phase breathing technique that gives the app its name; the default Breathing Technique (Classic Technique, `4-4-4-4`). Other Techniques may shorten/lengthen Phases or omit Hold Empty Phase.
**Cycle** — one full iteration of the active Breathing Technique. Cycle Length depends on the Technique (9 s for Beginner … 32 s for Advanced). Unit counted by Cycle Counter.
**Phase** — one segment of a Cycle, of duration set by the active Breathing Technique. Up to four exist (Inhale, Hold Full, Exhale, Hold Empty); the 4-7-8 Technique has only the first three.
**Inhale Phase** — first Phase of a Cycle; Filler Circle grows from Exhaled Reference Outline toward Inhaled Reference Outline over the Inhale Phase Duration.
**Hold Full Phase** — second Phase of a Cycle; Filler Circle held at Inhaled Reference Outline for the Hold Full Phase Duration.
**Exhale Phase** — third Phase of a Cycle; Filler Circle shrinks from Inhaled Reference Outline toward Exhaled Reference Outline over the Exhale Phase Duration.
**Hold Empty Phase** — fourth Phase of a Cycle (absent in the 4-7-8 Technique); Filler Circle held at Exhaled Reference Outline for the Hold Empty Phase Duration.
**Phase Boundary** — instant (Cycle seconds at the cumulative Phase Duration offsets of the active Breathing Technique, e.g. 0/4/8/12 for Classic, 0/4/11 for 4-7-8) where one Phase ends and the next begins. Drives Long Vibe in Vibe Every Second and the only Vibe in Vibe Phase Only.
**Cycle Wrap** — instant Cycle Elapsed Sec rolls `(Cycle Length − 1) → 0` (= end of the last Phase of the active Breathing Technique → start of next Inhale Phase). Triggers HR Sample capture and Completed Cycles increment.

**Session** — user-initiated run from app launch / last Reset until app close or next Reset.
**Session Elapsed** — wall time since Session started, shown as MM:SS in Session Elapsed Display.
**Completed Cycles** — total Cycles finished in the current Session (monotonic).
**Cycle Counter** — equals Completed Cycles directly (no Target Cycles wrap), rendered in Cycle Counter Display as `X/20`. Display reads `21/20`, `22/20`, … past the first batch; only Session-Complete Vibe still fires on multiples of Target Cycles.
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

**Breathing Technique** — user-cycled, persisted preset that sets the Phase Duration for each Phase of a Cycle and therefore the Cycle Length. Default Classic Technique (`4-4-4-4`). Advanced via Middle Button long press; persisted to Technique Persist Key.
**Technique Preset** — one row of the Phase Duration Table: Technique Display string, phase count (3 or 4), per-Phase Phase Durations in seconds.
**Phase Duration** — seconds spent in a single Phase under the active Breathing Technique.
**Phase Duration Table** — static array of all Technique Presets, defined in `techniques.c`. Single source of truth for Cycle Length, Phase Boundaries, and the Technique Display string.
**Cycle Length** — sum of all Phase Durations of the active Breathing Technique, in seconds. Drives the modulo in Cycle Elapsed Sec progression.
**Technique Display** — `GOTHIC_18_BOLD` line beneath the Clock rendering the active Breathing Technique as `X-X-X-X` (or `X-Y-Z` for the 4-7-8 Technique).
**Phase Dot Indicator** — 2 px white filled dot drawn under the digit of the Technique Display matching the current Phase. Advances digit-by-digit on each Phase Boundary; resets to the leftmost digit at Cycle Wrap.
**Technique Persist Key** — persist key `2`, int; stores active Breathing Technique across launches. Older builds without the key fall back to Classic Technique.
**Classic Technique** — `4-4-4-4` Box Breathing — 4 s Inhale / 4 s Hold Full / 4 s Exhale / 4 s Hold Empty (Cycle Length 16 s). Launch default.
**Beginner Technique** — `3-3-3-3` (Cycle Length 12 s).
**Extended Technique** — `5-5-5-5` (Cycle Length 20 s).
**Meditation Technique** — `6-6-6-6` (Cycle Length 24 s).
**Advanced Technique** — `8-8-8-8` (Cycle Length 32 s).
**Extended-Exhale Technique** — `4-4-6-4` (Cycle Length 18 s).
**4-7-8 Technique** — 3-Phase technique: 4 s Inhale, 7 s Hold Full, 8 s Exhale, immediate Cycle Wrap, no Hold Empty Phase (Cycle Length 19 s).

**HR Sample** — single BPM reading captured at Cycle Wrap (= end of Hold Empty Phase), validated `30 < bpm < 250`.
**HR Sample Buffer** — 20-entry circular buffer of HR Samples (one slot per Cycle).
**HR Write Index** — next write position in HR Sample Buffer.
**HR Graph** — line plot of HR Sample Buffer in the graph Layout Slot (Brilliant Rose line + 2 px dots per sample).
**HR Min Label** — bottom-left numeric label = min of HR Sample Buffer.
**HR Max Label** — top-left numeric label = max of HR Sample Buffer.
**Current HR** — most recent HR Sample, shown in Current HR Display.

**Status Strip** — horizontal row of indicator icons centered in the top container; auto-recenters when Pause Indicator appears / disappears.
**Vibe Indicator** — red Folly circle in Status Strip: filled = Vibe Every Second, outlined = Vibe Phase Only, crossed-outlined = Vibe Off.
**Backlight Indicator** — icon in Status Strip reflecting Display Mode: outline circle = Display Default, 4-point star = Display Backlight, filled dot = Display Minimal.

**Display Mode** — user-cycled, persisted three-state setting controlling Backlight Always-On and HR sampling: Display Default → Display Backlight → Display Minimal → … Persisted to Backlight Persist Key as int.
**Display Default** — backlight off, HR sampling on, full 31/41/28/0 Layout Slot weights. App's launch default.
**Display Backlight** — Backlight Always-On enabled, HR sampling on, full layout. (Equivalent to the legacy "backlight on" toggle.)
**Display Minimal** — backlight off, HR sampling off (period 0); Current HR Display blanked and HR Graph hidden; Layout Slot weights reflowed to 31/69/0/0 so the Breathing Circle expands into the freed 28%.
**Backlight Always-On** — backlight forced on for the Session; active iff Display Mode == Display Backlight.
**Minimal Mode** — synonym for Display Minimal.

**Clock** — current wall time, top-left of top container, 24h or 12h per system setting.
**Session Elapsed Display** — top-right cyan MM:SS rendering of Session Elapsed.
**Cycle Counter Display** — line beneath Session Elapsed Display, renders Cycle Counter as `X/20`.
**Current HR Display** — line beneath Cycle Counter Display, renders Current HR (or `--` until first HR Sample arrives).
(Technique Display sits beneath the Clock in the left half — see "Breathing technique" group above.)

**Tick Handler** — SECOND_UNIT subscriber; the **sole Vibe trigger** and the only writer of Cycle Elapsed Sec / Session Elapsed / Completed Cycles.
**Animation Timer** — 100 ms repeating `AppTimer`; only advances Anim Sub Ms and redraws Filler Circle. Never fires Vibes.
**Cycle Elapsed Sec** — `0 .. (Cycle Length − 1)`, owned by Tick Handler.
**Anim Sub Ms** — `0–999`, owned by Animation Timer, reset to 0 each Tick.
**Vibe Mode Persist Key** — persist key `0`; stores Vibe Mode across launches.
**Backlight Persist Key** — persist key `1`; stores Backlight Always-On across launches.
**Layout Slot** — proportional vertical slice of the window. The app uses 4 Layout Slots with weights `31 / 41 / 28 / 0`.

**Top Button** (UP) — short press = toggle Pause State; long press = Reset.
**Middle Button** (SELECT short) — advance Vibe Mode (Every Second → Phase Only → Off → …).
**Middle Button Long** (SELECT long, 500 ms) — advance Breathing Technique through the Phase Duration Table (Classic → Beginner → Extended → Meditation → Advanced → Extended-Exhale → 4-7-8 → Classic). Resets Cycle position only; keeps Session Elapsed, Completed Cycles, HR Sample Buffer.
**Bottom Button** (DOWN) — cycle Display Mode (Default → Backlight → Minimal → …).

---

## 2. Application overview

The app runs a continuous paced-breathing loop driven by two clocks (Tick Handler + Animation Timer), renders a Breathing Circle plus Status Strip plus HR Graph plus Technique Display, persists Vibe Mode, Display Mode, and Breathing Technique between launches, and reads heart rate once per Cycle.

The user controls only five things: Pause State (via Top Button short), Reset (via Top Button long), Vibe Mode (via Middle Button short), Breathing Technique (via Middle Button long), Display Mode (via Bottom Button — cycles Display Default → Display Backlight → Display Minimal).

---

## 3. Entities (persistent / runtime state)

### 3.1 AppState (runtime singleton)
Single global `g_state` aggregating all runtime values:

- `paused` → Pause State
- `display_mode` → Display Mode (DISPLAY_DEFAULT / DISPLAY_BACKLIGHT / DISPLAY_MINIMAL)
- `vibration_mode` → Vibe Mode
- `breathing_technique` → Breathing Technique (TECHNIQUE_CLASSIC … TECHNIQUE_478)
- `session_elapsed_ms` → Session Elapsed (ms)
- `cycle_elapsed_sec` → Cycle Elapsed Sec (`0 .. Cycle Length − 1`)
- `anim_sub_ms` → Anim Sub Ms (`0–999`)
- `current_phase` → current Phase enum value
- `completed_cycles` → Completed Cycles
- `hr_samples[20]` → HR Sample Buffer
- `hr_write_idx` → HR Write Index
- `hr_sample_count` → number of HR Samples currently held (0…20)
- `current_time` → wall clock seconds used by Clock

### 3.2 Persisted state
- Vibe Mode Persist Key (key `0`, int) — survives app close.
- Backlight Persist Key (key `1`, int) — stores Display Mode across launches. Older builds wrote a bool here; on upgrade `persist_read_int` returns 0 and the user falls back to Display Default.
- Technique Persist Key (key `2`, int) — stores Breathing Technique across launches. Absent on older builds → falls back to Classic Technique.
All other AppState fields reset on each launch.

---

## 4. Behaviours (event flows)

### 4.1 Cycle progression (Tick Handler, 1 Hz)
On each SECOND_UNIT tick while Pause State is false:
1. Increment Session Elapsed by 1000 ms.
2. Advance Cycle Elapsed Sec by 1 (mod Cycle Length of the active Breathing Technique); reset Anim Sub Ms to 0; update current Phase by walking the Phase Duration Table. Return `wrapped == true` on Cycle Wrap.
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
Called from Tick Handler with current Cycle Elapsed Sec; Phase Boundary is decided via the Phase Duration Table (true when Cycle Elapsed Sec equals any cumulative Phase Duration offset of the active Breathing Technique):
- Vibe Off → no-op.
- Vibe Phase Only → Short Vibe iff Phase Boundary.
- Vibe Every Second → Long Vibe on Phase Boundary, otherwise Short Vibe.

Session-Complete Vibe is *not* gated by Vibe Mode (intentional).

### 4.4 Phase Fill computation
Given current Phase, Cycle Elapsed Sec + Anim Sub Ms, and the Phase Duration `D` (ms) of the current Phase under the active Breathing Technique:
- Inhale Phase: `Phase Fill = phase_elapsed_ms / D` (ramps 0 → 1).
- Hold Full Phase: `Phase Fill = 1.0`.
- Exhale Phase: `Phase Fill = 1 - phase_elapsed_ms / D` (ramps 1 → 0).
- Hold Empty Phase: `Phase Fill = 0.0` (Phase not present in 4-7-8 Technique).

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

### 4.9 Display Mode cycling
Bottom Button advances Display Mode through Default → Backlight → Minimal → Default …, persists to Backlight Persist Key (int), then applies the new mode:
- Backlight: `light_enable(true)` iff Backlight; otherwise `light_enable(false)`.
- HR sampling: `health_service_set_heart_rate_sample_period(0)` in Minimal, `16` otherwise.
- Layout: `ui_apply_display_mode()` reflows Slot 1 / Slot 2 weights (41/28 ↔ 69/0) so the Breathing Circle grows in Minimal.
- Status Strip + all displays redraw via `ui_update_all()`.

### 4.10 Init / deinit lifecycle
- `init`: read Vibe Mode + Display Mode + Breathing Technique from persist (each clamped to valid range; absent Breathing Technique → Classic Technique); Reset; enable backlight iff Display Backlight; set HR sampling period (0 for Minimal, else 16); capture initial HR Sample (skipped in Minimal); build UI; apply Display Mode layout; subscribe Tick Handler; arm Animation Timer.
- `deinit`: zero HR sampling period; destroy UI.

### 4.11 Technique cycling
Middle Button long press (500 ms) advances Breathing Technique through Classic → Beginner → Extended → Meditation → Advanced → Extended-Exhale → 4-7-8 → Classic and persists to Technique Persist Key. Side effects:
- Resets Cycle position only (Cycle Elapsed Sec, Anim Sub Ms, current Phase → Inhale Phase).
- Does **not** clear Session Elapsed, Completed Cycles, or the HR Sample Buffer.
- Re-renders Technique Display + Phase Dot Indicator + Status Strip via `ui_update_all()`.

---

## 5. UI surfaces (Layout Slots)

The window uses a vertical 4-Layout-Slot layout, weights `31 / 41 / 28 / 0`. Background is black.

### 5.1 Slot 0 — Top container (31%)
Single container Layer hosting all top-row elements:
- **Clock** — left half row 1, GOTHIC_24_BOLD, white.
- **Session Elapsed Display** — right half row 1, GOTHIC_24_BOLD, cyan.
- **Technique Display** — left half row 2, GOTHIC_18_BOLD, drawn directly under the Clock as `X-X-X-X` (or `X-Y-Z` for the 4-7-8 Technique). Rendered by a dedicated custom Layer (not a TextLayer) so per-digit centres are known to the Phase Dot Indicator.
- **Phase Dot Indicator** — 2 px white filled dot inside the Technique Display layer, centred under the digit of the current Phase. Redraw is triggered by the Tick Handler when `breathing_tick` changes the Phase (the Animation Timer carries a defensive same-check, but phase transitions land on the second boundary so the Tick Handler is the actual trigger).
- **Cycle Counter Display** — right half row 2, GOTHIC_18_BOLD, white, format `X/20`.
- **Current HR Display** — right half row 3, GOTHIC_18_BOLD, white, format `NN` or `--`.
- **Status Strip** — centered horizontally near the top: Pause Indicator (when visible), Vibe Indicator, Backlight Indicator. Indicators are spaced by 11 px; the entire block recenters when Pause Indicator toggles.
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
| SELECT long  | Advance Breathing Technique | Middle Button Long |
| DOWN short | Advance Display Mode (Default → Backlight → Minimal) | Bottom Button |

---

## 7. Architecture & files

- `src/c/main.c` — app lifecycle (init / deinit), Tick Handler (Cycle progression, HR Sample at Cycle Wrap, Session-Complete Vibe, Completed Cycles increment), Animation Timer registration, all five Button click handlers (UP short/long, SELECT short/long, DOWN short), Vibe Mode + Display Mode + Breathing Technique persistence, initial HR Sample capture, Reset, and the `reset_cycle_position()` helper shared by Reset and Technique cycling.
- `src/c/app_state.h` — `AppState` struct, `BreathingPhase` enum (Inhale / Hold Full / Exhale / Hold Empty Phase), `VibrationMode` enum (Vibe Every Second / Phase Only / Off), `DisplayMode` enum (Default / Backlight / Minimal), `HR_SAMPLE_BUFFER` = 20 = Target Cycles. Includes `techniques.h` for `BreathingTechnique`.
- `src/c/techniques.c` / `.h` — `BreathingTechnique` enum, `TechniquePreset` struct, the Phase Duration Table (`g_techniques[]`), and helpers (`technique_current`, `technique_cycle_length_sec`, `technique_phase_at_sec`, `technique_phase_start_sec`, `technique_phase_duration_sec`, `technique_is_phase_boundary`, `technique_phase_index`).
- `src/c/breathing.c` / `.h` — Phase determination via the Phase Duration Table, `breathing_update` (Animation Timer hook, advances Anim Sub Ms), `breathing_tick` (Tick Handler hook, advances Cycle Elapsed Sec mod Cycle Length, returns Cycle Wrap), `breathing_get_fill` (Phase Fill computation against current Phase Duration).
- `src/c/vibration.c` / `.h` — Short Vibe / Long Vibe / Session-Complete Vibe pattern definitions; `vibration_trigger_for_second` (Vibe Mode dispatcher; consults the Phase Duration Table for Phase Boundary); `vibration_trigger_session_complete`.
- `src/c/ui.c` / `.h` — all rendering: Breathing Circle (Filler Circle + Exhaled / Inhaled Reference Outlines), Status Strip (Pause / Vibe / Backlight Indicators), top container text layers (Clock, Session Elapsed Display, Cycle Counter Display, Current HR Display) + the Technique Display custom Layer (digit-by-digit text + Phase Dot Indicator), HR Graph (line, dots, HR Min/Max Labels). `ui_apply_display_mode()` reflows Layout Slot weights for Display Minimal. `ui_update_technique()` marks the Technique Display layer dirty when the current Phase changes.
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
- `// glossary: breathing_technique, phase_duration_table, technique_persist_key`
- `// glossary: technique_display, phase_dot_indicator, classic_technique, technique_478`

To find code for a glossary term: `grep -rn "glossary:.*<term_snake_case>" src/`.
