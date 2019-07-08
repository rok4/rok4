FROM centos:7

ENV http_proxy=${http_proxy}
ENV https_proxy=${http_proxy}
ENV ftp_proxy=${http_proxy}

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

ADD ../ /sources

RUN mkdir -p /build
WORKDIR /build

RUN cmake -DCMAKE_INSTALL_PREFIX=/rok4server -DBUILD_OBJECT=1 -DBUILD_DOC=0 -DUNITTEST=0 -DDEBUG_BUILD=0 -DBUILD_BE4=0 /sources/
RUN make && make install


EXPOSE 9000
CMD /rok4server/bin/rok4 -f /confs/server.conf