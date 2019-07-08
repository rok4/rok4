FROM debian:buster

ENV http_proxy=${http_proxy}
ENV https_proxy=${http_proxy}
ENV ftp_proxy=${http_proxy}

# Environnement de compilation

RUN apt update \
        && apt -y install  \
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

COPY ./ /sources

RUN mkdir -p /build
WORKDIR /build

RUN cmake -DCMAKE_INSTALL_PREFIX=/rok4server -DBUILD_OBJECT=1 -DBUILD_DOC=0 -DUNITTEST=0 -DDEBUG_BUILD=0 -DBUILD_BE4=0 /sources/ && make && make install && rm -r /sources


EXPOSE 9000
CMD /rok4server/bin/rok4 -f /confs/server.conf