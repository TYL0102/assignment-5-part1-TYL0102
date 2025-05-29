#!/bin/sh

# check arguments
if [ $# -lt 2 ]
then
    echo "too few arguments ${#}"
    exit 1
fi

# check directory
if [ ! -d $1 ]
then
    echo "${1} is not a directory"
    exit 1
fi

# find
file_count=$(find $1 -type f | wc -l)
match_line=$(grep -r $2 $1 | wc -l)
echo "The number of files are ${file_count} and the number of matching lines are ${match_line}"
exit 0