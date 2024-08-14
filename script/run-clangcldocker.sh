#!/usr/bin/env bash
set -e
COMMAND=$*
SCRIPTPATH="$( cd "$(dirname "$0")" ; pwd -P )"
MAINSOURCE=$SCRIPTPATH/..
ALL_FILES=$(cd $MAINSOURCE && git ls-tree --full-tree --name-only -r HEAD | grep -e ".*\.\(c\|h\|cc\|cpp\|hh\)\$" | grep -vFf clang-format-ignore.txt)

if clang-format-17 --version  2>/dev/null | grep -qF 'version 17.'; then
  cd $MAINSOURCE; clang-format-17 --style=file --verbose -i "$@" $ALL_FILES
  exit 0
elif clang-format --version  2>/dev/null | grep -qF 'version 17.'; then
  cd $MAINSOURCE; clang-format --style=file --verbose -i "$@" $ALL_FILES
  exit 0
fi
echo "Trying to use docker"
command -v docker >/dev/null 2>&1 || { echo >&2 "Please install docker. E.g., go to https://www.docker.com/products/docker-desktop Type 'docker' to diagnose the problem."; exit 1; }
docker info >/dev/null 2>&1 || { echo >&2 "Docker server is not running? type 'docker info'."; exit 1; }

if [ -t 0 ]; then DOCKER_ARGS=-it; fi
docker pull kszonek/clang-format-17

docker run --rm $DOCKER_ARGS -v "$MAINSOURCE":"$MAINSOURCE":Z  -w "$MAINSOURCE" -u "$(id -u $USER):$(id -g $USER)" kszonek/clang-format-17 --style=file --verbose -i "$@" $ALL_FILES
