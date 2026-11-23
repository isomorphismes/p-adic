# Exact sampled Witt prefixes

The [repository sketch](https://github.com/isomorphismes/p-adic/blob/5fe710f864569f2b2ce8a07e8c5612939b20f645/README.md#L11-L27)
samples digits from 0 through (p-1) for (p=2,3,5). Its
[browser code](https://github.com/isomorphismes/p-adic/blob/5fe710f864569f2b2ce8a07e8c5612939b20f645/index.html#L216-L235)
evaluates the ghost polynomials over the integers using those representatives.
This example covers prefixes of one, two, or three known coordinates.

The Idriç sketch gives the prime a finite witness, digits an upper bound, and
both coordinate and ghost prefixes a known depth. Evaluating an exact coordinate
prefix preserves its depth in the result. Two projections discard the last
coordinate or the last ghost at corresponding depths. The equality witnesses
state that evaluating after either coordinate projection yields the same prefix
as projecting after evaluation. Applying both equalities gives the three-to-one
case. The Agda checker expresses the same finite claim with bounded digits,
dependent pairs for the three depths, and one general truncation relation.
For fixed earlier digits, both proofs also express the last-digit sensitivity
as an equality without subtraction. Adding (p^2c') to (w_2(a,b,c))
gives the same total as adding (p^2c) to (w_2(a,b,c')).

The polynomials are (w_0=a), (w_1=a^p+pb), and
(w_2=a^{p^2}+pb^p+p^2c). The underlying arithmetic uses nonnegative arbitrary-
precision values, sufficient for these integer representatives. Formula
evaluation and proofs are pure. The Idriç host fixture executable prints the
input prime and digits, followed by the observed ghosts, for comparison with a
plain TSV file. No effects are required for the proof modules.

A digit equal to the prime is invalid. Requesting the third ghost of a
two-coordinate prefix is ill typed. Explicitly extending that prefix by zero is
valid and yields a new, known ghost. The coefficients and exponents retain
their exact integer values. The proofs concern the representative integer
polynomials at these digits and depths.

## Reproduce the checks

- Agda 2.6.3 using its builtins. From the `agda` directory,
  check `WittFixtures.agda` with
  `agda --safe --without-K --no-libraries -i . WittFixtures.agda`.
  The module imports all proofs and checks the eight rows in `fixtures.tsv` as
  definitional equalities.
- Idriç compiler revision
  `2bce7677f513d711875e978bd6f5ad28b9e54d58`, built with Chez Scheme 10.4.1.
  From the `idric` directory, use the built
  compiler with `--cg chez --build witt-fixtures.ipkg` and compare the
  complete output of `build/exec/witt-fixtures` to `fixtures.tsv`.
  For an uninstalled compiler, point `IDRIS2_PATH` at its built prelude and
  base TTC directories and `IDRIS2_DATA` at `_/support`. Point
  `IDRIS2_LIBS` and `LD_LIBRARY_PATH` at `_/support/c` and put its
  bootstrapped Chez executable on `PATH`.
- Check `DigitTooLarge` and `MissingThirdGhost` separately in both languages
  with their corresponding flags (Idriç `--check`). Require a type mismatch
  at the invalid value or ghost projection. In a clean copy of each core,
  changing only the final coefficient in `level_two` or `third-ghost`
  from (p^2) to (p) must fail to check the unchanged balance proof.
