# TouchComp Project Notes

## Purpose

This project is forked from a working master-slave control codebase to create an independent haptic-master compensation platform. The immediate goal is to validate position-dependent gravity and mechanism-bias compensation on the OpenHaptics device without requiring the UR robot, CAN motor, needle mechanism, or force sensor.

The project intentionally starts from the validated OpenHaptics initialization, scheduler callback, position acquisition, button handling, and force-output path in the source project. This reduces hardware integration risk and preserves a known-good baseline.

## Scope Of The First Version

The first usable version shall run with only the haptic device connected and provide these modes:

1. Baseline mode: reads and displays haptic state, outputs zero compensation force.
2. Sampling mode: records a balanced position and its manually selected or measured compensation force.
3. Compensation mode: estimates compensation force from the current position and applies it to the haptic device.
4. Verification mode: records repeatable trials with compensation disabled and enabled.

The following subsystems are out of scope for this project version and must not be started by default:

- UR RTDE connection and servo control.
- USB-CAN, motor, needle, and M812x force-sensor communication.
- RCM mapping, puncture-depth control, and remote shared control.

## Planned Architecture

The existing `Touch` class remains the real-time device adapter. Its high-frequency callback continues to read device state and output the most recently computed force.

A lightweight compensation controller will replace the current full-system `Compute` startup path. It owns the experiment mode, receives haptic state from `Touch`, and calls `Touch::setForce()` with:

```text
F_output = clamp(low_pass(F_compensation(position)), force_limit)
```

Initial model options should be implemented in increasing complexity:

1. Constant force bias for device-path verification.
2. Nearest-neighbor lookup over sampled workspace points.
3. Inverse-distance weighting interpolation over the same points.
4. Optional RBF or small neural-network model only after the above methods are validated.

The compensation force must be limited and smoothly filtered. A failed lookup, an empty dataset, or a position outside the calibrated workspace must safely fall back to zero force.

## Data Files

Use UTF-8 CSV files with header rows. Keep raw data and derived results separate.

- `data/calibration_samples.csv`: position, requested compensation force, operator note, timestamp.
- `data/trial_log.csv`: timestamp, position, velocity, compensation-enabled flag, raw model force, filtered force, output force, trial ID.
- `data/model_metadata.json`: workspace bounds, interpolation method, filter parameters, force limit, and dataset version.

No result should be presented as a physical experiment unless it originates from a recorded trial. Simulations and parameter sweeps must be labelled as offline analysis.

## Demonstrable Deliverables

The first demonstration should contain real haptic-device video and recorded plots:

1. A baseline-versus-compensation video at several representative positions, with the compensation state and force vector/value visible.
2. A 3D scatter plot of sampled workspace positions, coloured by force magnitude.
3. Force-component slices or heatmaps for the calibrated workspace.
4. Position drift and applied-force curves for repeated compensation-off and compensation-on trials.
5. A coverage report showing calibrated points, rejected points, and interpolation validity bounds.

## Implementation Order

1. Decouple application startup from UR, Needle, CAN, and sensor construction while retaining the known-good `Touch` callback path.
2. Add an experiment controller with explicit baseline, sampling, compensation, and verification modes.
3. Add stable CSV logging before collecting calibration data.
4. Implement constant-force output and safety limits; verify force direction with a low magnitude.
5. Implement calibration-data loading and nearest-neighbor compensation.
6. Add inverse-distance interpolation, filtering, and out-of-workspace handling.
7. Record controlled trials and generate plots from the recorded logs.

## Safety Rules

- Begin every force test at a conservative force limit and keep a direct software disable control available.
- Do not run this project while the original source process is using the same haptic device.
- Do not connect or command UR, CAN, motor, needle, or force-sensor hardware from this project during the first version.
- Treat a device error, missing calibration dataset, invalid numeric value, or controller shutdown as a zero-force condition.

## Relationship To The Source Project

TouchComp is an independent validation project. A later integration into the source project may reuse the calibrated compensation model and `Touch` force path, but that integration is explicitly deferred until standalone behavior, force limits, and repeatability are verified.
