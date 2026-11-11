#!/usr/bin/env bash
# Test runner for the scheduler.
#
# Usage:
#   ./run_tests.sh                    run every case in tests/cases/
#   ./run_tests.sh rr_mixed_q3_cs1    run only the named case(s)
#   ./run_tests.sh unit               build and run the unit tests only
#   ./run_tests.sh --bless CASE       run CASE and save its output as the
#                                     expected output (check it by hand first!)
#   ./run_tests.sh --random N SEED ALGO [Q CS]
#                                     generate N processes with test_generator
#                                     (seed SEED) and run sanity checks only.
#                                     ALGO 1..4; Q and CS only for RR (2).
#
# A case is a folder tests/cases/<name>/ containing:
#   processes.txt        required (tab separated, as in the project document)
#   resources.txt        optional (an empty one with a header is used if absent)
#   input                required: what is typed to process_generator.out
#                        (algorithm, then quantum and context switch for RR)
#   expected.log         optional: compared with scheduler.log using diff
#   expected.perf        optional: compared with scheduler.perf using diff
# Without expected files, only sanity checks run.
#
# TIMEOUT sets the seconds allowed per case.

set -u
cd "$(dirname "$0")" || exit 2
ROOT=$(pwd)
CASES_DIR="$ROOT/tests/cases"
WORK_DIR="$ROOT/tests/.work"
TIMEOUT=${TIMEOUT:-600}
ME=$(id -un)

pass=0; fail=0
red()   { printf '\033[31m%s\033[0m\n' "$*"; }
green() { printf '\033[32m%s\033[0m\n' "$*"; }

# Count IPC objects owned by this user.
ipc_count() { ipcs -q -m -s 2>/dev/null | awk -v u="$ME" '$3 == u' | wc -l; }

ensure_built() {
    if [ ! -x process_generator.out ] || [ ! -x scheduler.out ] ||
       [ ! -x process.out ] || [ ! -x clk.out ]; then
        make build >/dev/null || { red "BUILD FAILED (a non-compiling project gets ZERO)"; exit 2; }
    fi
}

# Run sanity checks.
check_sanity() {
    local procs=$1 bad=0
    if [ ! -f scheduler.log ]; then echo "  missing scheduler.log"; return 1; fi
    if [ ! -f scheduler.perf ]; then echo "  missing scheduler.perf"; return 1; fi

    if [ "$(head -n 1 scheduler.log)" != "#At time x process y state arr w total z remain y wait k" ]; then
        echo "  scheduler.log: wrong or missing header line"; bad=1
    fi
    # Check log line format.
    if grep -v '^#' scheduler.log | grep -Evq \
        '^At time [0-9]+ process [0-9]+ (started|resumed|stopped) arr [0-9]+ total [0-9]+ remain [0-9]+ wait [0-9]+$|^At time [0-9]+ process [0-9]+ finished arr [0-9]+ total [0-9]+ remain 0 wait [0-9]+ TA [0-9]+ WTA [0-9]+\.[0-9]{2}$'; then
        echo "  scheduler.log: badly formatted line(s):"
        grep -v '^#' scheduler.log | grep -Ev \
        '^At time [0-9]+ process [0-9]+ (started|resumed|stopped) arr [0-9]+ total [0-9]+ remain [0-9]+ wait [0-9]+$|^At time [0-9]+ process [0-9]+ finished arr [0-9]+ total [0-9]+ remain 0 wait [0-9]+ TA [0-9]+ WTA [0-9]+\.[0-9]{2}$' | head -3 | sed 's/^/    /'
        bad=1
    fi
    # Check that every process finished once.
    local want got dup
    want=$(grep -Ev '^[[:space:]]*(#|$)' "$procs" | awk -F'\t' '{print $1}' | sort -n)
    # Field 5 is the process ID and field 6 is the state.
    got=$(awk '$6 == "finished" {print $5}' scheduler.log | sort -n)
    dup=$(echo "$got" | uniq -d)
    if [ "$want" != "$got" ]; then
        echo "  scheduler.log: finished set differs from processes.txt"
        [ -n "$dup" ] && echo "    finished twice: $(echo $dup)"
        bad=1
    fi
    # Check performance output.
    if [ "$(wc -l < scheduler.perf)" -ne 4 ] ||
       ! sed -n 1p scheduler.perf | grep -Eq '^CPU utilization = [0-9]+%$' ||
       ! sed -n 2p scheduler.perf | grep -Eq '^Avg WTA = [0-9]+\.[0-9]{2}$' ||
       ! sed -n 3p scheduler.perf | grep -Eq '^Avg Waiting = [0-9]+$' ||
       ! sed -n 4p scheduler.perf | grep -Eq '^Std WTA = [0-9]+\.[0-9]{2}$'; then
        echo "  scheduler.perf: must be exactly the four formatted lines"; bad=1
    fi
    return $bad
}

# Run one simulation.
run_simulation() {
    local dir=$1
    rm -f scheduler.log scheduler.perf
    cp "$dir/processes.txt" processes.txt
    if [ -f "$dir/resources.txt" ]; then
        cp "$dir/resources.txt" resources.txt
    else
        printf '#pid\tresourceid\tcapturetime\treleasetime\n' > resources.txt
    fi

    # Use a separate process group for cleanup.
    setsid ./process_generator.out < "$dir/input" > "$WORK_DIR/stdout.txt" 2>&1 &
    local pgid=$! waited=0
    while kill -0 "$pgid" 2>/dev/null; do
        if [ "$waited" -ge "$TIMEOUT" ]; then
            echo "  TIMEOUT after ${TIMEOUT}s"
            kill -INT -- "-$pgid" 2>/dev/null; sleep 2
            kill -KILL -- "-$pgid" 2>/dev/null
            wait "$pgid" 2>/dev/null
            return 1
        fi
        sleep 1; waited=$((waited + 1))
    done
    wait "$pgid"
    local status=$?
    sleep 1   # let the clock and children finish their own cleanup
    # SIGINT is the expected cleanup status.
    if [ "$status" -ne 0 ] && [ "$status" -ne 130 ]; then
        echo "  process_generator exited with status $status (crash?). Last output:"
        tail -n 5 "$WORK_DIR/stdout.txt" | sed 's/^/    /'
        return 1
    fi
    if pgrep -g "$pgid" >/dev/null 2>&1; then
        echo "  processes still alive after the run:"
        pgrep -g "$pgid" -l | sed 's/^/    /'
        kill -KILL -- "-$pgid" 2>/dev/null
        return 1
    fi
    return 0
}

# Run one case and print PASS/FAIL.
run_case() {
    local name=$1 bless=${2:-0} dir="$CASES_DIR/$1" ok=1
    if [ ! -f "$dir/processes.txt" ] || [ ! -f "$dir/input" ]; then
        red "SKIP $name (needs processes.txt and input)"; return
    fi
    echo "--- $name"
    local ipc_before ipc_after
    ipc_before=$(ipc_count)

    local ran=1
    run_simulation "$dir" || { ok=0; ran=0; }

    ipc_after=$(ipc_count)
    if [ "$ipc_after" -gt "$ipc_before" ]; then
        echo "  IPC leak: $((ipc_after - ipc_before)) object(s) left (see ipcs)"; ok=0
    fi

    # Stop after a crash or timeout.
    if [ "$ran" -eq 0 ]; then
        red "FAIL $name"; fail=$((fail + 1)); return
    fi

    check_sanity "$dir/processes.txt" || ok=0

    if [ "$bless" -eq 1 ] && [ "$ok" -eq 1 ]; then
        cp scheduler.log "$dir/expected.log"
        cp scheduler.perf "$dir/expected.perf"
        echo "  saved expected.log / expected.perf - CHECK THEM BY HAND before committing"
    else
        for f in log perf; do
            if [ -f "$dir/expected.$f" ]; then
                if ! diff -u "$dir/expected.$f" "scheduler.$f" > "$WORK_DIR/diff.txt"; then
                    echo "  scheduler.$f differs from expected.$f:"
                    sed 's/^/    /' "$WORK_DIR/diff.txt" | head -40
                    ok=0
                fi
            fi
        done
        [ -f "$dir/expected.log" ] || echo "  (no expected output: sanity checks only)"
    fi

    if [ "$ok" -eq 1 ]; then green "PASS $name"; pass=$((pass + 1))
    else red "FAIL $name"; fail=$((fail + 1)); fi
}

mkdir -p "$WORK_DIR"

case "${1:-}" in
unit)
    make unit-tests
    exit $?
    ;;
--bless)
    [ $# -eq 2 ] || { echo "usage: $0 --bless CASE"; exit 2; }
    ensure_built
    run_case "$2" 1
    ;;
--random)
    [ $# -ge 4 ] || { echo "usage: $0 --random N SEED ALGO [Q CS]"; exit 2; }
    ensure_built
    name="random_n$2_s$3_a$4"
    dir="$CASES_DIR/.$name"; mkdir -p "$dir"
    ( cd "$dir" && printf '%s\n3\n' "$2" | "$ROOT/test_generator.out" "$3" >/dev/null )
    if [ "$4" -eq 2 ]; then printf '2\n%s\n%s\n' "${5:-3}" "${6:-1}" > "$dir/input"
    else printf '%s\n' "$4" > "$dir/input"; fi
    run_case ".$name"
    ;;
*)
    ensure_built
    if [ $# -gt 0 ]; then names=("$@")
    else names=(); for d in "$CASES_DIR"/*/; do names+=("$(basename "$d")"); done; fi
    for n in "${names[@]}"; do run_case "$n"; done
    ;;
esac

echo
echo "passed: $pass  failed: $fail"
[ "$fail" -eq 0 ]
