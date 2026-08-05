#ifndef JPEG_ENCODER_H
#define JPEG_ENCODER_H

#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <jpeglib.h>

u8* Encode_JPEG(const u16 *buffer, int width, int height, int quality, size_t *out_size);

#endif
