#!/bin/bash

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <filename>"
    exit 1
fi

sed -i '/"op" : "mark_to_drop"/{s/"op" : "mark_to_drop"/"op" : "drop",\n/; N; N; N; N; N; N; N; s/\n.*\n.*\n.*\n.*\n.*\n.*\n/ " parameters" : [],\n/}' $1

echo "Replacement done."