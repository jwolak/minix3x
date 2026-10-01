#!/bin/sh
set -eu

repo_root=$(git rev-parse --show-toplevel)
cd "$repo_root"

chmod +x .githooks/post-commit
git config --local core.hooksPath .githooks
printf 'Git hooks installed from %s/.githooks\n' "$repo_root"