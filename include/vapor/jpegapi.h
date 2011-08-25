#ifndef JPEGAPI_H
#define JPEGAPI_H
/*
 *  This header defines the external api to the jpeg library.
 */

namespace VAPoR {

int write_JPEG_file(
	FILE * file, int image_width, int image_height, 
	unsigned char* image_buffer, int quality
);

int read_JPEG_file(const char* filename, unsigned char** imageBuffer, int* width, int* height);

//(void) free_image(unsigned char* imageData);

};

#endif //JPEGAPI_H

