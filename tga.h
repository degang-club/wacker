#ifndef TGA_H_INCLUDED
#define TGA_H_INCLUDED

#include <stdint.h>

#define TGA_MAP_NOCOLORMAP 0
#define TGA_MAP_COLORMAP 1

#define TGA_IMG_NOIMAGEDATA 0
#define TGA_IMG_COLORMAPPED 1
#define TGA_IMG_TRUECOLOR 2
#define TGA_IMG_BLACKWHITE 3
#define TGA_IMG_RUNLENGTH_COLORMAPPED 9
#define TGA_IMG_RUNLENGTH_TRUECOLOR 10
#define TGA_IMG_RUNLENGTH_BLACKWHITE 11

typedef struct {
	uint16_t entryIndex;
	uint16_t entryLength;
	uint8_t bpp;
} TGA_MAP_SPEC;

typedef struct {
	uint16_t xOrigin;
	uint16_t yOrigin;
	uint16_t width;
	uint16_t height;
	uint8_t pixelSize;
	uint8_t descriptorByte;
} TGA_IMAGE_SPEC;

typedef struct {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
} TGA_COLOR;

typedef struct {
	uint8_t idLength;
	uint8_t mapType;
	uint8_t imageType;
	TGA_MAP_SPEC mapSpec;
	TGA_IMAGE_SPEC imageSpec;
	TGA_COLOR *colorMapData;
	uint8_t *imageData;
} TGA;

int tga_load_file(TGA *tga, char *path);

void tga_free(TGA *tga);

void tga_print_debug(TGA *tga);

#endif // TGA_H_INCLUDED
