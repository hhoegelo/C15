#!/bin/sh

BINARY_DIR=$(realpath $1)
SOURCE_DIR=$(realpath $2)
SCRIPT=$(realpath $3)

TMP_DIR="$BINARY_DIR/tmp"
DOCKERNAME="nl-epc-build-environment"

USER_SCRIPT_BASE="bob-script.sh"
USER_SCRIPT_FILE="$TMP_DIR/$USER_SCRIPT_BASE"

mkdir -p $TMP_DIR
cp $SCRIPT $USER_SCRIPT_FILE

docker run --privileged -ti --rm -v $TMP_DIR:/script -v $BINARY_DIR:/bindir -v $SOURCE_DIR:/sources $DOCKERNAME bash /script/$USER_SCRIPT_BASE
