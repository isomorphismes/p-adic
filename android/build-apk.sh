#!/usr/bin/env bash
set -Eeuo pipefail

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
native_library=${1:-"$repo_root/build/android/armeabi-v7a/libwitt.so"}
output=${2:-"$repo_root/build/android/witt-miro-armeabi-v7a.apk"}

android_home=${ANDROID_HOME:-${ANDROID_SDK_ROOT:-}}
[[ -n "$android_home" ]] || {
    echo "ANDROID_HOME or ANDROID_SDK_ROOT is required" >&2
    exit 1
}

compile_sdk=${ANDROID_COMPILE_SDK:-36}
build_tools=${ANDROID_BUILD_TOOLS:-}
if [[ -z "$build_tools" ]]; then
    build_tools=$(find "$android_home/build-tools" -mindepth 1 -maxdepth 1 -type d | sort -V | tail -n 1)
fi

aapt2="$build_tools/aapt2"
zipalign="$build_tools/zipalign"
apksigner="$build_tools/apksigner"
android_jar="$android_home/platforms/android-$compile_sdk/android.jar"

for required in "$native_library" "$aapt2" "$zipalign" "$apksigner" "$android_jar"; do
    [[ -e "$required" ]] || {
        echo "missing APK build input: $required" >&2
        exit 1
    }
done

for name in WITT_KEYSTORE WITT_KEYSTORE_TYPE WITT_KEY_ALIAS WITT_STORE_PASSWORD WITT_KEY_PASSWORD WITT_EXPECTED_CERT_SHA256; do
    [[ -n "${!name:-}" ]] || {
        echo "required signing value is unset: $name" >&2
        exit 1
    }
done

expected=$(printf '%s' "$WITT_EXPECTED_CERT_SHA256" | tr '[:upper:]' '[:lower:]' | tr -d ':[:space:]')

work="$repo_root/build/android/apk-work"
rm -rf "$work"
mkdir -p "$work/lib/armeabi-v7a" "$(dirname -- "$output")"
cp "$native_library" "$work/lib/armeabi-v7a/libwitt.so"

manifest_apk="$work/manifest.apk"
unaligned="$work/unaligned.apk"
aligned="$work/aligned.apk"

"$aapt2" link     -I "$android_jar"     --manifest "$repo_root/android/AndroidManifest.xml"     --min-sdk-version 21     --target-sdk-version 36     -o "$manifest_apk"

cp "$manifest_apk" "$unaligned"
(
    cd "$work"
    zip -q -u "$unaligned" "lib/armeabi-v7a/libwitt.so"
)

"$zipalign" -f 4 "$unaligned" "$aligned"

"$apksigner" sign     --ks "$WITT_KEYSTORE"     --ks-type "$WITT_KEYSTORE_TYPE"     --ks-pass "pass:$WITT_STORE_PASSWORD"     --key-pass "pass:$WITT_KEY_PASSWORD"     --ks-key-alias "$WITT_KEY_ALIAS"     --out "$output"     "$aligned"

report=$("$apksigner" verify --verbose --print-certs "$output" 2>&1)
printf '%s\n' "$report"

actual=$(
    printf '%s\n' "$report" |
    sed -n 's/^.*certificate SHA-256 digest:[[:space:]]*//p' |
    tr '[:upper:]' '[:lower:]' |
    tr -d ':[:space:]' |
    sort -u
)

[[ "$actual" == "$expected" ]] || {
    echo "finished APK signer changed: expected $expected got ${actual:-missing}" >&2
    exit 1
}

unzip -l "$output" > "${output%.apk}.contents.txt"
grep -Fq "lib/armeabi-v7a/libwitt.so" "${output%.apk}.contents.txt"
! grep -Fq "classes.dex" "${output%.apk}.contents.txt"
sha256sum "$output" > "${output%.apk}.sha256"

printf 'APK             %s\n' "$output"
printf 'APK bytes       %s\n' "$(wc -c < "$output")"
cat "${output%.apk}.sha256"
