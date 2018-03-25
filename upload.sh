if [ "$#" -lt 1 ] || ! [ -d "$1" ]; then
  echo "Usage: $0 <board>" >&2
  exit 1
fi

board_name=$1
shift

rm -rf build/src/*
cp $board_name/$board_name.ino build/src/

cd build/
ino build -m $board_name
rc=$?
if [ $rc != 0 ]; then
  exit $rc
fi

ino upload -m $board_name $@
