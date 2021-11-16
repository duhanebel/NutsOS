#!/bin/bash
SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
echo $SCRIPTPATH
if [ -z "$1" ]; then
  echo "Please specify a command to run"
  exit 1
fi
#echo "Executing: " "$@"
#echo "  in /home/build/"
# --privileged is needed if we want to mount fat images to write some files on os.bin
docker run                              \
       --rm                             \
       --privileged                     \
       -v ${SCRIPTPATH}/..:/home/build/ \
       peach-gcc-2.11                   \
       /bin/bash -c "cd /home/build/ && $*"

