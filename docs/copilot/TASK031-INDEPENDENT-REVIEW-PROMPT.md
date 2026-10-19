# Task031 independent review prompt

Copy-ready prompt:

> Independently review Task031 in `C:\zephyrproject\hybrid-mesh-poc` without
> editing files, staging/committing, running hardware/USB/RF actions, flashing,
> or accessing the unrelated `C:\zephyrproject\hybrid-mesh` project.
>
> Read `AGENTS.md`, `docs/TASK030-BLE-ANDROID-POC-GAP-ANALYSIS.md`,
> `docs/TASK030-INDEPENDENT-REVIEW.md`, and
> `docs/TASK031-BLE-ANDROID-UX-FOUNDATION.md`. Review all Task031 code and
> config changes, especially the default-off Kconfig behavior, runtime GAP
> name and scan-response sizing, FICR-to-node-ID byte order, error handling,
> and C tests. Confirm the existing Task018 no-echo feature was reused rather
> than duplicated, Task012 artifacts/configuration remain untouched, and no
> claim suggests Kit1 currently runs BLE or that A→B→C is possible on the
> deployed arrangement.
>
> Record only evidence-backed findings, including whether the sandbox-blocked
> CMake/test commands need rerun after policy access is granted. Do not treat
> existing Task012/Task018 artifacts as a build of Task031. Return a concise
> review verdict and any required surgical fixes; leave all code and hardware
> unchanged.
