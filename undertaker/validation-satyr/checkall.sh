#!/bin/bash

#  checkall - testsuite for satyr
#
# Copyright (C) 2012 Ralf Hackner
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

satyr="../satyr"

checkscript=./check.sh
wrong=0
passed=0

# run  single test
# each test consists of _one_ *.fm file
# and a number of *.config files
# configurations named *_sat.config are expected to be satisfiable.
# configurations named *_unsat.config are expected to be not satisfiable

exectest () {
    subtest_dir=$1
    subtest_wrong=0
    subtests=0
    model=$subtest_dir/*.fm
    satlist=`find $subtest_dir -name *_sat.config`
    unsatlist=`find $subtest_dir -name *_unsat.config`

    echo sat: $satlist
    for config in $satlist; do
        $satyr $model -a $config
        if [ $? -ne 0 ]; then
            echo -e "\e[0;31m$subtest_dir: $config should be satifiable but is not\e[0m"
            (( subtest_wrong++ ))
        fi;
        (( subtests++ ))
    done;

    for config in $unsatlist; do
        $satyr $model -a $config
        if [ $? -ne 1 ]; then
            echo -e "\e[0;31m$subtest_dir: $config shouldn't be satifiable but is\e[0m"
            (( subtest_wrong++ ))
        fi;
        (( subtests++ ))
    done;

    (( subtest_passed = subtests - subtest_wrong ))

    if [ $subtest_wrong -ne 0 ]; then
        echo -ne '\e[0;31m'
    fi;

    echo "$subtest_passed/$subtests passed"
    echo -ne '\e[0m'
    return $subtest_wrong
}

# run test category
test_category () {
    testdir=$1;
    for i in `find $testdir -maxdepth 1 -mindepth 1 -type d`; do
        echo *Testing $i
        exectest "$i"
        if [ $? -eq 0 ]; then
            (( passed++ ));
        fi;
        (( tests++ ));
    done;
}

#"main"

#help
if [ x"$1" = x'-h' -o x"$1" = x'--help' ]; then
    echo "$0                   check all aviable tests ./"
    echo "$0  testdir1/ ...    check all tests in categories testdir1 ..."
    echo "$0 -s my_cute_test/  run my_cute_test/"
    exit 0
fi

#run single test
if [ x"$1" = x'-s' ]; then
    exectest "$2"
    exit 0
fi

#run categotries
if (( $# )); then
    todo=$@
else
    todo=`find . -maxdepth 1 -mindepth 1 -type d`
fi

for j in $todo; do
    test_category "$j";
done;

echo '___________________________________________________________'
echo "Summary: $passed/$tests passed"
if [ $passed != $tests ]; then
    exit 1;
fi

exit 0
