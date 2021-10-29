FROM centos:7 as libs

# Librairies

RUN yum -y update && yum -y install epel-release centos-release-scl-rh

RUN yum -y update && \
        yum -y --enablerepo=extras install \
        fcgi-devel \
        tinyxml-devel \
        openjpeg2-devel \
        zlib-devel \
        libtiff-devel \
        libpng-devel \
        libcurl-devel \
        proj-devel \
        openssl-devel \
        turbojpeg-devel \
        libjpeg-turbo-devel \
        librados2-devel

#### Compilation de l'application

FROM libs AS builder

# Environnement de compilation

RUN yum -y --enablerepo=extras install make cmake gcc gcc-c++ devtoolset-7-gcc-c++
ENV PATH=/opt/rh/devtoolset-7/root/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin

# Compilation et installation des librairies boost
RUN curl -L -o /boost_1_77_0.tar.gz "https://boostorg.jfrog.io/artifactory/main/release/1.77.0/source/boost_1_77_0.tar.gz" && tar xvf /boost_1_77_0.tar.gz
RUN cd /boost_1_77_0 && ./bootstrap.sh --prefix=/usr && ./b2 install --with-log --with-filesystem --with-system --with-thread

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

RUN cmake -DCMAKE_INSTALL_PREFIX=/ -DBUILD_OBJECT=1 -DBUILD_DOC=0 -DUNITTEST=0 -DDEBUG_BUILD=0 -DBUILD_BE4=0 /sources/ && make && make install

#### Image de run à partir des libs et de l'exécutable compilé

FROM libs

ENV PROJ_LIB=/etc/rok4/config/proj

WORKDIR /

# Récupération de l'exécutable
COPY --from=builder /bin/rok4 /bin/rok4
COPY --from=builder /etc/rok4/config/tileMatrixSet /etc/rok4/config/tileMatrixSet
COPY --from=builder /etc/rok4/config/styles /etc/rok4/config/styles
COPY --from=builder /etc/rok4/config/proj /etc/rok4/config/proj

# Récupération des librairies boost
COPY --from=builder /usr/lib/libboost_* /usr/lib/
RUN ldconfig

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
