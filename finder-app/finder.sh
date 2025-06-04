#!/bin/bash

filesdir=$1
searchstr=$2

if [ $# -lt 2 ]
then
    echo "expected 2 arguments, $# provided"
    exit 1
fi

if [ ! -d "$filesdir" ]
then
    echo "filesdir is not a directory"
    exit 1
fi

filecount=$(grep -rl $searchstr $filesdir | wc -l)
linecount=$(grep -r "$searchstr" "$filesdir" | wc -l)

echo "The number of files are $filecount and the number of matching lines are $linecount"

exit 0