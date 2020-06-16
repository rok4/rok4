FROM debian:buster

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
    perl perl-base \
    libgdal-perl libpq-dev gdal-bin \
    libsqlite3-dev git \
    && rm -rf /var/lib/apt/lists/*

RUN cpan -T Config::INI::Reader DBI DBD::Pg Data::Dumper Devel::Size Digest::SHA ExtUtils::MakeMaker File::Find::Rule File::Map FindBin Geo::GDAL Geo::OGR Geo::OSR HTTP::Request HTTP::Request::Common HTTP::Response JSON::Parse Log::Log4perl LWP::UserAgent Math::BigFloat Term::ProgressBar Test::More Tie::File XML::LibXML

# Compilation et installation

COPY ./CMakeLists.txt /sources/CMakeLists.txt
COPY ./cmake /sources/cmake
COPY ./lib /sources/lib
COPY ./rok4generation /sources/rok4generation
COPY ./README.md /sources/README.md
COPY ./rok4version.h.in /sources/rok4version.h.in
COPY ./config/proj /sources/config/proj
COPY ./config/styles /sources/config/styles
COPY ./config/tileMatrixSet /sources/config/tileMatrixSet

RUN mkdir -p /build
WORKDIR /build

RUN cmake -DCMAKE_INSTALL_PREFIX=/ -DBUILD_OBJECT=1 -DBUILD_DOC=0 -DUNITTEST=0 -DDEBUG_BUILD=0 -DBUILD_ROK4=0 /sources/ && make && make install && rm -r /sources /build

# Installation de tippecanoe

RUN git clone https://github.com/mapbox/tippecanoe.git /tippecanoe
WORKDIR /tippecanoe
RUN make -j && make install

WORKDIR /

CMD echo "No default command"