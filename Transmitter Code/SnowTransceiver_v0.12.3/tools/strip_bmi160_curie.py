"""
strip_bmi160_curie.py
=====================

PlatformIO pre-build hook.  The hanyazou/BMI160-Arduino library ships a
file `internal/ss_spi_101.cpp` that is specific to the Arduino 101 (Intel
Curie) board.  It #includes the Curie toolchain header `eiaextensions.h`
which doesn't exist on the ESP32 toolchain, so the build fails.

The Arduino IDE skips that file silently on non-Curie targets, but
PlatformIO's Library Dependency Finder compiles every .cpp it sees.  This
hook deletes the offending file (and emits a `.stripped` marker so we
don't keep deleting it on every build).

The script is idempotent: it does nothing if the library is not present
yet, and it does nothing if the file has already been stripped.
"""

import glob
import os

Import("env")  # noqa: F821 -- provided by PlatformIO's SCons environment

PROJECT_DIR  = env["PROJECT_DIR"]                                  # noqa: F821
LIBDEPS_DIR  = os.path.join(PROJECT_DIR, ".pio", "libdeps")
TARGETS = [
    # Files that break the ESP32 build because they assume the Arduino 101
    # (Intel Curie) toolchain.  Add more entries here if other libraries
    # later have similar issues.
    "BMI160-Arduino/internal/ss_spi_101.cpp",
]


def strip_curie_files(*args, **kwargs):
    if not os.path.isdir(LIBDEPS_DIR):
        return                                                # not cloned yet

    for env_dir in os.listdir(LIBDEPS_DIR):
        env_path = os.path.join(LIBDEPS_DIR, env_dir)
        if not os.path.isdir(env_path):
            continue
        for relpath in TARGETS:
            full = os.path.join(env_path, relpath)
            marker = full + ".stripped"
            if os.path.isfile(full):
                try:
                    os.remove(full)
                    open(marker, "w").close()
                    print("[strip_bmi160_curie] removed:", full)
                except OSError as e:
                    print("[strip_bmi160_curie] WARN: could not remove",
                          full, "->", e)


# Run immediately at script-load (which is BEFORE the LDF scans libraries
# for source files), and also re-run before the buildprog target as a
# belt-and-braces safety net.
strip_curie_files()
env.AddPreAction("buildprog", strip_curie_files)                   # noqa: F821
