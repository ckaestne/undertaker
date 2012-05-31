/*
 * check-name: Ensure that the blacklist operates correctly
 * check-command: undertaker -j checkexpr -m coverage_wl.model -B coverage_wl.blacklist CONFIG_OFF
 * check-exit-value: 1
 * check-output-start
I: loaded 1 items to blacklist
I: loaded rsf model for coverage_wl.model
I: Using coverage_wl.model as primary model
I: Expression is NOT satisfiable
 * check-output-end
 */
