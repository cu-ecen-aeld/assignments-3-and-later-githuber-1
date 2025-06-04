#!/bin/bash

writefile=$1
writestr=$2

if [ $# -lt 2 ]
then
	echo "expected 2 arguments, $# provided"
	exit 1
fi

mkdir -p "$(dirname "$writefile")"

echo $writestr > $writefile

exit 0
