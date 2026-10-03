# Windows verification and reproduction commands

The command and PowerShell files are at the package root, alongside `bin/`,
`data/`, `src/`, `docs/` and `reports/`. Keep `VERIFY_ALL.cmd`,
`REPRODUCE_ALL.cmd`, `RUN_PIPELINE.cmd`, `CHECK_HASHES.cmd`,
`CHECK_HASHES.ps1` and `BUILD_ALL.cmd` in that location. The executables belong
in `bin/`, while `data/` retains the stable nine CNF/RUP relative paths. These launchers
need Windows' built-in command processor and Windows PowerShell; they need no
Python, third-party Python packages, SAT installation, or Visual Studio to run
the supplied executables. All graph reconstruction, solving and RUP checking
are carried out by the supplied native C++14 executables.

## Check the archived proof

Double-click `VERIFY_ALL.cmd`. It first runs the complete nine-case graph input
audit. Only if that returns success does it check the archived RUP proofs,
one by one. A failed input audit or proof check stops further work and produces
no overall success claim. The window waits for a key at the end.

For unattended use, open a command prompt in the package root and run:

```text
VERIFY_ALL.cmd --no-pause
```

A quick selected proof check still audits all nine inputs:

```text
VERIFY_ALL.cmd --no-pause --case B-leafK4
```

## Generate new proofs

Double-click `REPRODUCE_ALL.cmd`, or run:

```text
REPRODUCE_ALL.cmd --no-pause
REPRODUCE_ALL.cmd --no-pause --case B-leafK4
```

This reconstructs all nine CNFs into a fresh directory and compares their full
clause multisets with the distributed data. For each selected case it then runs
`RamseySolve.exe GENERATED.cnf NEW.rup`. Solver exit status **20** means UNSAT,
but is only an intermediate result. The command accepts a case only after
`RamseyRup.exe GENERATED.cnf NEW.rup` independently returns **0**. A solver
returning 0, 10 (SAT), 2 (error), or any other status fails the reproduction
immediately. A solver reporting UNSAT with an invalid proof also fails.

Generated files are flat `ID.cnf` and `ID.rup` names. Their variable/clause
semantics and multiplicities match the audited archive; literal/clause ordering,
proof traces and byte hashes can differ from the distributed files. The new
proof is always checked against the **newly generated CNF**, not an archived
CNF. Original data and distributed executables are not overwritten.

## Use binaries rebuilt from source

After the bundle's build command has put freshly compiled programs into
`rebuilt_bin/`, run:

```text
VERIFY_ALL.cmd --no-pause --bin rebuilt_bin
REPRODUCE_ALL.cmd --no-pause --case B-leafK4 --bin rebuilt_bin
```

`--bin DIR` defaults to `bin`. Relative executable-directory paths are resolved
against the directory containing these command files, even if they were
launched from another working directory. Absolute paths are also supported.
This option lets rebuilt executables be tested without replacing the identified
distributed binaries or changing their checksum manifest.

The exact IDs are `48T1`, `48T2`, `H10-I3`, `H10-K2_K1`, `H10-P3`,
`B-root-1`, `B-root-2`, `B-leafK4`, and `B-C9`. Omitting `--case` selects
all nine. The option may be supplied once. `--help` lists the syntax.
Exit status 0 means the requested workflow succeeded; 1 means a computation
or proof check failed; 2 means an argument, installation or startup error.

## Find the output

Every launch creates a fresh folder such as:

```text
results/verify_20260923_120000_123_4567/
results/reproduce_20260923_120000_123_4567/
```

The name combines local date/time through milliseconds and a random suffix.
The launcher checks that the folder does not already exist. Each contains:

- `summary.txt`: selected cases, executable directory, case results, final status.
- `inputs.log` and `inputs.json`: all-nine input reconstruction/audit details.
- `ID.rup.log`: the independent checker's counts, timing and result.
- For reproduction, `ID.solve.log` and `generated/ID.cnf`, `generated/ID.rup`.

A failed run retains its partial files and logs for diagnosis. To try again,
launch the same command; a new folder is created. Complete regeneration of
all proofs may take substantially longer than checking archived proofs.

## Optional byte-identity check

The separate checksum command uses only PowerShell's built-in `Get-FileHash`:

```text
CHECK_HASHES.cmd --no-pause
```

It expects `SHA256SUMS.txt` in the package root, using conventional lines
`64_HEX_DIGITS` followed by two spaces and a relative file path (or one space
and `*` before the path). It rejects missing files, mismatches, empty manifests,
duplicate paths and paths outside the bundle. Blank lines and lines starting
with `#` are allowed. Generated results and rebuilt binaries should not be in
the immutable distribution manifest. This checksum step identifies exact bytes;
it is separate from the graph reconstruction and proof checks.

## Validation records

Historical validation results are retained in `reports/` with their recorded
executable identities. Checks performed for this release and the newly
compiled solver's identity are recorded separately in
`reports/release_validation.json`. A complete verification or reproduction
run must finish with all nine input audits and nine certificate checks passed,
with exit code 0. A previous report or a selected-case trial is not a substitute
for that complete run.
