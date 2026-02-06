#pragma once

// -------- DIP setting --------
// Pulse count per 10 baht from DIP switch (1,2,5,10)
#define NK77_PULSE_PER_10 2

// Tolerance for expected pulses per bill (used for early complete)
#define NK77_PULSE_TOL 1

// Tolerance for expected pulses per bill (used for early complete)
#define NK77_PULSE_TOL 1

// -------- Hardware --------
#define NK77_ACTIVE_LOW 1

// -------- Pulse filter --------
// Minimum pulses before treating as a valid candidate
#define MIN_VALID_PULSE ((NK77_PULSE_PER_10 * 2) - 1)
#define MAX_PULSE_LIMIT ((NK77_PULSE_PER_10 * 10) + 4)

#define MIN_PULSE_GAP_MS 30
#define PULSE_TIMEOUT_MS 200

// -------- Calibration --------
#define CALIB_MIN_SAMPLE 5
#define CALIB_SIGMA 3.0f
