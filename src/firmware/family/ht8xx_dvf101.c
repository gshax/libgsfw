#include <gsfw/firmware/family/ht8xx_dvf101.h>
#include <gsfw/firmware/family_defs.h>
#include <gsfw/libc.h>

#ifndef LIBGSFW_EMBEDDED
#include <stdio.h>

void ht8_dvf101_fw_infodump(ht8_dvf101_update_hdr_t* header) {
    // print header information
    fprintf(stderr, "oem id:\t\t%02x\n", header->oem_id);
    fprintf(stderr, "support bits:\t%04x %04x %04x %04x\n",
        header->support_bits[0], header->support_bits[1],
        header->support_bits[2], header->support_bits[3]);
    fprintf(stderr, "fw v mask:\t%04x\n", header->v_mask);
    fprintf(stderr, "encryption:\t%04x\n", header->encryption_type);

    // print checksum
    fprintf(stderr, "\nheader size:\t%08x bytes (check: %04x bytes)",
        header->header_size, GS_HT8_DVF101_FW_CHECK_SIZE);
    /*LOGV((realsum == header->model_header.checksum) ? GRN : RED,
        "\nchecksum:\t%04x (expected: %04x)\n\n", header->model_header.checksum, realsum);*/
    fprintf(stderr, "\nchecksum:\t%04x\n\n",
        header->checksum);
}

void ht8_dvf101_img_infodump(ht8_dvf101_image_hdr_t* header) {
    gs_version_t version = header->version;
    gs_timestamp_t ts = header->timestamp;

    fprintf(stderr, "image id:\t%02x\n", header->id);

    // image version/timestamp
    fprintf(stderr, "version:\t%u.%u.%u\n",
        version.major, version.minor, version.revision);
    fprintf(stderr, "build date:\t%04u-%02u-%02u %02u:%02u\n",
        ts.year, ts.month, ts.day, ts.hour, ts.minute);

    // print hardware and oem id
    fprintf(stderr, "\nhw id:\t\t%04x\n", header->hw_id);
    fprintf(stderr, "oem id:\t\t%04x\n", header->oem_id);
    
    // supbits flags
    fprintf(stderr, "support bits:\t%04x %04x %04x %04x\n",
        header->support_bits[0], header->support_bits[1],
        header->support_bits[2], header->support_bits[3]);

    // "compatible images"
    fprintf(stderr, "compat ver 0:\t%u.%u.%u\n",
        header->compat_version[0].major,
        header->compat_version[0].minor,
        header->compat_version[0].revision);
    fprintf(stderr, "compat ver 1:\t%u.%u.%u\n",
        header->compat_version[1].major,
        header->compat_version[1].minor,
        header->compat_version[1].revision);
    
    // additional unknown fields
    fprintf(stderr, "\nfw v mask:\t%04x\n", header->v_mask);
    fprintf(stderr, "pad size:\t%08x\n", header->pad_size);

    // provision counter, if it's >0 (dumped from live device)
    if (header->prov_counter > 0) {
        fprintf(stderr, "\nflash counter:\t%08x\n", header->prov_counter);
    }

    fprintf(stderr, "\nimage length:\t%08x\n", header->size_image);
    fprintf(stderr, "body start:\t%08x\n", header->start);

    // handle invalid sizes
    fprintf(stderr, "body length:\t%08x\n", header->size);

    // print current and expected checksum values
    fprintf(stderr, "checksum:\t%04x\n", header->checksum);
}
#endif // LIBGSFW_EMBEDDED

int ht8_dvf101_fw_parse_header(ht8_dvf101_update_hdr_t* header, gs_update_directory_t* directory) {
    //ht8_dvf101_update_hdr_t* header = (void*)start;
    directory->filenames = &header->filenames[0];
    directory->sizes = &header->sizes[0];
    directory->versions = &header->versions[0];
    //directory->body = start + GS_HT8_DVF101_FW_BODY_START;
    return 0;
}

int ht8_dvf101_fw_get_checksum(ht8_dvf101_update_hdr_t* header, uint16_t** checksum_field) {
    *checksum_field = &header->checksum;
    return 0;
}

int ht8_dvf101_fw_fix_support_bits(ht8_dvf101_update_hdr_t* header) {
    header->support_bits[0] = 0;
    header->support_bits[1] = 0;
    header->support_bits[2] = 0;
    header->support_bits[3] = 1;
    return 0;
}

int ht8_dvf101_img_get_checksum(ht8_dvf101_image_hdr_t* header, size_t file_size,
    uint16_t** stored_out, void** body_out, size_t* body_size_out)
{
    *stored_out = &header->checksum;
    *body_out = (char*)header + header->start;
    size_t actual = file_size - header->start;
    *body_size_out = (header->size < actual) ? header->size : actual;
    return 0;
}

int ht8_dvf101_img_fix_support_bits(ht8_dvf101_image_hdr_t* header) {
    header->support_bits[0] = 0;
    header->support_bits[1] = 0;
    header->support_bits[2] = 0;
    header->support_bits[3] = 1;
    return 0;
}

int ht8_dvf101_fw_build_header(ht8_dvf101_update_hdr_t* hdr,
    gs_update_directory_t* dir, ht8_dvf101_image_hdr_t* first_img)
{
    hdr->magic = GS_HT8_DVF101_FW_MAGIC;
    c_memcpy(hdr->filenames, dir->filenames, sizeof(hdr->filenames));
    c_memcpy(hdr->sizes,     dir->sizes,     sizeof(hdr->sizes));
    c_memcpy(hdr->versions,  dir->versions,  sizeof(hdr->versions));
    c_memcpy(hdr->support_bits, first_img->support_bits, sizeof(hdr->support_bits));
    hdr->v_mask      = first_img->v_mask;
    hdr->oem_id      = first_img->oem_id;
    hdr->header_size = GS_HT8_DVF101_FW_BODY_START;
    hdr->encryption_type = 0;
    return 0;
}

int ht8_dvf101_img_set_body_size(ht8_dvf101_image_hdr_t* hdr, size_t body_size, size_t file_size) {
    (void)file_size;
    hdr->size = body_size;
    return 0;
}

void ht8_dvf101_family_init(gs_family_def_t *family) {
    family->capabilities.img_decrypt = true;
    family->capabilities.img_encrypt = true;

#ifndef LIBGSFW_EMBEDDED
    family->methods.fw_infodump = (void*)ht8_dvf101_fw_infodump;
    family->methods.img_infodump = (void*)ht8_dvf101_img_infodump;
#endif
    family->methods.fw_parse_header = (void*)ht8_dvf101_fw_parse_header;
    family->methods.fw_get_checksum = (void*)ht8_dvf101_fw_get_checksum;
    family->methods.fw_fix_support_bits = (void*)ht8_dvf101_fw_fix_support_bits;
    family->methods.img_get_checksum = (void*)ht8_dvf101_img_get_checksum;
    family->methods.img_fix_support_bits = (void*)ht8_dvf101_img_fix_support_bits;
    family->methods.img_set_body_size = (void*)ht8_dvf101_img_set_body_size;
    family->methods.fw_build_header = (void*)ht8_dvf101_fw_build_header;
}
