#!/data/data/com.termux/files/usr/bin/bash
# Сборка JNI-исходников в Termux.
# Использование: bash build.sh [папка_с_исходниками] [имя_библиотеки]
# По умолчанию: текущая папка, libnative.so
# Ошибки сборки сохраняются в build.log

SRC="${1:-$PWD}"
LIB="${2:-libnative.so}"

pkg install -y clang || exit 1

# jni.h берём из JDK, если он есть; иначе ищем в проекте
JNI_H=$(find "$PREFIX" -name jni.h 2>/dev/null | head -n1)
if [ -z "$JNI_H" ]; then
  pkg install -y openjdk-21 >/dev/null 2>&1
  JNI_H=$(find "$PREFIX" -name jni.h 2>/dev/null | head -n1)
fi
if [ -z "$JNI_H" ]; then
  JNI_H=$(find "$SRC" -name jni.h 2>/dev/null | head -n1)
fi
if [ -z "$JNI_H" ]; then
  echo "jni.h not found"
  exit 1
fi
INC="$(dirname "$JNI_H")"
echo "jni.h: $JNI_H"

cd "$SRC" || exit 1

FILES=$(find . \( -name '*.cpp' -o -name '*.cc' -o -name '*.c' \) | sort)
if [ -z "$FILES" ]; then
  echo "No .c/.cpp files in $SRC"
  exit 1
fi

# папки проекта подключаем через -iquote, но enet/include пропускаем:
# его types.h и time.h подменяют системные заголовки
QUOTES=$(find . -type d -not -path '*/enet/include*' | sed 's/^/-iquote /' | tr '\n' ' ')

echo "Building $LIB from $(echo "$FILES" | wc -l) files..."
clang++ -shared -fPIC -O2 -static-libstdc++ -std=gnu++14 -fexceptions -w -I. -Ivendor -Ivendor/curl/include \
  $QUOTES \
  $FILES -o "$LIB" -llog 2> build.log

if [ $? -eq 0 ]; then
  echo "OK: $SRC/$LIB"
else
  echo "Build failed. First errors:"
  grep -m5 -B1 -A3 "error:" build.log
  echo "Full log: $SRC/build.log"
  exit 1
fi
