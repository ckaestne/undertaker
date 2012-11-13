/*
 * check-name: FormatCommented on empty file
 * check-command: undertaker -v -j coverage -O exec:cat /dev/null
 * check-output-start
I: /dev/null, Found Solutions: 1, Coverage: 1/1 blocks enabled (100%)
I: Calling: cat
I: Removed 0 leftovers for /dev/null
 * check-output-end
 */
