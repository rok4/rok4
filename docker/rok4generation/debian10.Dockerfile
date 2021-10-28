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
    perl-base \
    libgdal-perl libpq-dev gdal-bin \
    libsqlite3-dev \
    libboost-log-dev libboost-filesystem-dev libboost-system-dev 

RUN cpan -T Config::INI::Reader DBI DBD::Pg Data::Dumper Devel::Size Digest::SHA ExtUtils::MakeMaker File::Find::Rule File::Map FindBin Geo::GDAL Geo::OGR Geo::OSR HTTP::Request HTTP::Request::Common HTTP::Response JSON::Parse Log::Log4perl LWP::UserAgent LWP::Protocol::https Math::BigFloat Term::ProgressBar Test::More Tie::File XML::LibXML JSON

#### Compilation de l'application

FROM libs AS builder

# Environnement de compilation

RUN apt update && apt -y install build-essential cmake git

# Compilation et installation des outils ROK4

COPY ./CMakeLists.txt /sources/CMakeLists.txt
COPY ./README.md /sources/README.md
COPY ./cmake /sources/cmake
COPY ./lib /sources/lib
COPY ./rok4generation /sources/rok4generation
COPY ./rok4version.h.in /sources/rok4version.h.in
COPY ./config/proj /sources/config/proj
COPY ./config/styles /sources/config/styles
COPY ./config/tileMatrixSet /sources/config/tileMatrixSet

RUN mkdir -p /build
WORKDIR /build

RUN cmake -DCMAKE_INSTALL_PREFIX=/ -DBUILD_OBJECT=1 -DBUILD_DOC=0 -DUNITTEST=0 -DDEBUG_BUILD=0 -DBUILD_ROK4=0 /sources/
RUN make && make package

RUN git clone --depth=1 https://github.com/mapbox/tippecanoe.git /tippecanoe
WORKDIR /tippecanoe
RUN make -j && make install

#### Image de run à partir des libs et de l'exécutable compilé

FROM libs

ENV PROJ_LIB=/etc/rok4/config/proj

WORKDIR /

# Récupération des exécutables
COPY --from=builder /usr/local/bin/tippecanoe /bin/tippecanoe
COPY --from=builder /build/Rok4-*-Linux-64bit.tar.gz /

RUN apt -y install procps wget gdal-bin && tar xvzf /Rok4-*-Linux-64bit.tar.gz

CMD ls -l /bin
