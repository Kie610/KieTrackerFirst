# Perfboard schematic: KiCAD build spec

Executable instruction for building the schematic in
`hardware/tracker-perfboard/` from `docs/perfboard-netlist.md`. Everything
electrically required is reproduced here, so this file can be handed to an agent
as-is. Where this file and `perfboard-netlist.md` disagree, the netlist wins —
except for the one deliberate deviation recorded under "Nets".

## Status

**Built and verified on 2026-08-04.** This file is kept as the record of intent
and as the procedure for rebuilding. See `HANDOFF.agent.md` for the evidence.

## Preconditions

**Run the build from a child `claude.exe` process, not from the Claude Code app
session.** The app harness never makes dynamically loaded Konnect toolsets
callable: `load_toolset` returns `tools_added: N` and `get_active_toolsets`
reports them active, yet every call returns `No such tool available` and
`server_stats` shows the call never reached the server, because the harness does
not apply the MCP `tools/list_changed` notification. The callable set stays
frozen at the 18 startup tools (`project` + `config`). The CLI binary does apply
it, and `-p` being headless makes no difference — the earlier "needs an
interactive session" diagnosis was wrong.

```
claude -p "<prompt>" --mcp-config <config.json> --allowedTools "mcp__konnect,Read,Glob,Grep" --disallowedTools "Task,Bash,Write,Edit"
```

The binary is at `C:\Users\Kie\.local\bin\claude.exe` and is not on PATH. Konnect
is not in any `.claude.json` `mcpServers`, so the child needs `--mcp-config`
naming
`C:\Users\Kie\Documents\KiCad\10.0\3rdparty\plugins\com_github_mixelpixx_konnect\bin\konnect.exe`
with no arguments.

**Tell the child not to delegate.** Dynamically loaded toolsets do not reach
subagents either, so an agent that hands the work to `kicad-schematic-build-agent`
stalls and retries. Disallowing `Task` enforces this.

**Konnect cannot place custom-library symbols.** `add_schematic_component`,
`batch_place_components`, and `replace_component` only resolve KiCad's bundled
libraries and never consult `sym-lib-table`, failing with `Library '<name>' not
found in the installed KiCAD symbol libraries`. Place `U1` and `U2` by hand in
the KiCAD GUI first; wiring, no-connects, ERC, and export all work afterwards,
since `batch_get_schematic_pin_locations` reads the schematic's embedded
`lib_symbols`. In the GUI's symbol chooser, browse the `tracker-perfboard`
library tree rather than searching — searching `XIAO` also returns the global
`Seeed_XIAO` parts, whose `XIAO-ESP32-S3-SMD` (24 pins) and `XIAO-ESP32-S3-DIP`
(14 pins, no battery pins) carry SAMD21 pin names and were both placed by mistake
during this build. The correct symbol shows 16 pins including `BAT+` and `BAT-`.

**Close KiCAD before running the build.** A GUI holding the schematic open keeps
a stale copy in memory and will overwrite the tool's work if it saves.

**`kicad-cli` is already on PATH** as of 2026-08-04 — `C:\Program Files\KiCad\10.0\bin`
is in the User `PATH` and `kicad-cli version` reports `10.0.5`. Re-verify with
`kicad-cli version` before starting; if it is missing, restore it with:

```
setx PATH "%PATH%;C:\Program Files\KiCad\10.0\bin"
```

**All writes go through Konnect MCP tools.** Never edit `.kicad_sch`,
`.kicad_pro`, or `.kicad_sym` as text — they carry UUIDs and cross-references
that text edits corrupt. If the tools are unreachable, stop and report; do not
hand-write S-expressions.

## Project

| | |
| --- | --- |
| Project | `hardware/tracker-perfboard/xiao-lsm6dsv-tracker.kicad_pro` |
| Schematic | `hardware/tracker-perfboard/xiao-lsm6dsv-tracker.kicad_sch` |

The schematic is an empty skeleton — `(kicad_sch (version 20250610) (generator
"konnect") ... (lib_symbols))` with no components. This is a perfboard build:
**do not assign footprints and do not touch the PCB file.**

## Parts

| Ref | Part | Value / note |
| --- | --- | --- |
| U1 | Seeed XIAO ESP32-S3 | 14 castellated pads, 2.54 mm pitch, plus back-side `BAT+` / `BAT-` |
| U2 | LSM6DSV module (BOOTH, rev `5265159A-P1-241122`) | 12 pins |
| SW1 | Momentary button, normally open | Logic input only; never in series with the cell |
| R1 | 10k | GPIO7 pull-up to 3V3 |
| R2 | 100k | Battery divider, high side (`BAT+` to sense node) |
| R3 | 100k | Battery divider, low side (sense node to GND) |
| BT1 | Protected 1S Li-ion | To `BAT+` / `BAT-` on the XIAO back side |

`SW2`, the optional slide switch in the `BAT+` line, was **dropped by owner
decision on 2026-08-04** and is no longer in the schematic. `R2` / `R3` are fixed
at **100k** by the same decision; the 220k alternative that would have cut idle
draw from about 21 uA to about 9.5 uA is declined.

`C1`, the 100 nF ADC settling capacitor across `R3`, was **also dropped on
2026-08-04** and is no longer in the schematic. Its job moved into firmware:
`src/batterymonitor.cpp` discards one conversion and takes the median of
`BATTERY_ADC_SAMPLES` (15) readings on the ESP32 `BAT_EXTERNAL` path. See
`docs/perfboard-netlist.md` for the reasoning and its limits. The part count is
therefore **seven**, not nine.

Set the Value field on every passive to the value in this table.

## Symbols

`U1` and `U2` are not in the standard KiCAD libraries. Search the registered
libraries first in case something suitable already exists; otherwise create them
with `create_symbol` into a project-local `.kicad_sym` inside
`hardware/tracker-perfboard/`, and register it with `register_symbol_library` at
**project** scope so the project stays self-contained.

`U1` — XIAO ESP32-S3, 16 pins. Standard XIAO pad order:

- Left column, pads 1-7: `D0/GPIO1`, `D1/GPIO2`, `D2/GPIO3`, `D3/GPIO4`, `D4/GPIO5`, `D5/GPIO6`, `D6/GPIO43`
- Right column, pads 8-14: `D7/GPIO44`, `D8/GPIO7`, `D9/GPIO8`, `D10/GPIO9`, `3V3`, `GND`, `5V`
- Back side: `BAT+`, `BAT-`

Electrical types: `3V3` / `GND` / `5V` as power_in, GPIOs bidirectional.

`U2` — LSM6DSV module, 12 pins: `3V3`, `GND`, `SDA`, `SCL`, `INT1`, `INT2`,
`SDO`, `CS`, `OSDO`, `OCS`, `SCX`, `SDX`.

Stock libraries for the rest: `Device:R` (R1/R2/R3),
`Switch:SW_Push` (SW1), `Device:Battery_Cell` (BT1).
Substitute a close equivalent if a lib_id does not resolve, and record which.

## Nets

| Net | Connections |
| --- | --- |
| `+3V3` | U1 `3V3` — U2 `3V3` — R1 |
| `GND` | U1 `GND` — U2 `GND` — SW1 — R3 — BT1 `-` |
| `SDA` | U1 `D4`/GPIO5 — U2 `SDA` |
| `SCL` | U1 `D5`/GPIO6 — U2 `SCL` |
| `IMU_INT1` | U1 `D10`/GPIO9 (pin 11) — U2 `INT1` |
| `PWR_BTN` | U1 `D8`/GPIO7 — R1 — SW1 |
| `BAT_SENSE` | U1 `D0`/GPIO1 — R2 — R3 |
| `BAT+` | BT1 `+` — U1 `BAT+` pad — R2 |

Use `power:+3V3` and `power:GND` power symbols for those two rails; net labels
for the signal nets.

**Resolved — the former `BAT+_RAW`.** While `SW2` was still undecided this file
split `BAT+` in two, naming the cell side `BAT+_RAW`, because a schematic cannot
draw both sides of an optional switch as one node. With `SW2` dropped on
2026-08-04 the two merged back into a single `BAT+`, matching
`perfboard-netlist.md` exactly. There is no longer any deviation between the two
documents, and the net count is **eight**, not nine.

`IMU_INT1` is wired but unused by the current firmware.

## Deliberately unconnected

Add no-connect flags so ERC is clean:

- U2 `SDO`, `CS` — held high by the module's own 4.7k pull-ups. The top-side
  `SDO` solder jumper shorts `SDO` to GND and selects `0x6A`; leave it open.
- U2 `OSDO`, `OCS`, `SCX`, `SDX`, `INT2` — OIS auxiliary SPI, unused.
- U1 `5V` (VBUS), `D1`, `D2`, `D3`, `D6`, `D7`, `D9` — unused. `D3` / GPIO4 held
  `IMU_INT1` until the owner moved it to `D10` / GPIO9 on 2026-08-04.

**Do not add external I2C pull-ups.** The LSM6DSV module already carries 4.7k on
`SDA` and `SCL`.

## Layout

Readable A4 sheet:

- U1 left, U2 right, with the I2C pair running between them
- SW1 / R1 button circuit in its own area
- R2 / R3 divider in its own area
- BT1 at the bottom

Note for whoever lays this out physically, from the netlist: `3V3` and `GND` are
on the XIAO's right column while `SDA` and `SCL` are on its left, so power and
I2C leave the module on opposite edges. The `D8` run to `SW1` crosses the `3V3`
and `GND` returns — plan those two as insulated jumpers. See
`docs/wiring-xiao-lsm6dsv.svg`.

## Verify

1. Run ERC. Fix what it reports; record any warning left unfixed, with the reason.
   Two `power:PWR_FLAG` symbols on the `+3V3` and `GND` rails are required — the
   cell is not a power source KiCAD recognises, so without them ERC reports two
   `Input Power pin not driven by any Output Power pins` errors. They are
   annotation, not fitted parts.
2. Export an SVG or PNG of the sheet into the project directory so the result can
   be eyeballed.
3. Confirm the net list in the schematic matches the table above exactly — eight
   nets, no extras.

Do not take the building agent's word for any of this. Check it independently
with `kicad-cli`, which is on PATH:

```
kicad-cli sch erc --severity-all --exit-code-violations hardware/tracker-perfboard/xiao-lsm6dsv-tracker.kicad_sch
kicad-cli sch export netlist --format kicadsexpr --output net.net hardware/tracker-perfboard/xiao-lsm6dsv-tracker.kicad_sch
```

The netlist lists every net with its member pins by reference and pin number, so
the eight nets and the fifteen single-pin unconnected nets can be compared
against the tables above directly.

## Report

State: symbols created and where the `.kicad_sym` landed, any lib_id
substitutions, the verbatim ERC result, and any deviation from this spec.

## Out of scope

- PCB layout — perfboard build, no board file work
- Footprint assignment
- Component placement on the ElectroCookie board. The pad-row spacing of the XIAO
  and of the LSM6DSV module is still unmeasured; `perfboard-netlist.md` records
  the legal row pairs once calipers have been applied.
