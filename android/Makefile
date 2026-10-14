# The maintained C source is compiled by ICK. NDK tools retain their own glue,
# sysroot, assembly and link stages. Existing script callers enter this make API.
WITT_ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/..)
ICK_COMPILER ?=
ICK_COMPILER_OPTIONS ?=
ICK_TARGET_FLAGS ?= -marm -march=armv7-a -mfpu=vfpv3-d16 -mfloat-abi=softfp
ICK_HEADER_TARGET ?= arm-linux-androideabi
ANDROID_NDK_HOME ?= $(ANDROID_NDK_ROOT)
ANDROID_API ?= 21
NDK_BIN = $(ANDROID_NDK_HOME)/toolchains/llvm/prebuilt/linux-x86_64/bin
NDK_SYSROOT = $(ANDROID_NDK_HOME)/toolchains/llvm/prebuilt/linux-x86_64/sysroot
NDK_GLUE = $(ANDROID_NDK_HOME)/sources/android/native_app_glue
NDK_CLANG = $(NDK_BIN)/armv7a-linux-androideabi$(ANDROID_API)-clang
NDK_FLAGS = --sysroot="$(NDK_SYSROOT)"
NATIVE_LIBRARY ?= $(WITT_ROOT)/build/android/armeabi-v7a/libwitt.so
WITT_C_FLAGS = -std=c11 -Oz -fPIC -ffunction-sections -fdata-sections -Wall -Wextra

.PHONY: native
native:
	test -n "$(ICK_COMPILER)"
	test -x "$(ICK_COMPILER)"
	test -x "$(NDK_CLANG)"
	test "$$("$(ICK_COMPILER)" $(ICK_COMPILER_OPTIONS) -dumpmachine)" = arm-linux-gnueabi
	test "$(ICK_HEADER_TARGET)" = arm-linux-androideabi
	mkdir -p "$(dir $(NATIVE_LIBRARY))"
	"$(ICK_COMPILER)" $(ICK_COMPILER_OPTIONS) $(ICK_TARGET_FLAGS) $(WITT_C_FLAGS) -S --sysroot="$(NDK_SYSROOT)" -isystem "$(NDK_SYSROOT)/usr/include" -isystem "$(NDK_SYSROOT)/usr/include/$(ICK_HEADER_TARGET)" -I "$(NDK_GLUE)" -D__ANDROID__ -D__ANDROID_API__=$(ANDROID_API) -DBIONIC_IOCTL_NO_SIGNEDNESS_OVERLOAD "$(WITT_ROOT)/android/native/witt_native.c" -o "$(NATIVE_LIBRARY).s"
	"$(NDK_CLANG)" $(NDK_FLAGS) $(ICK_TARGET_FLAGS) -c "$(NATIVE_LIBRARY).s" -o "$(NATIVE_LIBRARY).witt.o"
	"$(NDK_CLANG)" $(NDK_FLAGS) $(WITT_C_FLAGS) -I "$(NDK_GLUE)" -c "$(NDK_GLUE)/android_native_app_glue.c" -o "$(NATIVE_LIBRARY).glue.o"
	"$(NDK_CLANG)" $(NDK_FLAGS) -shared "$(NATIVE_LIBRARY).witt.o" "$(NATIVE_LIBRARY).glue.o" -Wl,--gc-sections -Wl,--no-undefined -Wl,-soname,libwitt.so -landroid -llog -lEGL -lGLESv2 -lm -o "$(NATIVE_LIBRARY)"
	printf 'owned_source_compiler\tICK\nsource_stage\tC-to-assembly\nassembly_and_link\tNDK\nndk_glue_source\tNDK\napi\t%s\n' "$(ANDROID_API)" > "$(NATIVE_LIBRARY).compiler.tsv"
