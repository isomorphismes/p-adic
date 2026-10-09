# Native C producer

The five arithmetic divisions in `native/witt_native.c` use `÷`. The C source
is compiled directly by ICK at `c61e448251744a2f40ad743ebef1a027bdcd2f9d`.
NDK Clang assembles that output, compiles the unchanged NDK NativeActivity
glue, and links the library. There is no source normalization step.

The existing `build-native.sh` entry point calls `android/Makefile`. It requires
the absolute `ICK_COMPILER` path in addition to the existing NDK setting.
`ICK_TARGET_FLAGS` and `ICK_HEADER_TARGET` come from the pinned shared
`isomorphisms/ai-ci/ick-android` action. A local uninstalled compiler can also
provide `ICK_COMPILER_OPTIONS` for its support files.

The producer keeps the ARMv7 ABI, API 21 floor, original C11/Oz/PIC section
flags, warnings, link checks and strip/export checks. This direct NDK route
did not previously define `_FORTIFY_SOURCE`; the change retains that build
configuration. The workflow checks out the exact pull-request source head
and retains the producer-stage receipt with the library and APK.

Local validation on 2026-10-09 compiled the unchanged glyph source with the
qualified ARM ICK frontend, assembled and linked it with NDK r27c, and passed
the existing ELF export checks. The stripped ARM library is 22,528 bytes.
After restoring the exact r29 compiler runtime libraries, the full native
producer also passed with NDK r29. Its stripped library is 22,688 bytes with
SHA-256 `90ebd124ae0d9254dbe4284d5d0431846b1f41c8160260f195bad4cb7add9564`.
The workflow uses a complete SDK-installed NDK r29 for the APK build.
These are build checks; physical phone interaction remains separate.
