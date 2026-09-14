# Collision decompilation validation

`CCollision::ProcessColModels` (Windows `0x4185C0`) and its collision helpers were
reviewed against the stripped Windows executable and the Android symbol build. The
Windows implementation is treated as authoritative.

## Corrected behavior

- `ProcessSphereSphere` now follows the Windows order: distance is square-rooted,
  B's radius is subtracted before the float store, and the stored touch distance is
  squared for the limit check. The old radius-sum precheck incorrectly rejected
  exact/tangent cases.
- `ProcessSphereBox` preserves piece fields, accepts the Windows tangency (`<=`),
  and keeps the cancellation-sensitive distance calculation wide until the same
  float square-root boundary.
- `ProcessLineSphere` was restored to the Windows quadratic path. In particular,
  roots at zero are accepted, roots inside the sphere are rejected, the point
  stores match the x87 spill pattern, and the helper no longer writes fields that
  Windows leaves untouched.
- `ProcessLineBox` now handles a start point strictly inside the box, including
  the inside-box normal/depth helper and Windows crossing counts. Boundary-plane
  tests retain strict interior checks and the box piece fields.
- `ProcessLineTriangle` uses the stored plane, accepts an endpoint on the plane,
  preserves the Windows subtraction order, and avoids the old sign-bit/epsilon
  behavior that differed at signed zero and near-parallel lines.
- Sphere/triangle processing now uses the Windows stored-plane basis and region
  selection. Zero-distance contacts are valid (the vector normalizer supplies the
  binary's fallback). The Android `TestSphereTriangle` is a different SAT
  broad-phase algorithm; it was not substituted for the Windows path.
- Disk projection uses the Windows unclamped radicand and ordered NaN-rejecting
  comparisons. Candidate and triangle limits in `ProcessColModels` match the
  executable's 599-entry behavior and its disk/line contact ordering.

## Windows/Android divergence

Android's sphere/triangle test recomputes separating axes, while Windows uses the
stored compressed plane and a 2-D basis. This can differ for quantized planes and
degenerate triangles; Windows behavior is retained. Android also uses AArch64
single-precision instructions where Windows keeps x87 temporaries. The source uses
`double` only for cancellation-sensitive scalar expressions and deliberately keeps
the binary's explicit float stores; this improves the decision boundaries without
claiming bit-for-bit 80-bit emulation.

No compilation or runtime tests were performed, as requested.
