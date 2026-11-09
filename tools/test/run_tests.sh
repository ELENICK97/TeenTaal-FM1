#!/bin/sh
# Host regression tests (render hashes, CPU cost, numerical checks): tools/test/regress.c.
#   tools/test/run_tests.sh                    from the project's root
#   GOLDEN_UPDATE=1 tools/test/run_tests.sh    rewrite golden.txt: only for an intended change of the sound
#   BUDGET_UPDATE=1 tools/test/run_tests.sh    rewrite cpu_baseline.txt
set -e
cd "$(dirname "$0")/../.."
make -s build-host/regress
build-host/regress tools/test
