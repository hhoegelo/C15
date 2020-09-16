#!/bin/sh

BINARY_DIR=$(realpath $1)
SOURCE_DIR=$(realpath $2)
SCRIPT=$(realpath $3)

TMP_DIR="$BINARY_DIR/tmp"
DOCKERNAME="nl-epc-build-environment"

ROOT_SCRIPT_BASE="root-script.sh"
USER_SCRIPT_BASE="bob-script.sh"

ROOT_SCRIPT_FILE="$TMP_DIR/$ROOT_SCRIPT_BASE"
USER_SCRIPT_FILE="$TMP_DIR/$USER_SCRIPT_BASE"

USER_ID=$(id -u $USER)

mkdir -p $TMP_DIR
cp $SCRIPT $USER_SCRIPT_FILE

echo "
set -x
adduser bob-the-builder --uid $USER_ID --disabled-password --gecos \"\"
echo \"bob-the-builder   ALL=(ALL) NOPASSWD:ALL\" >> /etc/sudoers

chmod +x /script/bob-script.sh
sudo -i -u bob-the-builder /script/bob-script.sh
" > $ROOT_SCRIPT_FILE

docker run --privileged --rm -v $TMP_DIR:/script -v $BINARY_DIR:/bindir -v $SOURCE_DIR:/sources $DOCKERNAME bash /script/$ROOT_SCRIPT_BASE
