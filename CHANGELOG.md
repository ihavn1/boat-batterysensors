# Changelog

## Unreleased - 2025-11-30

- Persist Amp-hour (Ah) immediately when receiving an SK PUT for Ah. This complements
  the existing periodic/delta-based persistence to ensure manual resets/updates
  are stored to NVS immediately.
- Removed all remaining Serial debug output from `src/ah_integrator.cpp` and
  `src/battery_helper.cpp`.

Notes:
- Ah is still periodically persisted (every 10 minutes by default) if it
  changes by at least 0.5 Ah to reduce NVS wear.
