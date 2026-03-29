#!/bin/sh
make $1
make OUTDIR=arm CC=arm-none-linux-gnueabi-gcc $1
