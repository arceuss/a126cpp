#pragma once

// Deterministic scene capture: loads or generates a world, renders it through
// the production client path, and writes one PNG at a requested resolution.
//
// This is the framebuffer half of the parity harness. Comparing two backends
// means rendering the same scene twice and diffing the images, which needs a
// capture that is reproducible without a person driving the game.
//
// See docs/portable/parity-testing.md.
int runSceneCapture(int argc, char **argv, int firstArgument);
