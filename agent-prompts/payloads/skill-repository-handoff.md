# Task: add a reusable agent-handoff skill to this skills repository

MUST inspect and follow this repository's existing skill scaffolding, naming, metadata, validation, and marketplace conventions before editing. MUST preserve unrelated work. MUST validate executable scripts by running them. MUST NOT push, publish, install globally, or modify external repositories without explicit authorization.

## Skill objective and trigger contract

Create or update a skill named `agent-handoff` unless an existing semantically equivalent skill requires extension instead of duplication. The description MUST trigger for requests to create, compress, migrate, validate, or consume development handoffs; configure `AGENTS.md`/`CLAUDE.md`; reduce takeover context; or transfer project state between Codex, Claude Code, or another coding agent. Put every trigger condition in the frontmatter description because the body loads only after triggering.

`SKILL.md` frontmatter MUST contain only:

```yaml
---
name: agent-handoff
description: <complete function and trigger contract>
---
```

## Required skill structure

```text
agent-handoff/
├── SKILL.md
├── agents/
│   └── openai.yaml
├── scripts/
│   ├── handoff_codec.py
│   └── validate_handoff.py
├── references/
│   └── handoff-schema.md
└── assets/
    ├── AGENTS.md.template
    ├── CLAUDE.md.template
    ├── HANDOFF.md.template
    └── HANDOFF.agent.md.template
```

Do not add README, installation guide, quick reference, changelog, or duplicate explanatory files inside the skill.

## `SKILL.md` body contract

Write imperative instructions and keep the body concise, preferably <250 lines and always <500 lines. Assume the agent already understands Git and Markdown. Include only:

1. Inspect repository instructions, Git state, existing handoffs, generators, and user-owned changes before mutation.
2. Classify information into durable rules, current state, on-demand procedure/reference, and history.
3. Make `AGENTS.md` the canonical shared durable rules file. Make `CLAUDE.md` import it with `@AGENTS.md`, then add only necessary Claude-specific rules.
4. Create/update the compact `HANDOFF.agent.md` using the reference schema and `C` confirmed / `A` assumed / `U` unresolved truth labels.
5. Keep root `HANDOFF.md` as a short index; move existing detailed prose to `docs/handoff-history.md` without losing information.
6. Never convert compilation into runtime/hardware PASS; verify drift-prone branch, HEAD, worktree, external state, and current test results.
7. Use bundled scripts for deterministic compression and validation; do not reimplement codecs ad hoc.
8. Run repository-specific validation, then report changed paths, exact results, blockers, and external writes.

Route all schema details to `references/handoff-schema.md`. Route reusable file bodies to `assets/`. Route deterministic processing to `scripts/`. Avoid duplicating the same rule across `SKILL.md`, references, and assets.

## Reference schema requirements

Define `Agent handoff v1` with ordered sections: identity metadata; `State` (`complete`, dated `verified`, `not-run`); `Decisions` (`C`, `A`, `U`); ordered `Next`; `blocked-by`; `Paths`; `Resume protocol`. Require exact evidence scopes, distinguish compile/runtime/hardware, prefer live Git over recorded metadata, and define soft budgets of 100 lines/8 KiB for root durable instructions and 4 KiB for compact handoff state.

## Script contracts

`handoff_codec.py`:
- Python standard library only.
- Encode UTF-8 payload bytes deterministically with gzip level 9 and `mtime=0`, then Base64.
- Emit an `AX1` envelope containing encoding identifiers, uncompressed/compressed byte lengths, SHA-256, payload, and a decode/verify/execute directive.
- Decode only supported `AX1` envelopes; reject invalid Base64, length mismatch, SHA-256 mismatch, invalid UTF-8, unknown algorithms, or extra/missing envelope fields.
- Provide `encode`, `decode`, and `verify` CLI commands; return nonzero on failure.

`validate_handoff.py`:
- Python standard library only.
- Validate required files, `CLAUDE.md` import, unique ordered schema headings, required `C/A/U` labels, link/path existence when applicable, and configured byte/line budgets.
- Never infer PASS values or rewrite user data during validation.
- Provide actionable errors and nonzero exit on failure.

## Asset contracts

- `AGENTS.md.template`: concise placeholders for scope, source priority, invariants, safety/truth rules, exact commands, and handoff maintenance; no transient branch/test state.
- `CLAUDE.md.template`: exactly `@AGENTS.md` plus optional clearly delimited Claude-only placeholder.
- `HANDOFF.md.template`: short index only.
- `HANDOFF.agent.md.template`: exact reference schema with explicit `unknown`/`not-run` defaults and no invented evidence.

## Metadata and validation

- Generate `agents/openai.yaml` using this repository's approved generator. Derive human-facing `display_name`, `short_description`, and `default_prompt` from the final skill. Include no optional interface fields unless repository policy or user input requires them.
- Run the repository's initializer for a new skill when available; otherwise match existing structure exactly.
- Run the repository's quick validator and all tests for both scripts.
- Test codec round trips on ASCII and multibyte UTF-8; assert deterministic byte-identical envelopes; corrupt payload/hash/length and assert rejection.
- Test handoff validation with one valid fixture and failures for heading order, missing import, false/ambiguous verification field, missing path, and size overflow.
- If the repository has a catalog/marketplace/index, update the established source of truth and its generated outputs; do not invent a new registry.

## Acceptance

- The skill triggers from both Japanese and English handoff requests through a concise but complete description.
- Progressive disclosure holds: metadata always loaded; lean workflow loads on trigger; schema only on demand; templates copied without being loaded as prose; scripts execute deterministically.
- Codec round-trip reproduces exact original bytes and detects corruption using length plus SHA-256.
- Validator prevents lossy or misleading handoffs.
- Final report identifies changed paths, validator/test commands and results, any skipped forward test, remaining decisions, and confirms whether external state changed.
