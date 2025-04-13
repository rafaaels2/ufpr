#!/bin/bash

SCRIPTPATH=$(readlink -f "$0")
SCRIPTDIR=$(dirname "$SCRIPTPATH")

# In case a recent-enough glibc is available, use transparent huge pages
export GLIBC_TUNABLES=glibc.malloc.hugetlb=1

# Use every second CPU
"$SCRIPTDIR"/mergesat -cores=-2 $2