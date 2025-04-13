################### Build MergeSat
FROM satcomp-infrastructure:common

# Only build remote for now
# TODO: support host-local repositories
ARG REPO=https://github.com/conp-solutions/mergesat.git
ARG COMMIT=origin/master

USER root
# Install required softwares
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt install -y cmake build-essential zlib1g-dev libopenmpi-dev wget unzip build-essential zlib1g-dev cmake python3 build-essential gfortran wget curl

# Build MergeSat from given repo and commit
RUN git clone --depth=1 $REPO mergesat && cd mergesat && git checkout $COMMIT && cd ..
WORKDIR mergesat
RUN make r -j 6

