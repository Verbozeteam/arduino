if [ "$#" -ne 1 ] || ! [ -d "$1" ]; then
  echo "Usage: $0 <board>" >&2
  exit 1
fi

rm -rf build/src/*
cp $1/$1.ino build/src/

cd build/
ino build -m $1
ino upload -m $1

