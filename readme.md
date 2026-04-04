# libgsfw

library and utilities for reverse engineering and modifying Grandstream firmware. supports multiple device families (emphasizing the HT8xxv1 ones), with varying levels of support for other families.

## tools

- **fwtool** - firmware update swiss army knife: inspect, unpack, decrypt, pack+encrypt, fix checksums, reset rollback bits
- **imgtool** - partition image tool: inspect, extract header/body, fix headers/checksums, patch body

## building

```sh
# native build
make

# cross-compile
PREFIX=arm-none-linux-gnueabi- TARGET=arm make

# build embedded (for virgil)
PREFIX=arm-none-linux-gnueabi- TARGET=arm_embedded EXTRA_CFLAGS=-DLIBGSFW_EMBEDDED=1 make libgsfw
```

binaries are output to `src/native/`.

## firmware patching workflow

```sh
# unpack and decrypt a firmware update
fwtool -d ht818fw.bin

# extract body from a partition image
imgtool -F ht8_dvf101 -u ht818boot.bin

# (...perform desired modifications to ht818boot_body.bin...)

# replace the image body and fix the header
imgtool -F ht8_dvf101 --patch ht818boot_body.bin ht818boot.bin

# pack and encrypt into a new firmware update
fwtool -p -F ht8_dvf101 ht818fw_modified.bin ht818boot.bin ht818core.bin ht818base.bin ht818prog.bin
```

## supported families

| Family | Devices | Capabilities |
|---|---|---|
| ht8_dvf101 | HT818 | Full: unpack, decrypt, pack, checksum, rollback bits |
| ht8_dvf99 | HT801, HT802, HT813 | Unpack, decrypt, pack |
| ht8_rockchip | HT80xv2, HT81xv2 | Unpack only (encryption unsolved) |
| ht7 | HT701, HT702, HT704 | Unpack, decrypt (partial?) |
| ht5 | HT502, HT503 | Unpack, decrypt |

Run `fwtool --families` to see current capabilities.
