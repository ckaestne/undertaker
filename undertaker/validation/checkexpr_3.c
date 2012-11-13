/*
 * check-name: Ensure that the blacklist operates correctly
 * check-command: undertaker -v -j checkexpr -m coverage-wl.model -B coverage-wl.blacklist CONFIG_OFF
 * check-exit-value: 1
 * check-output-start
I: loaded 1 items to blacklist
I: loaded rsf model for coverage-wl
I: Using coverage-wl as primary model
I: Expression is NOT satisfiable
 * check-output-end
 */
