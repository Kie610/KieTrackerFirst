# Task: integrate token-efficient, cross-agent project handoff into this bootstrap repository

MUST inspect the repository before editing and adapt names/paths to its existing generator architecture. MUST preserve unrelated changes. MUST implement and test generated output, not only add documentation. MUST NOT push, merge, publish, or install globally without explicit authorization.

## Required generated-project architecture

Generate these repository-root files for every applicable new project:

1. `AGENTS.md`: canonical durable instructions shared by coding agents. Include only repository-wide invariants, source precedence, build/test commands, safety boundaries, truthfulness rules, and handoff-maintenance rules. Exclude transient branch state, completed-task history, long architecture descriptions derivable from code, and generic advice. Prefer <=100 lines and <=8 KiB.
2. `CLAUDE.md`: contain `@AGENTS.md`; append Claude-only rules only when required. Do not duplicate shared instructions. This import is required because Claude Code reads `CLAUDE.md`, while Codex reads `AGENTS.md`.
3. `HANDOFF.agent.md`: compact mutable state using the exact schema below. It is read on takeover tasks, not imported unconditionally into startup instructions. Prefer <=4 KiB; remove stale items when updating.
4. `HANDOFF.md`: short index linking `HANDOFF.agent.md`, `AGENTS.md`, and detailed history/procedures. Do not place the full handoff in this root file.
5. `docs/handoff-history.md`: optional append-only or snapshot history for rationale that is not needed at every takeover. Existing detailed handoff content MUST be preserved here when migrating a project.

## Exact `HANDOFF.agent.md` schema

Use these headings in this order; omit empty optional entries, never rename semantic keys:

```md
# Agent handoff v1
updated: YYYY-MM-DD
repo: OWNER/NAME or local identifier
work_branch: current branch
upstream: remote/ref@commit or none
base: ref@commit or unknown
goal: one line

## State
complete:
- delivered capability
verified-YYYY-MM-DD:
- exact environment/command scope + PASS/FAIL + counts; distinguish compile from runtime
not-run:
- test not executed

## Decisions
C:
- confirmed fact with evidence
A:
- provisional assumption
U:
- unresolved fact

## Next
1. highest-priority executable task
blocked-by:
- exact external input, hardware, permission, or none

## Paths
- purpose: `path`

## Resume protocol
1. Read `AGENTS.md` and this file.
2. Verify live Git branch, HEAD, status, and relevant external state; live state overrides recorded metadata.
3. Read only paths required for the first unblocked task.
4. Run the smallest relevant baseline checks.
5. Start the highest-priority unblocked item.
6. Update this file with evidence; never report unexecuted verification as PASS.
```

## Bootstrap integration requirements

- Find all project templates, init commands, generators, snapshots, fixtures, documentation, and tests that define generated repository contents.
- Add or update templates so generated projects contain the architecture above.
- Parameterize repository identity, goal, commands, invariants, and protected paths from existing bootstrap inputs. Use explicit TODO/unknown markers when data is unavailable; never invent a passing test or hardware result.
- If the bootstrap supports project types, keep shared rules minimal and place type-specific rules in nested instruction files or optional template fragments rather than bloating the root.
- Provide a deterministic handoff updater or documented generator path if the repository already has a scripting convention. Prefer scripts for schema validation and mechanical updates.
- Validate that `CLAUDE.md` imports `AGENTS.md`, all linked paths exist, required headings occur once and in order, no transient state is duplicated into `AGENTS.md`, and generated files respect configured size limits.
- Add regression tests that create a temporary sample project and assert exact generated files/content. Include a migration fixture when the repository already supports upgrades.
- Update only the repository's existing user documentation surface. Do not add redundant README/quick-reference/changelog files.

## Acceptance

- A clean bootstrap run produces all required files with no unresolved template syntax.
- Codex receives durable rules from `AGENTS.md`; Claude Code receives the same bytes through `@AGENTS.md` plus any explicit Claude-only suffix.
- Takeover requires reading the compact state, while history remains on demand.
- Tests prove template generation, link integrity, schema order, truth labels, and size bounds.
- Final report lists changed paths, exact test commands/results, remaining unknowns, and confirms whether any external write occurred.
