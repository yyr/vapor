#ifndef JPEGAPI_H
#define JPEGAPI_H
/*
 *  This header defines the external api to the jpeg library.
 */
#include "common.h"

JPEG_EXTERN(int)
write_JPEG_file(FILE * file, int image_width, int image_height, unsigned char* image_buffer,
				 int quality);

#endif //JPEGAPI_H

