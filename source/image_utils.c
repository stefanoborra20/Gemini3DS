#include "image_utils.h"

u8* Encode_JPEG(const u16 *buffer, int width, int height, int quality, size_t *out_size) {
    if (!buffer || !out_size) return NULL;

    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    u8 *jpeg_buffer = NULL;
    unsigned long jpeg_size = 0;
    jpeg_mem_dest(&cinfo, &jpeg_buffer, &jpeg_size);

    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, TRUE);

    jpeg_start_compress(&cinfo, TRUE);

    u8 *row_buffer = (u8 *)malloc(width * 3);
    if (!row_buffer) {
        jpeg_destroy_compress(&cinfo);
        return NULL;
    }
    JSAMPROW row_pointer[1];
    row_pointer[0] = row_buffer;

    while (cinfo.next_scanline < cinfo.image_height) {
        int y = cinfo.next_scanline;
        
        for (int x = 0; x < width; x++) {
            u16 px = buffer[y * width + x];
            
            row_buffer[x * 3 + 0] = ((px >> 11) & 0x1F) * 255 / 31; // Red
            row_buffer[x * 3 + 1] = ((px >> 5)  & 0x3F) * 255 / 63; // Green
            row_buffer[x * 3 + 2] = (px         & 0x1F) * 255 / 31; // Blue
        }
        
        jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }

    jpeg_finish_compress(&cinfo);
    free(row_buffer);
    jpeg_destroy_compress(&cinfo);

    *out_size = (size_t)jpeg_size;
    return jpeg_buffer;
}
