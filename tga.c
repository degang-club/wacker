#include <stdio.h>
#include <stdlib.h>
#include "tga.h"

int tga_load_file(TGA *tga, char *path)
{
	FILE *f_tga;
	int i;
	f_tga = fopen(path, "r");
	
	fread(&tga->idLength, sizeof(tga->idLength), 1, f_tga);
	fread(&tga->mapType, sizeof(tga->mapType), 1, f_tga);
	fread(&tga->imageType, sizeof(tga->imageType), 1, f_tga);
	
	fread(&tga->mapSpec.entryIndex, sizeof(tga->mapSpec.entryIndex), 1, f_tga);
	fread(&tga->mapSpec.entryLength, sizeof(tga->mapSpec.entryLength), 1, f_tga);
	fread(&tga->mapSpec.bpp, sizeof(tga->mapSpec.bpp), 1, f_tga);
	
	fread(&tga->imageSpec.xOrigin, sizeof(tga->imageSpec.xOrigin), 1, f_tga);
	fread(&tga->imageSpec.yOrigin, sizeof(tga->imageSpec.yOrigin), 1, f_tga);
	fread(&tga->imageSpec.width, sizeof(tga->imageSpec.width), 1, f_tga);
	fread(&tga->imageSpec.height, sizeof(tga->imageSpec.height), 1, f_tga);
	fread(&tga->imageSpec.pixelSize, sizeof(tga->imageSpec.pixelSize), 1, f_tga);
	fread(&tga->imageSpec.descriptorByte, sizeof(tga->imageSpec.descriptorByte), 1, f_tga);

	tga->colorMapData = malloc(tga->mapSpec.entryLength * sizeof(TGA_COLOR));
	for (i=0; i < tga->mapSpec.entryLength; i++) {
		fread(&tga->colorMapData[i].blue, sizeof(tga->colorMapData[i].blue), 1, f_tga);
		fread(&tga->colorMapData[i].green, sizeof(tga->colorMapData[i].green), 1, f_tga);
		fread(&tga->colorMapData[i].red, sizeof(tga->colorMapData[i].red), 1, f_tga);
	}

	tga->imageData = malloc(tga->imageSpec.width * tga->imageSpec.height);
	for (i=0; i < tga->imageSpec.width * tga->imageSpec.height; i++) {
		fread(&tga->imageData[i], sizeof(tga->imageData[i]), 1, f_tga);
	}
	
	return 0;
}

void tga_free(TGA *tga)
{
	free(tga->colorMapData);
	free(tga->imageData);
}

void tga_print_debug(TGA *tga)
{
	int i;

	printf("TGA FILE\n");
	printf("	ID Length:	0x%02x\n", tga->idLength);

	printf("	Map Type:	");
	switch(tga->mapType) {
		case TGA_MAP_NOCOLORMAP:
			printf("No Color Map\n");
			break;
		case TGA_MAP_COLORMAP:
			printf("Color Map\n");
			break;
		default:
			printf("Undefined maptype!\n");
			break;
	}
	
	printf("	Image Type:	");
	switch(tga->mapType) {
		case TGA_IMG_NOIMAGEDATA:
			printf("No image data\n");
			break;
		case TGA_IMG_COLORMAPPED:
			printf("Colormapped\n");
			break;
		case TGA_IMG_TRUECOLOR:
			printf("Truecolor\n");
			break;
		case TGA_IMG_BLACKWHITE:
			printf("Black & white\n");
			break;
		case TGA_IMG_RUNLENGTH_COLORMAPPED:
			printf("Runlength-encoded colormapped\n");
			break;
		case TGA_IMG_RUNLENGTH_TRUECOLOR:
			printf("Runlength-encoded truecolor\n");
			break;
		case TGA_IMG_RUNLENGTH_BLACKWHITE:
			printf("Runlength-encoded black & white\n");
			break;
		default:
			printf("Undefined maptype!\n");
	}

	printf("\tEntry Index:\t%d (0x%04x)\n", tga->mapSpec.entryIndex, tga->mapSpec.entryIndex);
	printf("\tEntry Length:\t%d (0x%04x)\n", tga->mapSpec.entryLength, tga->mapSpec.entryLength);
	printf("\tBits-Per-Pixel:\t%d (%d bytes)\n", tga->mapSpec.bpp, tga->mapSpec.bpp / 8);

	printf("\tX Origin:\t%d (0x%04x)\n", tga->imageSpec.xOrigin, tga->imageSpec.xOrigin);
	printf("\tY Origin:\t%d (0x%04x)\n", tga->imageSpec.yOrigin, tga->imageSpec.yOrigin);
	printf("\tWidth:\t\t%d (0x%04x)\n", tga->imageSpec.width, tga->imageSpec.width);
	printf("\tHeight:\t\t%d (0x%04x)\n", tga->imageSpec.height, tga->imageSpec.height);
	printf("\tPixel Size:\t%d (0x%04x)\n", tga->imageSpec.pixelSize, tga->imageSpec.pixelSize);
	printf("\tDescriptor Byte:%d (0x%04x)\n", tga->imageSpec.descriptorByte, tga->imageSpec.descriptorByte);

	printf("\tColor Map:\n");
	for (i=0; i < tga->mapSpec.entryLength; i++) {
		printf("\t\tR: 0x%02x G: 0x%02x B: 0x%02x\n", tga->colorMapData[i].red,
			tga->colorMapData[i].green, tga->colorMapData[i].blue);
	}

	for (i=0; i < tga->imageSpec.width * tga->imageSpec.height; i++) {
		printf("0x%02x \n", tga->imageData[i]);
	}
}
