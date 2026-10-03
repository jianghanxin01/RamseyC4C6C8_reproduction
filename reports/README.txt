Validation records

release_validation.json records the RamseyC4C6C8 executable hashes, complete
reconstruction of all nine input formulas, checks of all nine archived RUP
certificates, and fresh solving and checking of all nine regenerated formulas.

The other JSON files are historical records supplied with the earlier archive:
- checker_all_nine.json: checks of the nine archived certificates.
- checker_negative_oracle_tests.json: invalid-trace tests and full-scan comparisons.
- inputs_all_nine.json: complete reconstruction of the nine archived inputs.
- inputs_mutation_tests.json and inputs_cli_tests.json: input and command validation.
- original_data_integrity.json: identities of the 18 archived data files.
- solver_original_all_nine.json: fresh proofs for the archived input order.
- solver_wrapper_tests.json: small-formula and solver-interface tests.
- full_regeneration.json: the earlier complete regeneration workflow.

Historical compiler details, executable hashes and timings describe their
original runs. Fresh runs create their own results/ directories. The release
archive contains the original CNF/RUP pairs and compact validation records;
generated duplicate formulas, traces and build intermediates are excluded.
