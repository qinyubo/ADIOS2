FROM centos:centos7

# Install core dev packages
RUN yum -y install make curl file git gcc pkg-config \
        gdb zlib zlib-devel bzip2 bzip2-libs bzip2-devel python-devel
RUN yum -y install epel-release
RUN yum -y install zeromq-devel

# Install Intel C++ compiler
WORKDIR /tmp
COPY silent-custom.cfg /tmp
RUN curl -L http://registrationcenter-download.intel.com/akdlm/irc_nas/tec/12717/parallel_studio_xe_2018_update2_cluster_edition.tgz | tar -xz \
  && ./parallel_studio_xe_2018_update2_cluster_edition/install.sh -s ./silent-custom.cfg \
  && rm -rf parallel_studio_xe_2018* silent*

# Install extra repos
RUN yum -y install epel-release https://github.com/openhpc/ohpc/releases/download/v1.3.GA/ohpc-release-1.3-1.el7.x86_64.rpm

# Install intel OpenHPC packages
RUN yum -y install \
  lmod-ohpc intel-compilers-devel-ohpc python-numpy-intel-ohpc \
  openmpi3-intel-ohpc python-mpi4py-intel-openmpi3-ohpc \
  phdf5-intel-openmpi3-ohpc netcdf-intel-openmpi3-ohpc adios-intel-openmpi3-ohpc

# Cleanup headers and packages
RUN yum clean all

# Install libfabric
WORKDIR /opt/libfabric
RUN curl -L https://github.com/ofiwg/libfabric/releases/download/v1.6.0/libfabric-1.6.0.tar.bz2 | tar -xj \
  && pushd libfabric-1.6.0 \
  && ./configure --prefix=/opt/libfabric/1.6.0 --libdir=/opt/libfabric/1.6.0/lib64 \
  && make -j8 install \
  && popd \
  && rm -rf libfabric-1.6.0

# Install the CMake binary
WORKDIR /opt/cmake/3.6.3
RUN curl -L https://cmake.org/files/v3.6/cmake-3.6.3-Linux-x86_64.tar.gz | \
  tar --strip-components=1 -xz

# Create a non-root user to run the builds/tests
RUN export uid=1001 gid=1001 && \
    mkdir -p /home/adios2 && \
    echo "adios2:x:${uid}:${gid}:adios2,,,:/home/adios2:/bin/bash" >> /etc/passwd && \
    echo "adios2:x:${uid}:" >> /etc/group && \
    chown ${uid}:${gid} -R /home/adios2

# Misc cleanup of unneeded files
RUN rm -rfv /tmp/* \
  && rm -rfv \
    /opt/intel/man \
    /opt/intel/documentation_2018 \
    /opt/intel/ide_support_2018 \
    /opt/intel/samples_2018 \
    /opt/intel/compilers_and_libraries_2018.2.199/linux/mkl/benchmarks \
    /opt/intel/compilers_and_libraries_2018.2.199/linux/mkl/examples \
  && rm -rf /var/cache/yum

USER adios2
ENV HOME /home/adios2
WORKDIR /home/adios2
CMD /bin/bash
