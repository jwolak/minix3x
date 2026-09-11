#!/usr/bin/env bash
set -euo pipefail

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <path-to-file>" >&2
    exit 1
fi

file="$1"

if [ ! -f "$file" ]; then
    echo "File not found: $file" >&2
    exit 1
fi

if ! command -v clang-format >/dev/null 2>&1; then
    echo "clang-format is not installed or not in PATH." >&2
    exit 127
fi

clang-format -style=file -i -- "$file"
