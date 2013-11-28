#ifdef CONFIG_BAR

#else

#endif

/*
 * check-name: Check for file-based presence conditions
 * check-command: undertaker -v -m file-presence-conditions.model $file
 * check-output-start
I: Using file-presence-conditions as primary model
I: loaded rsf model for file-presence-conditions
I: creating inference.c.B0.kconfig.globally.undead
I: creating inference.c.B1.kconfig.globally.dead
 * check-output-end
 */

