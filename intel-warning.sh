#!/bin/sh

# This message is given on my laptop when Vulkan is run
# MESA-INTEL: warning: Performance support disabled, consider sysctl dev.i915.perf_stream_paranoid=0

# This script runs the given command:
sudo sysctl dev.i915.perf_stream_paranoid=0
