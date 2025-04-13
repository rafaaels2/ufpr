FROM fedora:36

# Install basic MergeSat requirements for static linking
RUN yum -y update && \
    yum -y install apt-utils automake file g++ gcc git glibc-static libstdc++-static make

# Install SAT competition 2022 dependencies
RUN yum -y install cmake curl gfortran iproute iputils python python-pip unzip
RUN yum -y install openssh-server openmpi-devel python3-pip
# ... also install python dependencies
RUN pip3 install awscli boto3 flask pytz polling2 supervisor waitress

# Import and build the solver
ADD . /opt/mergesat

# Clean solver
RUN cd /opt/mergesat && make clean
RUN rm -rf /opt/mergesat/build

# Build solver from scratch
RUN cd /opt/mergesat && make r

# Check whether the binary is statically linked
RUN file /opt/mergesat/build/release/bin/mergesat

# Enable using transparent huge pages via glibc tunables, since 2.35
ENV GLIBC_TUNABLES=glibc.malloc.hugetlb=1

# Run the solver with the environment variable parameters
CMD /opt/mergesat/tools/container-default-command.sh
