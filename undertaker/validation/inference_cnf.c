#ifdef CONFIG_BAR

#else

#endif

/*
 * check-name: CNF: Check for file-based presence conditions
 * check-command: undertaker -v -m file-presence-conditions.cnf $file
 * check-output-start
I: Using file-presence-conditions as primary model
I: loaded cnf model for file-presence-conditions
I: creating inference_cnf.c.B0.kconfig.globally.undead
I: creating inference_cnf.c.B1.kconfig.globally.dead
 * check-output-end
 */
