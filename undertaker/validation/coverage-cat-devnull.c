/*
 * check-name: FormatCommented on empty file
 * check-command: undertaker -j coverage -O exec:cat /dev/null
 * check-output-start
I: /dev/null, Found Solutions: 1, Coverage: 1/1 blocks enabled (100%)
I: Calling: cat
I: Entries in missingSet: 0
I: Removed 0 old configurations matching /dev/null.config*
 * check-output-end
 */
