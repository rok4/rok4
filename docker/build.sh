#!/bin/bash

set -e

rok4server="0"
rok4generation="0"
baseos=""
build_params=""

ARGUMENTS="os:,rok4server,rok4generation,proxy:,help"
# read arguments
opts=$(getopt \
    --longoptions "${ARGUMENTS}" \
    --name "$(basename "$0")" \
    --options "" \
    -- "$@"
)
eval set --$opts

while [[ $# -gt 0 ]]; do
    case "$1" in
        --help)
            echo "./build.sh <OPTIONS>"
            echo "    --os debian10|centos7"
            echo "    --rok4server"
            echo "    --rok4generation"
            echo "    --proxy http://proxy.chez.vous:port"
            exit 0
            ;;

        --os)
            baseos=$2
            shift 2
            ;;

        --rok4generation)
            rok4generation="1"
            shift 1
            ;;

        --rok4server)
            rok4server="1"
            shift 1
            ;;

        --proxy)
            build_params="$build_params --build-arg http_proxy=$2 --build-arg https_proxy=$2"
            shift 2
            ;;

        *)
            break
            ;;
    esac
done

if [[ -z $baseos ]]; then
    echo "No provided OS"
    exit 1
fi

script_dir=$(dirname "$0")

# Récupération de la version depuis le README

cd $script_dir/../

version=$(grep "ROK4 Version : " README.md | sed "s#ROK4 Version : ##")
docker_tag="${version}-${baseos}"

###### ROK4GENERATION
if [[ "$rok4generation" == "1" ]]; then
    docker build -t rok4/rok4generation:${docker_tag} -f docker/rok4generation/${baseos}.Dockerfile $build_params .
fi

###### ROK4SERVER
if [[ "$rok4server" == "1" ]]; then
    docker build -t rok4/rok4server:${docker_tag} -f docker/rok4server/${baseos}.Dockerfile $build_params .
fi