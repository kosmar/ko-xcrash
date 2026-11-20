# Endless plate continuum – per-genre recipe

The preset knob of a genre walks a deterministic continuum of generated plates per era. The 60 curated ROM plates
stay as landmarks at `n = k * 8` (k = 0…9 per era) and fade out over ±3 clicks; EDIT + PRESET jumps by 997, EDIT +
ERA sets era compliance, DJ transitions step along `n`. Status: done for all eleven genres (TECHNO, HOUSE, DUB, FUNK,
SNOWCRASH, IDM, HIPHOP, HEAVY_PSYCH, JAZZ, POP, BASS) on `main`. This file tells an agent how a genre was added alone,
without touching another genre’s files.

## What is shared (do not edit per genre)

| File | Role |
| --- | --- |
| `firmware/src/fm_plate_gen.h` | noise, landmark weight, blend, mask, name helpers, cfg tail, DJ step constants |
| `firmware/src/fm_plate_views.h` | runtime S2…S8 views, bit-exact C port of `tools/gen_views.py` |
| `firmware/src/fm_pgen_core.h` | `fm_pgen_genome_t` (one row per era), `fm_pgen_def_t` (wiring), `fm_pgen_t` (live plate), `fm_pgen_lead_t` (lead phrase), `fm_pgen_build()` |
| `firmware/src/genre_pack.h` | generic `#if defined(FM_PGEN_STATE)` block, no per-genre lines |
| `firmware/src/fm_app.c` | all integration is generic via `FM_PGEN_ON` / `genre_pgen_*` |
| `firmware/src/fm_views_data.h` + `tools/gen_views.py` | ROM views + `genre_pgen_views_hit` hook (generated file) |
| `tests/plate_gen_drift.c` | generic check (views port, landmarks, determinism, drift, hard rules, cross-era, compliance, golden); optional genre extras via the rules header (`PGEN_LANDMARK_EXTRA`, `PGEN_GOLDEN_EXTRA`, `PGEN_EXTRA_CHECKS`) and `FM_PGEN_VIEWS_HAND` (skip the views port) – used by BASS only |
| `tests/pgen_check.sh <genre>` | builds and runs the check for one genre |
| `tests/pgen_dj_trace.sh <genre>` | lldb trace: DJ turns step `n` by the right amount, compliance wander ±16 |
| `tests/pgen_ui_test.txt`, `tests/pgen_dj_test.txt` | host input scripts (UI walk, DJ phrases), genre-agnostic |
| `tools/pgen_era_stats.py <genre>` | per-era ROM stats + draft genome table |

If a genre really needs a core change (new gene, new per-lane rule), do it in its own commit that keeps the TECHNO and
HOUSE goldens unchanged (`tests/pgen_check.sh techno`, `… house`). Gene ids in `fm_pgen_core.h` are part of every
continuum: never renumber or reorder them.

## Files a genre touches (merge-safe)

Only its own files, so genres can be done in parallel:

- `firmware/src/<genre>_data.h` – RAM state, opt-in macros, routed accessors, `#include "<genre>_gen.h"` at the end
- the generator that writes that header, if one exists (mirror the snippet edit there, see table below)
- `firmware/src/<genre>_gen.h` – new: genome, limits, def, build wrapper
- `tests/pgen_rules_<genre>.h` – new: musical rules beyond must/ban
- `tests/pgen_golden/<genre>.txt` – new: locked golden hash
- POP only: a POP-only `#if defined(GENRE_POP)` spot in `fm_app.c` (direct `POP_PLATE_ROOT[]` read, see POP)
- JAZZ only: its section of `fm_views_data.h` (was stale, fixed); see JAZZ

Never edit `genre_pack.h`, `fm_pgen_core.h`, `plate_gen_drift.c` or another genre’s files in a genre commit.

## Steps (same for every genre)

1. **Survey.** `python3 tools/pgen_era_stats.py <genre>`: ranges, pools, lane densities, `all` (on in every curated
   plate of the era) and `never` (on in none) per lane, plus a draft genome. Note the mask width it prints (16 or 32
   steps) and which per-slot tables it found. Read the genre section below for hard rules and special paths.

2. **RAM state + routing in `<genre>_data.h`** (KO HOUSE is the 16-step template, KO TECHNO the lead-gate template):

   ```c
   #include "fm_pgen_core.h"

   /* Live generated plate (endless continuum); shadows the ROM trax at <genre>_gen.slot. */
   static fm_pgen_t <genre>_gen = { .slot = 0xFFu };
   static fm_pgen_lead_t <genre>_gen_lead;          /* lead-gate genres only */

   #define FM_PGEN_STATE <genre>_gen
   #define FM_PGEN_BUILD(era, n, c) <genre>_gen_build(era, n, c)

   static inline uint8_t <genre>_gen_hit(uint8_t ix)
   {
       return fm_pgen_hit(&<genre>_gen, ix);
   }
   ```

   Put it before the first accessor (after the ROM tables it needs only for types; the def lives in `<genre>_gen.h`).
   Then route **every** read of a per-slot table through a hit check, inside the header and outside it
   (`rg -n '<PREFIX>(_[A-Z_]+)?\[' firmware/src`):

   - `<genre>_<preset>_name()` → `<genre>_gen.name`; `<genre>_<preset>()` → `&<genre>_gen.pl`
   - root / scale / swing / kit / lead-or-stab rate → new accessors returning `<genre>_gen.root|scale|swing|kit|rate`
   - lead gate32 / gate_cs → `<genre>_gen.lead_gate|gate_cs`; lead pitch / len / bend → `<genre>_gen_lead.pitch|len|bend`
   - `*_apply`, `*_preset_rebuild`, `dub_plate_groove_read`: replace every `TABLE[slot]` with the accessor
   - reads of `TABLE[era_first_slot]` in `fm_app.c` (era defaults, ~line 9830–9862) may stay: they want the ROM
   - per-slot tables the core does not generate (BASS_WALK, PLATE_SEQ, POP harmony, HP lead voice…) keep reading the
     landmark slot `k`: the generated plate shadows slot `era * 10 + k`, so these follow the nearest landmark. That is
     the accepted default; generate them only if the genre section says so.

   Finally `#include "<genre>_gen.h"` right before the header’s closing `#endif`.

3. **Mirror in the generator.** If the header is written by a script (see table), make the same edit in the script’s
   text (TECHNO: `write_apply_snippet()` in `tools/gen_techno_pack.py`) and prove reproduction in a scratch copy:
   `git ls-files -co --exclude-standard | tar -cf - -T - | tar -x -C /tmp/x`, `git init`, commit, run the script,
   `git status --short` must not list `<genre>_data.h`. Several scripts also rewrite `fm_app.c` / `fm_views_data.h` /
   DSP headers non-idempotently on main already – that drift is not yours; only the data header must be byte-exact.

4. **Genome in `<genre>_gen.h`** (copy `house_gen.h` or `techno_gen.h`):
   - one `fm_pgen_genome_t` row per era in era order; positional fields:
     `bpm_lo, bpm_hi, dens_lo, dens_hi, swing_lo, swing_hi, pump_lo, pump_hi, tank_lo, tank_hi, wob_lo, wob_hi,
     dly_lo, dly_hi, rev_lo, rev_hi, fb_lo, fb_hi, loop_filt, root_lo, root_hi, scale_pool, kit_pool, bass_pool,
     voice_pool, d[7], must[7], ban[7], lead_len_max, gate_cs, rate_lo, rate_hi`; lanes in `d/must/ban` are
     kick, rim, hat, sub, chord, siren, lead
   - ranges: take the curated min…max of the full-tempo plates; most genres have slow/variant plates at slots 8 and 9
     of each era – leave those out of bpm/density windows (they stay reachable as landmarks)
   - `d[]` is the step probability **before** must/ban; with must steps already on, lower `d` to the extra density
   - hard rules = `must` / `ban` masks of the plate’s own era (never borrowed): `must & ban` must be 0 (the check fails
     otherwise; only TECHNO has a grandfathered overlap). bpm window, scale pool and rate range are hard too
   - a lane that must never be empty needs an anchor `must` step (HOUSE: offbeat bass 0x0404), density alone
     produces empty bars now and then, especially at compliance 0 where densities are borrowed
   - limits row (`<GENRE>_GEN_LIM`, designated init): floor/ceil per numeric gene after compliance widening
   - def: `key` = 4 ASCII bytes of the genre + `1` (e.g. `0x484F5531u` “HOU1”), `words` / `word_n` = name word list
     (see “Plate names” below),
     `rom` = plate table, ROM per-slot tables (`NULL` if absent; `uint8_t` tables cast to `const int8_t *`, values
     < 128), `steps` = mask width (16 or 32), lead genres: `lead_gate`, `gate_cs`, `lead_pitch` (flattened
     `&T[0][0]`), `lead_len`, `lead_bend`, `intervals` (8 pitch moves in the table’s pitch unit), `pitch_n` (16 or 32),
     `lead_hi_echo = 1u` (matches `gen_views.py`; TECHNO keeps 0 for its golden)
   - `#define FM_PGEN_DEF <GENRE>_GEN_DEF`, lead genres also `#define FM_PGEN_LEAD_STATE <genre>_gen_lead`
   - wrapper: `fm_pgen_build(&<genre>_gen, &<genre>_gen_lead /* or NULL */, &<GENRE>_GEN_DEF, <genre>_clamp_era(era), n, comply);`

5. **Tests.**
   - `tests/pgen_rules_<genre>.h`: `static int pgen_rules_check(uint8_t era, int32_t n)` returning a fail count for
     the current `G` (= state) plate; rules the masks cannot express (minimum hits, maximum busy-ness, lane
     relations such as “clap never doubles the downbeat kick”). Called for every generated (w = 0) plate at
     compliance 0 and 127.
   - `tests/pgen_check.sh <genre>` until `OK`: views port 0 diffs, no landmark FAIL, 0 hard-rule breaks, cross-era
     same-n distance < 60 % of unrelated-n distance (free steps only), compliance 0 borrows / 127 never.
   - lock the golden: put the printed hash into `tests/pgen_golden/<genre>.txt`, rerun (`want` = `got`).
   - `tests/pgen_check.sh techno` and `tests/pgen_check.sh house` still OK (core untouched → trivially).
   - `tests/pgen_dj_trace.sh <genre>` → `pgen_dj: OK`.
   - Host UI walk: `cc -O2 -Wall -Wno-unused-function -DDINGHY_HOST '-DAPP_VER="dev"' -DGENRE_<G> -Ifirmware/src
     -Ifirmware/gen -o build/host/<g> firmware/src/fm_app.c` (zero new warnings; add the genre’s `-DFELUCCA_ID`
     like `tests/run_dub_host.sh` does), then `build/host/<g> build/host/<g>_pg tests/pgen_ui_test.txt`; convert a
     shot (`sips -s format png build/host/<g>_pg-900.ppm --out /tmp/a.png`) and check a generated word name (up to 8 letters), the
     era and a sane BPM.
   - Existing host test of the genre (`tests/run_dub_host.sh <genre>` where listed) still passes.
   - All genres still build with unchanged warning counts (baseline on a428d43: DUB 1, HOUSE 0, HEAVY_PSYCH 9,
     JAZZ 1, IDM 0, SNOWCRASH 2, HIPHOP 1, POP 0, FUNK 1, TECHNO 0, BASS 10).
   - Firmware: `./build.sh --app <app>` passes its checks (image / RAM budget / register access lines `ok`).
   - Emu: `sh web/emu/build.sh <app>-only` builds without warnings; for a live check serve this worktree on a free
     port (`PORT=8768 python3 web/emu/serve.py`), never on :8767 (another worktree owns it).

6. **Commit** on the genre branch: `KO <GENRE>: endless plate continuum`, English, typographic quotes/dashes.

Filter all long outputs (`| tail`, `| rg`): build logs and the check’s per-plate FAIL lines are long.

## Generators and reproduction (state on a428d43)

| Genre | Data header | Writer | Data header reproduced byte-exactly? |
| --- | --- | --- | --- |
| DUB (default) | `dub_data.h` | hand-maintained | – (no generator) |
| HOUSE | `house_data.h` | hand-maintained (`expand_*` one-shot scripts are historical) | – |
| HEAVY_PSYCH | `heavy_psych_data.h` | hand + `tools/gen_hp_lead_riffs.py` (lead tables only) | lead tables yes (needs full git history: reads commit bcf4c42) |
| JAZZ | `jazz_data.h` | hand + `tools/gen_jazz_lead_riffs.py` (lead tables) | **no**: the script rewrites `jazz_data.h` (a428d43 hand-encoded three gates) |
| IDM | `idm_data.h` | hand + `tools/gen_idm_lead_riffs.py` | **no**: script is stale (expects 48 rows, header has 60) – edit by hand |
| SNOWCRASH | `snowcrash_data.h` | `tools/gen_snowcrash_pack.py` | yes (script also rewrites `fm_app.c`, `fm_views_data.h`, `snowcrash_dsp.h`) |
| HIPHOP | `hiphop_data.h` | hand-maintained | – |
| POP | `pop_data.h` | `tools/gen_pop_pack.py` | yes |
| FUNK | `funk_data.h` | hand-maintained | – |
| TECHNO | `techno_data.h` | `tools/gen_techno_pack.py` | yes (script also drifts `fm_app.c`, `fm_views_data.h`) |
| (views) | `fm_views_data.h` | `tools/gen_views.py` | yes: BASS block is hand-authored, kept verbatim (`PRESERVE`) |

For the lead-riff scripts (HP/JAZZ/IDM) the routing snippet lives outside the regions they rewrite; check that a run
leaves your snippet intact rather than mirroring it into the script.

## Per-genre sections

Stats below are from `tools/pgen_era_stats.py` (curated min…max over all ten plates; narrow BPM to the full-tempo
plates). “all” = on in every curated plate of the era.

### HOUSE – done (reference for 16-step genres)

- Files: `house_data.h` (routing), `house_gen.h`, `tests/pgen_rules_house.h`, `tests/pgen_golden/house.txt`.
- 16-step masks, lane S = siren (STAB), per-slot ROOT/SCALE/SWING/KIT/STAB_RATE (int8), no lead tables.
- Hard rules: 4/4 kick 0x1111 with extras only on 13–15 (ban 0x0EEE); GARAGE downbeat only + 2-step, no kick on the
  backbeat; clap 0x1010, never on 1/3; open hat 0x4444 (ACID 0x8888 16ths); bass anchor 0x0404 (FILTER disco
  0x4444, TECH no bass on quarters). Rules header: sub ≥ 2 hits, chord + stab ≤ 10, kick ≤ 7 (GARAGE ≤ 6).
- Kit indices go up to 13 > `DKIT_N`; every consumer uses `% DKIT_N`, keep curated values in the pool.
- `house_bass_walk_deg()` and `HOUSE_PLATE_SEQ` follow the landmark slot.

### FUNK – done (HOUSE clone)

- `funk_data.h` hand-maintained, `FUNK_GROOVE` (`funk_groove_t` = `dub_plate_t`), 16-step, lane S = siren (stab),
  per-slot `FUNK_GROOVE_ROOT/SCALE/SWING/KIT` + `FUNK_GROOVE_STAB_RATE` (int8). `FUNK_BASS_WALK[era][k]` follows k.
- Eras POCKET, CLAV, SLAP, DISCO, BOOGIE, BROKEN. BPM: POCKET 96–108, CLAV 100–112, SLAP 92–104, DISCO 112–122,
  BOOGIE 108–118, BROKEN 84–96 (narrow to full-tempo plates).
- Hard rules: “the one” kick – POCKET/CLAV must 0x2141 (all plates), SLAP 0x0101, DISCO/BOOGIE 4/4 0x1111, BROKEN
  0x0001; snare/clap backbeat 0x1010 every era; hats POCKET/BROKEN 0x4444, CLAV 0x4C4C, SLAP 0x0404, DISCO/BOOGIE
  0x5555 (16th disco hats: ban nothing). Rules: bass ≥ 3 hits (funk bass is busy), kick never on the clap.
- Host: `tests/run_dub_host.sh funk` (`-DFELUCCA_ID="FM-1_902"`). Firmware `--app funk`.
- Risk: low. Same accessors as HOUSE (`funk_groove_*`).

### SNOWCRASH – done (TECHNO clone, generator-mirrored)

- `snowcrash_data.h` written by `tools/gen_snowcrash_pack.py` → mirror the snippet in the script. `SNOWCRASH_SLAB`,
  32-step, lead-gate genre (`SNOWCRASH_SLAB_LEAD_GATE`, `_LEAD_GATE_CS`, `_LEAD_PITCH[16]`, `_LEAD_LEN[32]`,
  `_LEAD_BEND[32]`, `_LEAD_RATE`), ROOT/SCALE/SWING/KIT int8. Shares DSP and `IDM_PLATE_SEQ` with IDM (no change).
- Eras (enum CRASH, GLOSS, SUMER, STATIC, TIDE, CHAPEL; UI names …CRASH). BPM 70–100 by era, densities ~50 % on all
  lanes, nothing always-on: hard rules are tempo + scale + texture, not grid. Keep `must` 0 except a downbeat kick
  (0x00000001) if the era’s plates mostly have it; rules header: total hits per lane within the era’s curated
  min…max (texture density is the genre’s identity).
- Risk: low; check the lead pitch unit (semitone vs scale degree) before reusing TECHNO’s `intervals`.

### IDM – done

- `idm_data.h` hand-maintained (`tools/gen_idm_lead_riffs.py` is stale, do not run). `IDM_PIECE`, 32-step,
  lead-gate tables as SNOWCRASH (`IDM_PIECE_*`, pitch[16]). Era extras `IDM_ERA_DENS_NUM/FLOOR`, `IDM_ERA_BPM_ADJ`
  are era-level (no routing).
- Eras WARM, MELO, GLITCH, BREAK, DRILL, OBLIQ; BPM 90–113 (WARM/MELO), 110–149 (GLITCH/BREAK), 150–171 (DRILL),
  OBLIQ 90–172 (two tempo families: split by plate, pick the dominant one for the window).
- Curated “all” masks give the era signature: rim WARM/MELO 0x1CC10000, GLITCH/BREAK 0x080E0080, DRILL 0x920A0080,
  OBLIQ 0xC00800A2; hats similar. Use them as `must`; kick has no rule. Rules: DRILL hat/rim density high (≥ 12 hits),
  WARM/MELO lead gate sparse.
- Risk: medium – irregular grids; the cross-era test may need lower densities where eras differ wildly.

### HIPHOP – done

- `hiphop_data.h` hand-maintained, `HIPHOP_BEAT`, 32-step, lead-gate tables (`HIPHOP_BEAT_*`, pitch[16]),
  ROOT/SCALE/SWING/KIT/LEAD_RATE int8, `HIPHOP_KICK_PUNCH[era]`, `HIPHOP_BASS_WALK[era][k]`.
- Eras PARK, GOLD, BOOM, GRIME, TRAP, CLOUD. BPM PARK 92–108, GOLD 80–96, BOOM 78–94, GRIME 84–100 (140 half-time),
  TRAP 133–150, CLOUD 60–134 (split).
- Hard rules: boom-bap backbeat – snare/rim must 0x10101010 for PARK/GOLD/BOOM (curated all = 0x10100000; the full
  backbeat is the musical rule), kick on 1 (0x01010101 PARK–GRIME, TRAP 0x01000101, CLOUD 0x00010001) and **ban**
  kick on the snare steps; hats PARK–BOOM 0x44444444, GRIME 0xEEEEEEEE, TRAP 0x77777777 (rolls), CLOUD 0x0000AAAA.
  Swing is the identity (67–80 for the golden era): keep swing windows tight.
- Host: `tests/run_dub_host.sh hiphop` (`-DFELUCCA_ID="FM-1_910"`).
- Risk: medium – `must & ban` overlap is easy to get wrong here (kick ban vs snare must on different steps only).

### HEAVY_PSYCH – done

- `heavy_psych_data.h` hand + `gen_hp_lead_riffs.py` for lead tables. `HEAVY_PSYCH_TRAX`, 32-step, lead-gate genre;
  ROOT/SCALE/SWING/KIT are **uint8** tables (cast), LEAD_RATE int8 (range −8…14), pitch[16].
- Extra per-slot: `HEAVY_PSYCH_TRAX_LEAD_VOICE[slot]` (struct) via `heavy_psych_trax_lead_voice()` – route it to the
  landmark slot (`<genre>_gen.slot`) explicitly so a generated plate keeps a coherent lead voice; era tables
  `HEAVY_PSYCH_LEAD_SUSTAIN`, `HEAVY_PSYCH_KICK_PUNCH`, `HEAVY_PSYCH_LEAD_MELODY` are era-level.
- Eras STONER, DESERT, SPACE, DRONE, GAZE, DOOM; slow: BPM 54–106 overall (DRONE 54–70, DOOM 58–86).
- Hard rules: kick on each bar’s 1 (0x00010001), snare 0x10100000, hat quarters 0x11111111 and **ban** 0xAAAAAAAA
  (never odd 16ths); DRONE lead gate must 0x80008001 (curated). Rules: kick ≤ 6 hits (heavy = space), lead gate
  ≥ 4 hits (riff exists).
- Risk: medium – the lead voice routing and the uint8 casts; `gen_hp_lead_riffs.py --check` needs full history.

### DUB (default genre) – done

- `dub_data.h` hand-maintained; it is the `#else` genre (no `GENRE_` flag; host build without `-D`). Plate table
  `DUB_PLATES` (16-step), lane S = siren. Per-slot tables are lowercase **uint8**: `plate_roots`, `plate_scales`,
  `plate_swings`; **no** per-slot kit or rate table (`.kit = .rate = NULL`, kit comes from the era) –
  `pgen_era_stats.py` does not find these tables, read them by hand. Also `DUB_BASS_WALK[era][k]`, `DUB_PLATE_SEQ`.
- Eras KINGSTON, LOVERS, CHANNEL, BRISTOL, CROYDON, POSTDUB. BPM KINGSTON 62–84, LOVERS 54–76, CHANNEL 112–132,
  BRISTOL 86–134 and POSTDUB 54–133 (split families), CROYDON 124–140.
- Hard rules: one-drop for KINGSTON/LOVERS – kick must 0x0100 (beat 3), ban the curated never-steps (KINGSTON 0xEAEA,
  LOVERS 0xFEFE); CHANNEL steppers 0x0101 (+ density for 4/4); BRISTOL/POSTDUB kick 0x0001; CROYDON half-time:
  kick 0x0001, snare (rim) 0x0100. Siren stays rare (d ≤ 16).
- cfg: DUB has the oldest cfg history (`DUB_CFG_MAGIC_PREV` migration). The generic `pgen` tail fallback in
  `fm_app.c` handles it (load at full size, else at `offsetof(dub_cfg_t, pgen)`); `tests/host.c` does not persist
  settings across runs, so check the two magic paths by reading that code, not by a host run.
- Host: `tests/run_dub_host.sh dub`. Firmware `--app dub`.
- Risk: medium – default genre (every non-genre build path), missing kit/rate tables, split tempo families.

### JAZZ – done

- Files: `jazz_data.h` (routing), `jazz_gen.h`, `tests/pgen_rules_jazz.h`, `tests/pgen_golden/jazz.txt`, plus the
  JAZZ section of `fm_views_data.h` (own commit, see below).
- `jazz_data.h` hand + `gen_jazz_lead_riffs.py` (which rewrites the header – do not run it). `JAZZ_TRAX`, 32-step,
  lead-gate genre with **pitch[32]** (`pitch_n = 32`), LEAD_GATE_CS, ROOT/SCALE/SWING/KIT/LEAD_RATE int8.
- Lead pitch is a scale-legal semitone offset from root + 12, not a move: `jazz_gen_build()` snaps generated phrases
  (`views_live`) to the generated scale (`JGEN_SCALE_PC` mirrors `SCALE_SEMIS` in `fm_app.c`); curated phrases are
  copied untouched. The rules header checks all 32 pitches.
- `JAZZ_PLATE_SEQ[slot]` (vary/trig per lane, `fm_app.c`) and `JAZZ_BASS_WALK` follow the landmark slot.
- Views: the ROM JAZZ views were stale (a428d43 changed three gates without regenerating). Fixed by splicing only
  the `#elif defined(GENRE_JAZZ)` … `#elif defined(GENRE_IDM)` block of a scratch-copy `tools/gen_views.py` run
  into `fm_views_data.h` (commit “Regenerate the JAZZ section of fm_views_data.h”); the BASS block is hand-authored
  (KO BASS views), so `gen_views.py` never generates it: it copies the existing block verbatim on write and
  ignores it on `--check` (`PRESERVE`); `--check` passes. The JAZZ block is byte-identical to the generator’s output (views port 0 diffs).
- Eras SPIRIT, MODAL, MUTE, FUSION, ACID, BROKEN. BPM (full-tempo trax): SPIRIT 114–121, MODAL 128–135, MUTE
  116–123, FUSION 104–111, ACID 120–127, BROKEN 108–115; swing windows = slots 0–7, rate −4…3.
- Hard rules: ride = curated all as must, curated never as ban except one ghost slot per beat (SPIRIT/MODAL/FUSION
  0x88888888 skip note, MUTE 0x44444444 offbeat; ACID/BROKEN fully fixed); feathered kick and hi-hat foot (rim) =
  curated all, neither under the other’s figure, swing eras ban odd 16ths for the kick and rim extras; comping
  (chord) = curated all, never on the bar’s one (swing eras), ACID off the quarters; bass keeps its two most-used
  curated steps, lead one. Mask literals inside the mmio windows use `JGEN_M(lo, hi)`.
- Rules header: bass ≥ 2 hits, kick ≤ must + 4, chord ≤ must + 8, lead gate 1…16 hits, lead pitch in scale.

### POP – done

- `pop_data.h` written by `tools/gen_pop_pack.py` (reproduces byte-exactly) → mirror the snippet there.
  `POP_PLATE`, 32-step, lane S = siren (no lead gate table; the lead comes from melody tables).
- Many per-slot arrangement tables read directly outside the data header: `pop_harmony.h` (`POP_TRACK_CHORD_BARS`,
  `POP_TRACK_PROG_IX`, `POP_PLATE_ROOT/SCALE`, `POP_MELO_A/B`, `POP_BASS_STEP`), `pop_data.h` (`POP_LEAD_VARIANT`,
  `POP_PLATE_LEAD_TIMB`, `POP_PLATE_KIT`, …) and `fm_app.c` ~20451 (`POP_PLATE_ROOT/SCALE[plate]`). Route root/scale
  through accessors everywhere (the `fm_app.c` spot is POP-only code); harmony/melody/bass-step tables index with
  the landmark slot (`k`), which `<genre>_gen.slot` already is. `pop_arrange_events()` / `pop_lead_arrange_bar0()`
  take the plate index → they see the landmark’s arrangement: accepted.
- Root/scale must stay compatible with the landmark’s progression and melody (written for that key and mode). In
  `pop_gen_build()`, after `fm_pgen_build()`, overwrite `pop_gen.root` / `pop_gen.scale` with the landmark slot’s
  ROM values; the continuum then varies groove, sound and tempo while harmony follows the nearest landmark.
- Eras (UI) COLD, RADIO, CHROME, ITALO, BLUE, AFTER. BPM COLD 108–126, RADIO 100–124, CHROME 112–128, ITALO
  118–134, BLUE 76–98, AFTER 114–128.
- Hard rules: snare backbeat 0x10101010 (all but BLUE: 0x01000100), kick on 1 (0x00010001) and ITALO/AFTER 4/4
  0x11111111, hats COLD/CHROME/BLUE 0x55555555, ITALO/AFTER 0x44444444.
- Host: `tests/run_dub_host.sh pop` (`-DFELUCCA_ID="FM-1_901"`), plus `tests/measure_pop_lead.sh` if touched.
- Risk: hard – widest routing surface, harmony coupling, a shared `fm_app.c` edit.

### BASS – done (own phrase model)

- Files: `bass_data.h` (include point + routing + bake), `bass_gen.h`, `tests/pgen_rules_bass.h` (rules + extras),
  `tests/pgen_golden/bass.txt` (`0x6C7962B7` since the word names), two BASS-only `#if` spots in `fm_app.c` (cfg fallback, boot).
- What plays in KO BASS is not the plate mask but the genre-fader bake (`bass_genre_fader_bake()`): the fader picks
  one era line, whose 8-bar phrase (`BASS_LINE_BAR_GATE[8]`, `LEAD_PITCH[16]`, `LEAD_LEN[128]`, `LEAD_BEND[128]`)
  becomes the step lane. The plate sets sound and groove (bpm, density, model, voicing, FX, root/scale/swing).
- A BASS plate = sound genes from the shared `fm_pgen_build()` (genome rows per era from
  `tools/pgen_era_stats.py bass`; drums/chord/siren `d = 0`) + a phrase from `bass_phrase_build()` in
  `bass_gen.h`, built from the core primitives (noise, landmark weight, mask blend, gene-era borrowing) with
  BASS-only gene keys `0x100…` (never renumber). `pl.sub` = phrase bars 1|2, as in the ROM.
- Phrase genome per era (from the curated lines): per-step density of the base bar, chance that bars 2…8 redraw
  a step instead of repeating bar 1, long-note value pool, bend value on step 7. Pitch moves use one table for
  all eras (7 / 12 / −5). Hard rules (0 violations at w = 0): downbeat on every bar, never steps 1/5/9/15, 1…6 hits
  a bar (cap drops the least likely step), len ≠ 0 exactly on gates, long notes (9…11) only on steps 7/14, bends
  only on steps 7 (4/5 by era) and 15 (5/6), none on the last bar’s 15. bpm window, scale pool as in the core.
- Landmarks: w = 1 copies the ROM bars; w ≥ ½ takes the curated pitch/len/bend; bars hand over per step.
- Genre fader on the continuum: fader line `L` plays the plate at `n_L = n + (L − k) · FM_PGEN_GAP` (`k` = the
  plate’s landmark slot), i.e. the ten plates of the plate’s 80-click block at the same offset. On a landmark,
  and with the continuum off (artist era, unit tests), every fader line is the curated line; line `k` is the plate
  itself; the fader name is the generated name of `n_L` (curated name only on the landmark, w = 1).
- Views: BASS S2…S8 are the phrase bars 2…8 (bar-stack seq), so generated plates carry their own BASS views at
  runtime (`bass_gen_ph.bar[]`, routed through `bass_line_bar_step_bit()`), reproducing `BASS_LINE_BAR_GATE` at
  landmarks. The core’s gen_views.py-style runtime views are switched off for BASS (`views_live = 0`,
  `FM_PGEN_VIEWS_HAND`): the hand-authored `FM_VIEWS` BASS block stays untouched and answers for the nearest
  landmark (only reachable on an unforced step; KO BASS forces every step).
- `bass_gen.h` is included right after the clamp helpers in `bass_data.h`, not at the end: the routed accessors
  and the bake below need it. `tools/gen_bass_pack.py` is stale (it lacks the WIP bake fix and the routing) – do not
  run it; `bass_data.h` is hand-maintained now.
- cfg: the `pgen` tail sits after `bass_cfg_ext_t`. Fallback load at `offsetof(dub_cfg_t, pgen)` accepts only
  BAS4 (the BAS1 `MAGIC_PREV` layout differs and was never migrated). KO BASS boots like the other genres: `cfg_boot_random_song()`
  picks a random `n`, so the continuum is live from boot.
- Checks: `tests/pgen_check.sh bass` (landmarks incl. every fader line, phrase rules, phrase drift ~4 bits/click,
  phrase cross-era ~37 %, fader axis), `tests/pgen_dj_trace.sh bass`, `tests/bass_host_smoke.sh`, UI walk with
  `-DFELUCCA_ID="FM-1_904"`.

## Parallel plan

| Genre | Rating | Why |
| --- | --- | --- |
| FUNK | easy | HOUSE clone: 16-step, siren lane, hand-maintained header |
| SNOWCRASH | easy | TECHNO clone incl. generator mirroring; no grid rules |
| IDM | easy/medium | TECHNO-shaped tables; irregular grids, stale riff script |
| HIPHOP | medium | lead tables + strict backbeat/kick rules, swing identity |
| HEAVY_PSYCH | medium | uint8 tables, per-slot lead voice struct, slow tempi |
| DUB | medium | default genre, lowercase tables, no kit/rate table, split tempo families |
| JAZZ | medium/hard | ROM views (fixed), pitch[32], vary/trig |
| POP | hard | arrangement/harmony tables across files, root/scale coupling, `fm_app.c` edit |

All eight can run in parallel: each touches only its own data header (+ generator), its new `<genre>_gen.h`,
`tests/pgen_rules_<genre>.h` and `tests/pgen_golden/<genre>.txt`. Two exceptions need ordering at merge time: JAZZ’s
`fm_views_data.h` regeneration (conflicts with any other `fm_views_data.h` change, and with KO BASS work) and POP’s
`fm_app.c` edit (small, POP-only `#if` – rebase-safe). Use one worktree and branch per genre off `plate-gen-all`.

## Status after the merge

All eight branches merged `--no-ff` into `plate-gen-all` without conflicts (DUB first for the shared `name[9]` core
change). Locked goldens, unchanged by the merge: TECHNO ABCA1064, HOUSE 0DD283C5, DUB 52CEC248, FUNK 5F62703A,
SNOWCRASH 862DD686, IDM 536C4B1D, HIPHOP 283C5D05, HEAVY_PSYCH 5B0463F1, JAZZ 096C12E2, POP 529C3921. Host warning
counts equal the a428d43 baseline; `./build.sh` passes for every app; `pgen_dj_trace.sh` OK for every genre.

Fixes made at merge time:
- `mmio_check` reads 32-bit mask literals such as `0x00010001u` as register addresses. HEAVY_PSYCH and IDM genomes
  now write them as `lo | hi << 16` (like the ROM tables; JAZZ uses `JGEN_M`). Check new masks with `./build.sh`.
- IDM `dub_plate_groove_read()` now adds `IDM_ERA_BPM_ADJ` like `idm_piece_apply()`; before, a stop-commit or CUE
  glide landed the raw piece BPM.
- HEAVY_PSYCH SPACE plates (ROM FLARE/ECLIP and the genome pool) carry voicing 5 = `VOICE_N`. All voicing readers wrap
  with `% VOICE_N` (5 sounds as PWR5); the knob bar now wraps too. Pool left as is (ROM-faithful, golden unchanged).

BPM after an era change while playing stays at the old tempo by design (`dub_lock_groove_while_playing`, same as a
plate change): it commits to the plate BPM on stop, or glides there on CUE / the DJ CUEING turn.

## Plate names

Generated plates are named from a per-genre word list (replaced the old CVCVC consonant/vowel names).

- List: `<GENRE>_GEN_WORDS` in `<genre>_gen.h`, one packed string, words separated by `\0`, the empty word ends it;
  `.words` / `.word_n` in the def. Only the active genre’s list is compiled in. 170–200 words per genre, ≈ 1.2 KB
  flash each (image +1.15…1.36 KB per genre app).
- Word rules (enforced by `plate_gen_drift.c`): A–Z only, 3–8 letters (`name[9]`), no duplicates, never a curated
  plate name of the genre, even count. Content rules (by hand): fits the genre and its eras, real English or
  genre-common words, no trademarks, nothing vulgar or offensive, no drug words.
- Pick (`fm_pgen_name()`): keyed by the genre’s `FM_PGEN_G_NAME` gene key and `n`, never the era, so an era turn keeps
  the name. Even `n` use even list indices, odd `n` odd ones → neighbouring clicks never repeat. Each parity walks a
  hashed affine permutation of its half per block of `word_n / 2` picks, so a word does not return within a block
  (n−2 repeats only at block edges, ~1 per 10 000 clicks).
- The curated name shows only on the landmark itself (w = 1); the ±1 neighbours (w = ⅔, curated categorical genes)
  already carry a word, so every click shows a new name. BASS fader names follow the same rule.
- Display: names are drawn in the 8 px wave font at x = 4 of the status bar, followed by an optional `*` (user plate)
  and the era name; 8 letters + `*` + the longest era name (SNOWCRASH “GLOSSCRASH”) end at ≈ 164 px, clear of the
  page dots. Checked with host shots (TECHNO TERMINAL/AUTOBAHN, BASS BASEMENT, SNOWCRASH PIXELATE).
- Tests: `tests/pgen_check.sh <genre>` prints `names: <n> words …, 0 bad, 0 adjacent repeats` and ten samples.
  A name-only change must keep the pattern-only golden (`cc … -DPGEN_GOLDEN_NO_NAME …`, same as `pgen_check.sh`
  without the `PGEN_GOLDEN` define) identical before/after; then relock the full goldens.
- Goldens after the switch (old → new): TECHNO ABCA1064 → C82C1F7F, HOUSE 0DD283C5 → EA6C76D9, DUB 52CEC248 →
  BFC33960, FUNK 5F62703A → 862EA2DA, SNOWCRASH 862DD686 → E0218EB2, IDM 536C4B1D → 219F3355, HIPHOP 283C5D05 →
  E74A0449, HEAVY_PSYCH 5B0463F1 → 38375809, JAZZ 096C12E2 → 0616599E, POP 529C3921 → F9CD24CD, BASS 1E023F30 →
  6C7962B7; pattern-only hashes unchanged in all eleven.
