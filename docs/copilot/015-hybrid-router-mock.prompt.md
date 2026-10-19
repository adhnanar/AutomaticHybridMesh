# Copilot prompt 015 – hybrid router mock review/continuation

Software-only. Do not flash, reset, power or transmit. Do not modify root CMake/prj.conf, default devicetree or the deployed BLE app.

1. Read AGENTS.md and docs/TASK015-HYBRID-ROUTER-MOCK-ACCEPTANCE.md.
2. Review src/features/hybrid_router and tests/hybrid_router for logic errors.
3. On a host where it works, run `west twister -T tests/hybrid_router -p native_sim -v` and record real results in the TASK015 doc and docs/PROGRESS.md.
4. Do not claim C execution from the Python model in tests/hybrid_router_sim.
