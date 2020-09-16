#!/bin/sh

BINARY_DIR=$(realpath $1)
shift
SOURCE_DIR=$(realpath $1)
shift
DOCKER_ARGS="$1"
shift

SSH_DIR=$(realpath "$HOME/.ssh")
TMP_DIR="$BINARY_DIR/tmp"

ROOT_SCRIPT_BASE="root-script.sh"
ROOT_SCRIPT_FILE="$TMP_DIR/$ROOT_SCRIPT_BASE"

USER_SCRIPT_BASE="bob-script.sh"
USER_SCRIPT_FILE="$TMP_DIR/$USER_SCRIPT_BASE"

USER_ID=$(id -u $USER)

mkdir -p $TMP_DIR

echo "#!/bin/bash" > $USER_SCRIPT_FILE
echo "set -x" >> $USER_SCRIPT_FILE
for var in "$@"
do
    echo "$var" >> $USER_SCRIPT_FILE
done

# COPY local ssh files into the docker container to be able to push onto github
echo "
set -x
mkdir -p /docker-ssh
rsync -a /host-ssh/ /docker-ssh
chown -R root /docker-ssh
if ! grep \"/docker-ssh\" /etc/ssh/ssh_config; then
 echo \"IdentityFile /docker-ssh/id_rsa\" >> /etc/ssh/ssh_config
fi

adduser bob-the-builder --uid $USER_ID --disabled-password --gecos \"\"
echo \"bob-the-builder   ALL=(ALL) NOPASSWD:ALL\" >> /etc/sudoers

chmod +x /script/bob-script.sh
sudo -i -u bob-the-builder /script/bob-script.sh
" > $ROOT_SCRIPT_FILE


DOCKERNAME="nl-cross-build-environment"
docker run $DOCKER_ARGS \
    --privileged \
    --rm \
    -ti \
    -v $SSH_DIR:/host-ssh \
    -v $TMP_DIR:/script \
    -v $BINARY_DIR:/workdir \
    -v $SOURCE_DIR:/sources \
    $DOCKERNAME bash -e /script/$ROOT_SCRIPT_BASE
