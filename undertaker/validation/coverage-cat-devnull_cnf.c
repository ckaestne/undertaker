/*
 * check-name: CNF: FormatCommented on empty file
 * check-command: undertaker -v -j coverage -m epsilon.cnf -O exec:cat /dev/null
 * check-output-start
I: /dev/null, Found Solutions: 1, Coverage: 1/1 blocks enabled (100%)
I: Calling: cat
I: loaded cnf model for epsilon
I: Using epsilon as primary model
I: Removed 0 leftovers for /dev/null
 * check-output-end
 */
