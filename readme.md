# fwtool

Utilities for working with Grandstream device firmware. Currently only supports HT818v1.

### High-level commands

- `gs_fwunpack` - unpack `ht818fw.bin` to decrypted images
- `gs_fwpack` - pack `ht818fw.bin` from decrypted images
- `gs_imgpatch` - replace the body of a decrypted image

### Low-level commands

- `gs_fwtool` - HT818 firmware update analysis and patching utility
- `gs_fwmkhdr` - HT818 firmware update header generator
- `gs_imgcrypt` - decrypt/encrypt HT818 partition images
- `gs_imgtool` - HT818 partition image analysis and patching utility

## Requirements

- Linux
- Working toolchain for building C programs
- Fairly recent NodeJS

Development and testing was done on Gentoo with GCC 14 and Node 23.

## Setup

`./init.sh` should automatically initialize the repo, install the NPM dependencies, and build all the C tools. If you'd rather do that yourself, just read the script.

## Firmware patching workflow

```sh
# get environment ready
./init.sh
. env.sh

# unpack the firmware update
gs_fwunpack ht818fw.bin

# patching an image
gs_imgtool -u ht818boot.bin
# ...perform desired modifications to ht818boot.img...
gs_imgpatch ht818boot.bin ht818boot.img

# rebuild the firmware update
gs_fwpack ht818fw_modified.bin
```