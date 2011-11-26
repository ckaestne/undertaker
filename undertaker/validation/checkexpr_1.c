/*
 * check-name: find interesting items for symbol
 * check-command: undertaker -j checkexpr -m ../kconfig-dumps/models/x86.model 'CONFIG_SMP'
 * check-output-start
I: loaded rsf model for x86
I: Using x86 as primary model
CONFIG_SMP=y
 * check-output-end
 */
