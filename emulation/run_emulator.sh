#!/bin/sh

if [ "$#" -lt 1 ]; then
  echo "Usage: $0 <board> [<rpc port> [<serial port> [<no libs>]]]" >&2
  exit 1
fi

RPC_PORT="5001"
if [ "$#" -ge 2 ]; then
    RPC_PORT="$2"
fi

SERIAL_PORT="9911"
if [ "$#" -ge 3 ]; then
    SERIAL_PORT="$3"
fi

NO_LIBS=""
if [ "$#" -ge 4 ]; then
    NO_LIBS=$4
fi

PATH_TO_SHAMMAM="../../shammam/shammam.py"
PROTOCOL="custom_protocol.proto"
SHAMMAM_ARGS=""

BOARD="$1"
LIBS_PATH="../build/lib"
LIBRARY_SOURCES="$LIBS_PATH/V*/*.cpp"
LIBRARY_INCLUDE_DIRS="$LIBS_PATH/V*"

STUB_SOURCES="src/*"
STUB_INCLUDE_DIRS="inc"

if [ "$NO_LIBS" = "1" ]; then
    LIBRARY_SOURCES=""
    LIBRARY_INCLUDE_DIRS=""
fi

SOURCES="../$BOARD/$BOARD.ino $LIBRARY_SOURCES $STUB_SOURCES"
EXTRA_INCLUDES="$LIBRARY_INCLUDE_DIRS $STUB_INCLUDE_DIRS"
python $PATH_TO_SHAMMAM -n $BOARD -s $SOURCES -i $EXTRA_INCLUDES -p $PROTOCOL --port $RPC_PORT --serial_port $SERIAL_PORT $SHAMMAM_ARGS

