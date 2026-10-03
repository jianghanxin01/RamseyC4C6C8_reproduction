# RamseyRup C++14 checker

Run the supplied binary from the package root with:

```text
bin\RamseyRup.exe INPUT.cnf PROOF.rup
```

`VERIFIED` and exit 0 mean every added proof clause passed reverse unit
propagation (RUP) and the last addition was explicitly empty. Invalid proof
steps or a missing final empty addition return 1. Malformed input, file errors,
allocation failures or incorrect command syntax return 2.

The input must have exactly one `p cnf V C` header, exactly C zero-terminated
clauses and variables in `1..V`. CNF clauses may span lines or share a line.
Proof additions and `d` deletion records must each occupy one line. Blank
lines and lines whose first token is `c` are ignored. The terminator must be
exactly `0`. Overflow, out-of-range literals, unterminated records and extra
proof tokens are rejected. Deletion records are fully parsed, then ignored.
Variables are dynamically allocated, without a 31/63-variable mask limit.
The format is textual RUP, not arbitrary RAT proofs or clause-ID formats.

For each addition C, the program assigns all literals of C false and applies
unit propagation to the existing database. A conflict establishes that C is a
logical consequence. Retaining deleted clauses is sound because all retained
additions have already been justified from the original formula. The final
empty-clause addition therefore establishes unsatisfiability of the original
CNF. An initially empty input clause still requires an explicit empty addition
in the proof. The whole proof is parsed, including any trailing deletions.

Duplicate literals are normalized so they cannot create two watched
occurrences of the same literal. Tautologies can be omitted from storage.
Each nonunit clause has two distinct watched positions. When a watch becomes
false, it is replaced by a nonfalse literal, or the other watch satisfies the
clause, becomes unit, or exposes a conflict. Watch positions may persist when
assignments are reset between proof additions. The implementation checks for
stale watch references and records counts and timings.

Run `build_release.cmd` in this source directory for an individual build, or
the package's `BUILD_ALL.cmd` for an isolated source-copy build. The compiler
flags include `/std:c++14 /O2 /MT /EHsc /W4`. The portable C++14 source can also
be built with `g++ -std=c++14 -O2 RamseyRup.cpp -o RamseyRup`; a non-Windows
build was not part of the reported validation.

All nine archived proofs were freshly checked by this executable. There were
37 fixed parser/negative/edge tests and 240 comparisons with an independently
written full-clause-scan oracle. See `../../reports/checker_all_nine.json` and
`../../reports/checker_negative_oracle_tests.json`. These tests and the
soundness explanation are not a formal verification of the implementation.

The C++ implementation was prepared with OpenAI Codex assistance, based on
the algorithm in the project's earlier `RupCheck.cs`, with stronger parsing
and final-empty checks. No SAT-solver code was incorporated. The earlier file
had no explicit licence; this adaptation does not assert a new copyright owner
or grant rights in third-party work. See the package's provenance/licensing note.
