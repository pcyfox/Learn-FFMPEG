#!/bin/bash
NDK=/Users/mac/NDK/android-ndk-r21d
ADDI_LDFLAGS="-fPIE -pie"
ADDI_CFLAGS="-fPIE -pie -march=armv7-a -mfloat-abi=softfp -mfpu=neon"
CPU=armv7-a
ARCH=arm
HOST=arm-linux
SYSROOT=$NDK/toolchains/llvm/prebuilt/darwin-x86_64/sysroot
TOOLCHAIN=$NDK/toolchains/llvm/prebuilt/darwin-x86_64/bin

PREFIX=$(pwd)/android/$CPU

configure()
{
    ./configure \
    --prefix=$PREFIX \
    --toolchain=clang-usan \
    --enable-cross-compile \
    --target-os=android \
    --arch=$ARCH \
    --sysroot=$SYSROOT \
    --cc=$TOOLCHAIN/armv7a-linux-androideabi21-clang \
    --cxx=$TOOLCHAIN/armv7a-linux-androideabi21-clang++ \
    --strip=$TOOLCHAIN/arm-linux-androideabi-strip \

    --extra-cflags="$ADDI_CFLAGS" \
    --extra-ldflags="$ADDI_LDFLAGS" \


    --disable-static \
    --enable-shared \
    --enable-small \
    --enable-neon \
    --disable-programs \
    --disable-ffmpeg \
    --disable-ffplay \
    --disable-ffprobe \
    --disable-doc \
    --disable-symver \
    --disable-asm \ 
    --disable-avdevice \
    --disable-symver \
    --disable-ffprobe \
    

}

build()
{
    configure
    make clean
    make -j8
    make install
}

build
