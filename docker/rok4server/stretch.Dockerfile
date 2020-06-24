FROM debian:stretch-slim as builder

ARG proxy=

ENV http_proxy=${proxy}
ENV https_proxy=${proxy}
ENV ftp_proxy=${proxy}

# Environnement de compilation

RUN apt update && apt -y install  \
    build-essential cmake \
    libfcgi-dev \
    libtinyxml-dev \
    libopenjp2-7-dev \
    gettext \
    zlib1g-dev \
    libtiff5-dev \
    libpng-dev \
    libcurl4-openssl-dev \
    libssl-dev \
    libturbojpeg0-dev \
    libjpeg-dev \
    libc6-dev \
    librados-dev \
    && rm -rf /var/lib/apt/lists/*

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

RUN apt remove -y build-essential cmake libfcgi-dev libtinyxml-dev libopenjp2-7-dev zlib1g-dev libtiff5-dev libpng-dev libcurl4-openssl-dev libssl-dev libturbojpeg0-dev libjpeg-dev libc6-dev librados-dev

FROM builder

ENV PROJ_LIB=/etc/rok4/config/proj

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