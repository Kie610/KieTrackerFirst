# Perfboard schematic: KiCAD build spec

Executable instruction for building the schematic in
`hardware/tracker-perfboard/` from `docs/perfboard-netlist.md`. Everything
electrically required is reproduced here, so this file can be handed to an agent
as-is. Where this file and `perfboard-netlist.md` disagree, the netlist wins —
except for the one deliberate deviation recorded under "Nets".

## Preconditions

**Run this in an interactive `claude` session.** In a non-interactive/SDK
session, Konnect's `load_toolset` succeeds server-side but the loaded tools never
become callable: `get_active_toolsets` reports them active while every call
returns `No such tool available`, and `server_stats` shows the calls never
reached the server. The harness does not apply the MCP `tools/list_changed`
notification, so the callable set stays frozen at the 18 startup tools
(`project` + `config`). Confirmed twice, including from a fresh subagent.

**Put `kicad-cli` on PATH.** KiCAD 10.0.5 is installed but `kicad-cli.exe` is not
on PATH, so `snapshot_project` fails with `Failed to spawn kicad-cli`. Without
this, ERC, SVG export, and annotation all fail even once the tools are callable.

```
setx PATH "%PATH%;C:\Program Files\KiCad\10.0\bin"
```

Verify with `kicad-cli version` (expect `10.0.5`) in a new shell before starting.

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
| C1 | 100nF | Sense node to GND, ADC settling |
| BT1 | Protected 1S Li-ion | To `BAT+` / `BAT-` on the XIAO back side |
| SW2 | Slide switch | In the `BAT+` line — see the naming note under "Nets" |

`R2` / `R3` may later both become 220k to cut idle current from about 21 uA to
about 9.5 uA. The firmware multiplier depends on the ratio, not the absolute
value, so that swap needs no schematic change beyond the two Value fields.

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

Stock libraries for the rest: `Device:R` (R1/R2/R3), `Device:C` (C1),
`Switch:SW_Push` (SW1), `Device:Battery_Cell` (BT1), `Switch:SW_SPST` (SW2).
Substitute a close equivalent if a lib_id does not resolve, and record which.

## Nets

| Net | Connections |
| --- | --- |
| `+3V3` | U1 `3V3` — U2 `3V3` — R1 |
| `GND` | U1 `GND` — U2 `GND` — SW1 — R3 — C1 — BT1 `-` |
| `SDA` | U1 `D4`/GPIO5 — U2 `SDA` |
| `SCL` | U1 `D5`/GPIO6 — U2 `SCL` |
| `IMU_INT1` | U1 `D9`/GPIO8 — U2 `INT1` |
| `PWR_BTN` | U1 `D8`/GPIO7 — R1 — SW1 |
| `BAT_SENSE` | U1 `D0`/GPIO1 — R2 — R3 — C1 |
| `BAT+` | SW2 — U1 `BAT+` pad — R2 |
| `BAT+_RAW` | BT1 `+` — SW2 |

Use `power:+3V3` and `power:GND` power symbols for those two rails; net labels
for the signal nets.

**Deliberate deviation — `BAT+_RAW`.** The netlist writes this as a single net,
`BAT+ | BT1 + — (SW2) — XIAO BAT+ — R2`, treating both sides of the optional
switch as one node. A schematic cannot represent that, so the cell side is named
`BAT+_RAW` and the load side keeps `BAT+`. `SW2` is marked "optional, undecided"
in the netlist; it is drawn fitted. If it is later dropped, merge the two nets
back into `BAT+`.

`IMU_INT1` is wired but unused by the current firmware. It is held on `D9`/GPIO8
because that pad is an RTC GPIO and sits next to the `D8`/GPIO7 button pad, which
keeps a later wake-on-motion path available without rewiring.

## Deliberately unconnected

Add no-connect flags so ERC is clean:

- U2 `SDO`, `CS` — held high by the module's own 4.7k pull-ups. The top-side
  `SDO` solder jumper shorts `SDO` to GND and selects `0x6A`; leave it open.
- U2 `OSDO`, `OCS`, `SCX`, `SDX`, `INT2` — OIS auxiliary SPI, unused.
- U1 `5V` (VBUS), `D1`, `D2`, `D6`, `D7`, `D9`, `D10` — unused.

**Do not add external I2C pull-ups.** The LSM6DSV module already carries 4.7k on
`SDA` and `SCL`.

## Layout

Readable A4 sheet:

- U1 left, U2 right, with the I2C pair running between them
- SW1 / R1 button circuit in its own area
- R2 / R3 / C1 divider in its own area
- BT1 and SW2 at the bottom

Note for whoever lays this out physically, from the netlist: `3V3` and `GND` are
on the XIAO's right column while `SDA` and `SCL` are on its left, so power and
I2C leave the module on opposite edges. The `D8` run to `SW1` crosses the `3V3`
and `GND` returns — plan those two as insulated jumpers. See
`docs/wiring-xiao-lsm6dsv.svg`.

## Verify

1. Run ERC. Fix what it reports; record any warning left unfixed, with the reason.
2. Export an SVG or PNG of the sheet into the project directory so the result can
   be eyeballed.
3. Confirm the net list in the schematic matches the table above exactly — nine
   nets, no extras.

## Report

State: symbols created and where the `.kicad_sym` landed, any lib_id
substitutions, the verbatim ERC result, and any deviation from this spec.

## Out of scope

- PCB layout — perfboard build, no board file work
- Footprint assignment
- Component placement on the ElectroCookie board. The pad-row spacing of the XIAO
  and of the LSM6DSV module is still unmeasured; `perfboard-netlist.md` records
  the legal row pairs once calipers have been applied.
