#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wad.h"

WAD wad_init(void)
{
	return (WAD){.textures_count = 0, .lumps_offset = 0, .textures = 0, .lumps = 0};
}

int wad_update(WAD *wad)
{
	int i;
	uint64_t image_size;

	wad->lumps_offset = 0x0c;

	/* Allocate memory for the lumps */
	if (!wad->lumps)
		wad->lumps = malloc(sizeof(&wad->lumps) * wad->textures_count);
	else
		return 1; /* Do something here to be able to update the lumps multiple times */

	for (i=0; i < wad->textures_count; i++) {
		wad->lumps[i] = malloc(sizeof(WAD_LUMP_ITEM));

		wad->lumps[i]->texture_offset = wad->lumps_offset;
		
		image_size = wad->textures[i]->width * wad->textures[i]->height;

		/* Calculate the size */
		wad->lumps[i]->texture_size = 16 /* Texture name size */
			+ 4 + 4 /* Width and height */
			+ 4 + 4 + 4 + 4 /* The offsets */
			+ image_size
			+ image_size / 4
			+ image_size / 16
			+ image_size / 64
			+ 1 + 1 /* The two unknown bytes */
			+ 768 /* The color map */
			+ 2 /* Padding */;
		
		wad->lumps[i]->texture_compressed_size = wad->lumps[i]->texture_size;

		wad->lumps_offset += wad->lumps[i]->texture_size;

		wad->lumps[i]->texture_type = WAD_TEXTURE_TYPE_MIPTEX;
		wad->lumps[i]->compression_type = 0;
		wad->lumps[i]->padding = 0;

		/* Calculate offsets */
		wad->textures[i]->image_offset = 40; /* The header in front of the texture is always 40 bytes */
		wad->textures[i]->mipmap_1_offset = wad->textures[i]->image_offset + image_size;
		wad->textures[i]->mipmap_2_offset = wad->textures[i]->mipmap_1_offset + image_size / 4;
		wad->textures[i]->mipmap_3_offset = wad->textures[i]->mipmap_2_offset + image_size / 16;

		wad->textures[i]->unknown_byte_1 = 0x00;
		wad->textures[i]->unknown_byte_2 = 0x01;

		wad->textures[i]->padding = 0;
	}
	
	return 0;
}

/* NOTE This code will probably create bad WAD files on big-endian systems */
int wad_save_file(WAD *wad, char *path) {
	FILE *f;
	int i;
	uint64_t image_size;
	
	/* Check if there are lumps */
	if (!wad->lumps) {
		return 1;
	}

	f = fopen(path, "w");

	fwrite(WAD_MAGIC, 4, 1, f);
	fwrite(&wad->textures_count, sizeof(wad->textures_count), 1, f);

	/* We need to set this after we've gone through all the textures, then we'll know where the lumps begin */
	fwrite(&wad->lumps_offset, sizeof(wad->lumps_offset), 1, f); 

	for (i=0; i < wad->textures_count; i++) {
		fwrite(wad->textures[i]->texture_name, 16, 1, f);
		fwrite(&wad->textures[i]->width, sizeof(wad->textures[i]->width), 1, f);
		fwrite(&wad->textures[i]->height, sizeof(wad->textures[i]->height), 1, f);

		/* Write offsets to WAD file */
		fwrite(&wad->textures[i]->image_offset, sizeof(wad->textures[i]->image_offset), 1, f);
		fwrite(&wad->textures[i]->mipmap_1_offset, sizeof(wad->textures[i]->mipmap_1_offset), 1, f);
		fwrite(&wad->textures[i]->mipmap_2_offset, sizeof(wad->textures[i]->mipmap_2_offset), 1, f);
		fwrite(&wad->textures[i]->mipmap_3_offset, sizeof(wad->textures[i]->mipmap_3_offset), 1, f);

		image_size = wad->textures[i]->width * wad->textures[i]->height;
		
		/* Write image and mimmap data to WAD file*/
		fwrite(wad->textures[i]->image_data, image_size, 1, f);
		fwrite(wad->textures[i]->mipmap_1_data, image_size / 4, 1, f);
		fwrite(wad->textures[i]->mipmap_2_data, image_size / 16, 1, f);
		fwrite(wad->textures[i]->mipmap_3_data, image_size / 64, 1, f);

		/* Write the unknown bytes */
		fwrite(&wad->textures[i]->unknown_byte_1, sizeof(wad->textures[i]->unknown_byte_1), 1, f);
		fwrite(&wad->textures[i]->unknown_byte_2, sizeof(wad->textures[i]->unknown_byte_2), 1, f);

		/* Write the color map */
		fwrite(&wad->textures[i]->color_map, sizeof(wad->textures[i]->color_map), 1, f);

		/* Write padding (2 bytes) */
		fwrite(&wad->textures[i]->padding, sizeof(wad->textures[i]->padding), 1, f);

		memcpy(wad->lumps[i]->texture_name, wad->textures[i]->texture_name, sizeof(wad->textures[i]->texture_name));
	}

	for (i=0; i < wad->textures_count; i++) {
		fwrite(&wad->lumps[i]->texture_offset, sizeof(wad->lumps[i]->texture_offset), 1, f);

		fwrite(&wad->lumps[i]->texture_compressed_size, sizeof(wad->lumps[i]->texture_compressed_size), 1, f);
		fwrite(&wad->lumps[i]->texture_size, sizeof(wad->lumps[i]->texture_size), 1, f);

		fwrite(&wad->lumps[i]->texture_type, sizeof(wad->lumps[i]->texture_type), 1, f);
		fwrite(&wad->lumps[i]->compression_type, sizeof(wad->lumps[i]->compression_type), 1, f);

		fwrite(&wad->lumps[i]->padding, sizeof(wad->lumps[i]->padding), 1, f);

		fwrite(&wad->lumps[i]->texture_name, sizeof(wad->lumps[i]->texture_name), 1, f);
	}

	fclose(f);

	return 0;
}

void wad_free(WAD *wad)
{
	for (int i=0; i < wad->textures_count; i++) {
		free(wad->lumps[i]);
		free(wad->textures[i]);
	}

	free(wad->lumps);
	free(wad->textures);
}
