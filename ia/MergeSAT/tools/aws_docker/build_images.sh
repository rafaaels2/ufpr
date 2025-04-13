#!/usr/bin/env bash

declare -r COMMON_TAG="satcomp-mergesat:common"
declare -r LEADER_TAG="satcomp-mergesat:leader"

build_competition_base_images() (
    if docker image ls satcomp-infrastructure:leader | grep "satcomp-infrastructure" &&
        docker image ls satcomp-infrastructure:worker | grep "satcomp-infrastructure"; then
        echo "Already detected AWS infrastructure images, not re-building them"
        echo "To rebuild, delete images via e.g. docker rmi satcomp-infrastructure:leader"
        exit 0
    fi

    trap '[ -d $TMP_DOCKER_DIR ] && rm "$TMP_DOCKER_DIR"' EXIT
    TMP_DOCKER_DIR$(mktemp -d)
    pushd "$TMP_DOCKER_DIR"
    # operate in temporary directory
    git clone https://github.com/aws-samples/aws-batch-comp-infrastructure-sample.git
    cd aws-batch-comp-infrastructure-sample/
    cd docker/
    cd satcomp-images/
    ./build_satcomp_images.sh
    docker images ls
    popd
)

if ! docker info &>/dev/null; then
    echo "ERROR Failed to access docker"
fi

build_competition_base_images

pushd common
if ! docker build -t "$COMMON_TAG" . | grep "Successfully tagged $COMMON_TAG"; then
    docker build -t "$COMMON_TAG" .
    echo "ERROR Failed to build common docker image."
    exit 1
fi
popd

pushd leader
if ! docker build -t "$LEADER_TAG" . | grep "Successfully tagged $LEADER_TAG"; then
    docker build -t "$LEADER_TAG" .
    echo "ERROR Failed to build leader docker image."
    exit 1
fi

popd

echo "Not building worker image ..."
echo ""
docker image ls "$LEADER_TAG"
echo ""
echo "SUCCESS: Build docker image wit tag '$LEADER_TAG'"
exit 0
