#!/usr/bin/env bash
set -Eeuo pipefail

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
output=${1:-"$repo_root/build/android/armeabi-v7a/libwitt.so"}

abi=${ANDROID_ABI:-armeabi-v7a}
api=${ANDROID_API:-21}
[[ "$abi" == "armeabi-v7a" ]] || {
    echo "This lane is intentionally MIRO/armeabi-v7a only: $abi" >&2
    exit 1
}

ndk=${ANDROID_NDK_HOME:-${ANDROID_NDK_ROOT:-}}
if [[ -z "$ndk" ]]; then
    android_home=${ANDROID_HOME:-${ANDROID_SDK_ROOT:-}}
    [[ -n "$android_home" ]] || {
        echo "ANDROID_NDK_HOME or ANDROID_HOME is required" >&2
        exit 1
    }
    ndk=$(find "$android_home/ndk" -mindepth 1 -maxdepth 1 -type d | sort -V | tail -n 1)
fi

toolchain=$(find "$ndk/toolchains/llvm/prebuilt" -mindepth 1 -maxdepth 1 -type d | head -n 1)
clang="$toolchain/bin/armv7a-linux-androideabi${api}-clang"
strip="$toolchain/bin/llvm-strip"
readelf="$toolchain/bin/llvm-readelf"
glue_dir="$ndk/sources/android/native_app_glue"
glue="$glue_dir/android_native_app_glue.c"

for required in "$clang" "$strip" "$readelf" "$glue"; do
    [[ -e "$required" ]] || {
        echo "missing Android build input: $required" >&2
        exit 1
    }
done

mkdir -p "$(dirname -- "$output")"

"$clang"     -std=c11     -Oz     -fPIC     -ffunction-sections     -fdata-sections     -shared     -Wall     -Wextra     -Werror     -I "$glue_dir"     "$repo_root/android/native/witt_native.c"     "$glue"     -Wl,--gc-sections     -Wl,--no-undefined     -Wl,-soname,libwitt.so     -landroid     -llog     -lEGL     -lGLESv2     -lm     -o "$output"

"$strip" --strip-unneeded "$output"

symbols=$("$readelf" -Ws "$output")
grep -Fq "ANativeActivity_onCreate" <<<"$symbols"
grep -Fq "android_main" <<<"$symbols"

printf 'ABI             %s\n' "$abi"
printf 'API floor       %s\n' "$api"
printf 'native library  %s\n' "$output"
printf 'native bytes    %s\n' "$(wc -c < "$output")"
