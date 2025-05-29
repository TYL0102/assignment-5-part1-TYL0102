#!/bin/sh

# check arguments
if [ $# -lt 2 ]
then
    echo "too few arguments ${#}"
    exit 1
fi

# create directory if not exist
if [ ! -e $1 ]
then
    mkdir -p $(dirname $1)
fi

# write file
echo $2 > $1

# check success
if [ $? -ne 0 ]
then 
    echo "fail to create file ${1}"
    exit 1
fi
exit 0