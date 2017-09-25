if [ "$#" -ne 1 ] || ! [ -d "$1" ]; then
  echo "Usage: $0 <board>" >&2
  exit 1
fi

rm -rf build/src/*
touch build/src/sketch.ino

for f in lib/*
do
  cat $f >> build/src/sketch.ino
  printf '\n' >> build/src/sketch.ino
done

cat $1/$1.ino >> build/src/sketch.ino

cd build/
ino build -m $1
ino upload -m $1

