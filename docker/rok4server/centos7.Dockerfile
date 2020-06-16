FROM centos:7

ENV http_proxy=${proxy}
ENV https_proxy=${proxy}
ENV ftp_proxy=${proxy}

RUN yum install -y epel-release centos-release-scl-rh

# Environnement de compilation

RUN yum -y --enablerepo=extras install \
        make cmake gcc gcc-c++ devtoolset-7-gcc-c++ \
        fcgi-devel \
        tinyxml-devel \
        openjpeg2-devel \
        gettext \
        zlib-devel \
        libtiff-devel \
        libpng-devel \
        libcurl-devel \
        openssl-devel \
        turbojpeg-devel \
        libjpeg-turbo-devel \
        librados2-devel

ENV PATH=/opt/rh/devtoolset-7/root/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin

# Compilation et installation

COPY ./CMakeLists.txt /sources/CMakeLists.txt
COPY ./cmake /sources/cmake
COPY ./lib /sources/lib
COPY ./rok4server /sources/rok4server
COPY ./README.md /sources/README.md
COPY ./rok4version.h.in /sources/rok4version.h.in
COPY ./config/proj /sources/config/proj
COPY ./config/styles /sources/config/styles
COPY ./config/tileMatrixSet /sources/config/tileMatrixSet

RUN mkdir -p /build
WORKDIR /build

RUN cmake -DCMAKE_INSTALL_PREFIX=/ -DBUILD_OBJECT=1 -DBUILD_DOC=0 -DUNITTEST=0 -DDEBUG_BUILD=0 -DBUILD_BE4=0 /sources/ && make && make install && rm -r /sources /build

WORKDIR /

# Configuration

COPY ./config/server.conf.docker /etc/rok4/config/server.conf
COPY ./config/services.conf.docker /etc/rok4/config/services.conf
COPY ./config/restrictedCRSList.txt.docker /etc/rok4/config/restrictedCRSList.txt

COPY ./docker/rok4server/start_rok4server.sh /start_rok4server.sh
RUN chmod +x /start_rok4server.sh
COPY ./docker/rok4server/defaults /etc/default/rok4server

RUN mkdir /layers /pyramids

VOLUME /layers
VOLUME /pyramids

EXPOSE 9000

CMD /start_rok4server.sh