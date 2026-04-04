# libgsfw

library and utilities for reverse engineering and modifying Grandstream firmware. supports multiple device families (emphasizing the HT8xxv1 ones), with varying levels of support for other families.

## repo layout

- `include/` - C headers
- `src/` - C source for library
  - `bootrom/` - code for working with DSPG BootROM preloader images
  - `firmware/` - code for working with Grandstream firmware images
  - `shared/` - AES library, libc subset
  - `util.c` - helper functions for utilities
- `tools/` - utilities
- `build/` - build output

## tools

### C tools (src/)

- **fwtool** - firmware update swiss army knife: inspect, unpack, decrypt, pack+encrypt, fix checksums, reset rollback bits. auto-detects device family for inspection/unpack; requires `--family` for pack.
- **imgtool** - partition image tool: inspect, extract header/body, fix headers/checksums, patch body. requires `--family` for all operations (image fingerprinting not yet viable).

## firmware patching workflow

```sh
# unpack + decrypt partition images
fwtool -d firmware.bin

# ... modify images ...

# pack + encrypt into a new firmware update
fwtool -p -F ht8_dvf101 output.bin ht818boot.bin ht818core.bin ht818base.bin ht818prog.bin
```

image-level patching:
```sh
# extract body from a partition image
imgtool -F ht8_dvf101 -u image.bin

# ... modify body ...

# replace body and fix header
imgtool -F ht8_dvf101 --patch new_body.bin image.bin
```

## family architecture

device support is organized by family. each family has a `gs_family_def_t` in `firmware/family_defs.c` with:

- format constants (magic, header offsets, crypto parameters)
- method pointers in `gs_family_methods_t` for family-specific behavior

families leave method pointers NULL for unsupported features. code that calls family methods must check for NULL and handle gracefully (warn or skip).

per-family implementations live in their own files:
- `firmware/ht8xx_dvf101.c` - HT818 (most complete: infodump, directory, checksum, support bits, crypto, pack)
- `firmware/ht8xx.c` - HT8xx v1 (non-818) and v2 rockchip
- `firmware/htlegacy.c` - HT5xx and HT7xx

auto-detection works via `gs_family_fw_fingerprint()` which checks magic number and first filename against all known families. image-level fingerprinting is not yet implemented (img_magic is ambiguous across ht8xx variants).

## supported families

| family | devices | status |
|---|---|---|
| ht8_dvf101 | HT818 | full support (unpack, decrypt, pack, checksum, rollback bits) |
| ht8_dvf99 | HT801, HT802, HT813 | unpack + decrypt + pack |
| ht8_rockchip | HT80xv2, HT81xv2 | unpack only (encryption unsolved) |
| ht7 | HT701, HT702, HT704 | unpack + decrypt |
| ht5 | HT502, HT503 | unpack + decrypt |
