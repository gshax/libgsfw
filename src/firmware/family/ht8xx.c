#include <gsfw/firmware/family/ht8xx.h>
#include <gsfw/firmware/family_defs.h>
#include <gsfw/libc.h>

int ht8_dvf99_fw_parse_header(ht8_v1_update_hdr_t* header, gs_update_directory_t* directory) {
    //ht8_v1_update_hdr_t* header = (void*)start;
    directory->filenames = &header->filenames[0];
    directory->sizes = &header->sizes[0];
    directory->versions = &header->versions[0];
    //directory->body = start + GS_HT8_DVF99_FW_BODY_START;
    return 0;
}

int ht8_dvf99_fw_build_header(ht8_v1_update_hdr_t* hdr,
    gs_update_directory_t* dir, void* first_img)
{
    (void)first_img;
    hdr->magic = GS_HT8_DVF99_FW_MAGIC;
    c_memcpy(hdr->filenames, dir->filenames, sizeof(hdr->filenames));
    c_memcpy(hdr->sizes,     dir->sizes,     sizeof(hdr->sizes));
    c_memcpy(hdr->versions,  dir->versions,  sizeof(hdr->versions));
    return 0;
}

void ht8_dvf99_family_init(gs_family_def_t *family) {
    family->capabilities.img_decrypt = true;
    family->capabilities.img_encrypt = true;

    family->methods.fw_parse_header = (void*)ht8_dvf99_fw_parse_header;
    family->methods.fw_build_header = (void*)ht8_dvf99_fw_build_header;
}

int ht8_rockchip_parse_header(ht8_v2_update_hdr_t* header, gs_update_directory_t* directory) {
    //ht8_v2_update_hdr_t* header = (void*)(start + GS_HT8_ROCKCHIP_FW_START);
    directory->filenames = &header->filenames[0];
    directory->sizes = &header->sizes[0];
    directory->versions = &header->versions[0];
    //directory->body = start + GS_HT8_ROCKCHIP_FW_START + GS_HT8_ROCKCHIP_FW_BODY_START;
    return 0;
}

void ht8_rockchip_family_init(gs_family_def_t *family) {
    family->methods.fw_parse_header = (void*)ht8_rockchip_parse_header;
}