#include <gsfw/firmware/family/htlegacy.h>
#include <gsfw/firmware/family_defs.h>

int ht5_parse_header(ht5_update_hdr_t* header, gs_update_directory_t* directory) {
    //ht5_update_hdr_t* header = (void*)start;
    directory->filenames = &header->filenames[0];
    directory->sizes = &header->sizes[0];
    directory->versions = &header->versions[0];
    //directory->body = start + GS_HT8_DVF99_FW_BODY_START;
    return 0;
}

void ht5_family_init(gs_family_def_t *family) {
    family->capabilities.img_decrypt = true;
    
    family->methods.fw_parse_header = (void*)ht5_parse_header;
}

int ht7_parse_header(ht7_update_hdr_t* header, gs_update_directory_t* directory) {
    //ht7_update_hdr_t* header = (void*)start;
    directory->filenames = &header->filenames[0];
    directory->sizes = &header->sizes[0];
    directory->versions = &header->versions[0];
    //directory->body = start + GS_HT8_DVF99_FW_BODY_START;
    return 0;
}

void ht7_family_init(gs_family_def_t *family) {
    family->capabilities.img_decrypt = true;

    family->methods.fw_parse_header = (void*)ht7_parse_header;
}
