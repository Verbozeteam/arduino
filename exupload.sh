if [ "$#" -lt 2 ] || ! [ -d "$1" ]; then
  echo "Usage: $0 <code_folder> <board>" >&2
  exit 1
fi

code_folder=$1
board_name=$2
shift
shift

rm -rf build/src/*
cp $code_folder/$code_folder.ino build/src/

cd build/
ino build -m $board_name
rc=$?
if [ $rc != 0 ]; then
  exit $rc
fi

ino upload -m $board_name $@
