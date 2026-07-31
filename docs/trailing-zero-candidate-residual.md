# Draft handoff: trailing-zero candidate residual

Status: **not ironclad — do not merge or describe as a general performance
improvement.**

This note records the evidence and remaining work for candidate
`52a1f4f15e972fa30c38be4dc27faca8c550c295` (parent
`308e266`). It is a handoff for a later repair-and-measurement session, not a
request to change the optimization in this draft.

## Supported narrow result

The candidate trims a sufficiently long trailing-zero suffix before the
fallback digit comparison. The sealed paired measurements support the intended
long-suffix workloads:

| workload | comparison | candidate |
| --- | ---: | ---: |
| direct `digit_comp` fallback | 1444.9 ns/digit | 200.4 ns/digit |
| public `fast_float::from_chars` fallback route | 2788.5 ns/parse | 1525.7 ns/parse |

The public workload is not merely an internal helper benchmark: it reaches
`from_chars`, its slow path, and then `digit_comp`; its verification gate checks
that route. Functional checks and differential parsing probes found no semantic
difference for the tested inputs.

## Blocking residual

`trim_trailing_zeros` scans the suffix before it decides whether the suffix has
the required 16 zeros. Inputs with 0 through 15 zeros therefore pay the scan
but keep the old digit span and receive no trimming benefit.

An alternating paired replay through the public `from_chars` fallback path
showed that this is a reproducible regression, not measurement noise:

| trailing zeros | result versus comparison |
| ---: | --- |
| 0 | candidate 3.34 ns/parse slower |
| 15 | candidate 7.43 ns/parse slower |
| 16 | candidate 1.86 ns/parse faster |
| 64 | candidate 23.57 ns/parse faster |

The cliff is exactly at the 16-zero cutoff. The direct 0--32 suffix sweep and
the public boundary replay agree on that shape.

## Why the current benchmark is insufficient

The current benchmark samples only suffix lengths 64, 700, 769, and 4096. All
of those are already past the cutoff, so it demonstrates the narrow
long-suffix benefit but cannot establish a no-regression claim for the public
parser. It is therefore not sufficient evidence for a general optimization.

## Required next session

Do not change the implementation as part of this draft. The smallest credible
repair/proof plan is:

1. Add a bounded, at-most-16-code-unit threshold probe before the unbounded
   reverse suffix scan, so short suffixes do not pay a full scan before the
   candidate can reject them.
2. Run a sealed, alternating paired sweep through both the direct fallback and
   public `from_chars` paths for suffixes 0--17 and the existing long-suffix
   shapes.
3. Preserve the route and correctness gates, and set an explicit no-regression
   criterion for every 0--15 point before evaluating the long-suffix wins.

Until that work passes, this candidate is not ready for merge, release notes,
or an external performance-evidence package. The long-suffix result remains a
useful hypothesis; it is not an approval-grade general parser improvement.
