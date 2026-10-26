# MINIX3x Developers Guide

This guide collects practical information for developing and contributing to
this MINIX 3 repository. It complements the existing manual pages and
repository-specific instructions in the main [README](../../README.md).

## Contents

- [Getting started](#getting-started)
- [Repository layout](#repository-layout)
- [Building MINIX](#building-minix)
- [Running and testing](#running-and-testing)
- [Code style and contributions](#code-style-and-contributions)
- [Further reading](#further-reading)

## Getting started

Start from a clean checkout and build outside the source tree. For an i386
release build, run this from the repository root:

```bash
./build.sh -j "$(nproc)" -m i386 -O ../obj.i386 -D ../obj.i386/destdir.i386 -U -u release
```

The build produces the release files needed to create a bootable disk or CD
image. The main [README](../../README.md) describes image creation and running
MINIX in QEMU or Hyper-V.

## Repository layout

The repository combines MINIX-specific code with a large NetBSD-derived source
tree. Some useful starting points are:

- `minix/` — MINIX-specific kernel, servers, drivers, libraries, and utilities.
- `sys/` — NetBSD-derived kernel and low-level system source.
- `tests/` — regression tests for userland and libraries.
- `docs/` — repository documentation and maintenance notes.
- `tools/` — development helpers, including the C formatting script.
- `releasetools/` — scripts for producing bootable images.

When changing an existing component, first look for its source directory,
Makefile, manual pages, and nearby tests. Keep changes scoped to the component
and update its tests or documentation when appropriate.

## Building MINIX

Use the repository's `build.sh` wrapper for a full build. Keep object and
destination directories outside the source tree, and use consistent values for
`-O` and `-D` across build and image-generation commands.

For a bootable hard-disk image, after the release build run:

```bash
ARCH=i386 OBJ=../obj.i386 releasetools/x86_hdimage.sh
```

For a bootable CD image, run:

```bash
ARCH=i386 OBJ=../obj.i386 releasetools/x86_cdimage.sh
```

These scripts produce `minix_x86.img` and `minix_x86.iso` in the repository
root, respectively. See the main [README](../../README.md) for QEMU and
Hyper-V launch examples.

## Running and testing

Run the built image in an emulator to verify changes that affect booting,
drivers, services, or system behavior. The top-level build includes the
`tests/` tree; which tests are built depends on the configured test options.

Tests for library routines and system calls should follow the source-tree
layout under `tests/`; tests for userland utilities should likewise be placed
under the corresponding `tests/` subtree. See [`tests/README`](../../tests/README)
for the existing conventions.

## Code style and contributions

For C changes, use the repository's [.clang-format](../../.clang-format) setup
and format only files you modify:

```bash
./tools/format-file.sh path/to/file.c
```

The equivalent Make target is `make format FILE=path/to/file.c`. The main
[README](../../README.md#contributing--style-guide) also documents the Git hook
and VS Code formatting behavior.

Before submitting a change, review the diff, run the relevant tests, and
include any necessary documentation updates.

## Further reading

- [Main README](../../README.md) — build, image creation, and emulator usage.
- [`tests/README`](../../tests/README) — regression-test organization.
- [`docs/UPDATING`](../UPDATING) — historical upgrade notes.
- [`docs/profiling.txt`](../profiling.txt) — profiling-related note.
