FROM debian:buster-slim as libs

# Librairies

RUN apt update && apt -y install  \
    libfcgi-dev \
    libtinyxml-dev \
    libopenjp2-7-dev \
    zlib1g-dev \
    libtiff5-dev \
    libpng-dev \
    libcurl4-openssl-dev \
    libssl-dev \
    libturbojpeg0-dev \
    libjpeg-dev \
    libc6-dev \
    librados-dev \
    libboost-log-dev libboost-filesystem-dev libboost-system-dev 

#### Compilation de l'application

FROM libs AS builder

# Environnement de compilation

RUN apt -y install build-essential cmake

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

#### Image de run à partir des libs et de l'exécutable compilé

FROM libs

ENV PROJ_LIB=/etc/rok4/config/proj
WORKDIR /

# Récupération de l'exécutable
COPY --from=builder /bin/rok4 /bin/rok4
COPY --from=builder /etc/rok4/config/tileMatrixSet /etc/rok4/config/tileMatrixSet
COPY --from=builder /etc/rok4/config/styles /etc/rok4/config/styles

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
