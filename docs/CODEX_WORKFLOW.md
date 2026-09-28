# Codex Workflow — Flipper Dongle

## Starting a session

1. Open/clone `stryginuv/Flipper_Dongle`.
2. Read `AGENTS.md` completely.
3. Read `STATUS.md`.
4. Read `tasks/current.md`.
5. Inspect the relevant branch before editing.

Do not begin implementation until these files are read.

## Branch policy

- `main` = v0.5.5 working baseline.
- `v0.5.5-first-test` = preserved first-test snapshot.
- `v0.5.6-test-candidate` = pairing race-fix candidate awaiting verification.

For new experimental work, create a new branch from the appropriate baseline/candidate.

Never overwrite the known-good baseline just to simplify an experiment.

## During work

- Keep patches project-local.
- Do not modify a global ESP-IDF installation.
- Preserve the known-good TinyUSB HID TX synchronization.
- Preserve keyboard/mouse behavior unless logs/tests prove it must change.
- Prefer targeted fixes over broad rewrites.
- If hardware information is uncertain, document uncertainty instead of inventing a pinout.
- For the T-Dongle clone, update `docs/hardware/T_DONGLE_CLONE.md` as facts become known.

## Validation

When the environment permits:

```powershell
idf.py set-target esp32s3
idf.py build
```

Hardware behavior cannot be marked verified from compilation alone.

Record:

- build result
- changed files
- expected hardware test
- actual hardware result once supplied by the user

## Finishing a task

Before ending a meaningful task:

1. Update `STATUS.md` if the project state changed.
2. Update `tasks/current.md` if priorities/progress changed.
3. Update README EN/RU for user-facing behavior.
4. Keep `CHANGELOG.md` aligned with version changes.
5. Commit with a clear version/task-oriented message.

## Immediate project priorities

1. Identify the non-original T-Dongle-style board.
2. Hardware-test v0.5.6 pairing/reset race fix.
3. Add a board abstraction without breaking Waveshare.
4. Create Debug (HID+CDC) and Release (HID-only) configurations.
5. Merge the old proven HID/DRIVE + microSD MSC path.
6. Add display UI only after core reliability and pinout validation.
