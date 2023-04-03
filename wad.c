#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wad.h"

/* NOTE This code will probably create bad WAD files on big-endian systems */
int wad_save_file(WAD *wad, char *path) {
	FILE *f;
	uint64_t image_size;
	int i;
	
	/* Allocate memory for the lumps */
	wad->lumps = malloc(sizeof(&wad->lumps) * wad->textures_count);
	
	f = fopen(path, "w");
		
	fwrite(WAD_MAGIC, 4, 1, f);
	fwrite(&wad->textures_count, sizeof(wad->textures_count), 1, f);
	
	/* We need to set this after we've gone through all the textures, then we'll know where the lumps begin */
	fwrite(&wad->lumps_offset, sizeof(wad->lumps_offset), 1, f); 
	
	for (i=0; i < wad->textures_count; i++) {
		wad->lumps[i] = malloc(sizeof(WAD_LUMP_ITEM));
		wad->lumps[i]->texture_offset = ftell(f);
		
		fwrite(wad->textures[i]->texture_name, 16, 1, f);
		fwrite(&wad->textures[i]->width, sizeof(wad->textures[i]->width), 1, f);
		fwrite(&wad->textures[i]->height, sizeof(wad->textures[i]->height), 1, f);
		
		image_size = wad->textures[i]->width * wad->textures[i]->height;
		
		/* Calculate offsets */
		wad->textures[i]->image_offset = 40; /* The header in front of the texture is always 40 bytes */
		wad->textures[i]->mipmap_1_offset = wad->textures[i]->image_offset + image_size;
		wad->textures[i]->mipmap_2_offset = wad->textures[i]->mipmap_1_offset + image_size / 4;
		wad->textures[i]->mipmap_3_offset = wad->textures[i]->mipmap_2_offset + image_size / 16;
		
		/* Write offsets to WAD file */
		fwrite(&wad->textures[i]->image_offset, sizeof(wad->textures[i]->image_offset), 1, f);
		fwrite(&wad->textures[i]->mipmap_1_offset, sizeof(wad->textures[i]->mipmap_1_offset), 1, f);
		fwrite(&wad->textures[i]->mipmap_2_offset, sizeof(wad->textures[i]->mipmap_2_offset), 1, f);
		fwrite(&wad->textures[i]->mipmap_3_offset, sizeof(wad->textures[i]->mipmap_3_offset), 1, f);
		
		/* Write image and mimmap data to WAD file*/
		fwrite(wad->textures[i]->image_data, image_size, 1, f);
		fwrite(wad->textures[i]->mipmap_1_data, image_size / 4, 1, f);
		fwrite(wad->textures[i]->mipmap_2_data, image_size / 16, 1, f);
		fwrite(wad->textures[i]->mipmap_3_data, image_size / 64, 1, f);
		
		/* Write the unknown bytes */
		wad->textures[i]->unknown_byte_1 = 0x00;
		wad->textures[i]->unknown_byte_2 = 0x01;
		fwrite(&wad->textures[i]->unknown_byte_1, sizeof(wad->textures[i]->unknown_byte_1), 1, f);
		fwrite(&wad->textures[i]->unknown_byte_2, sizeof(wad->textures[i]->unknown_byte_2), 1, f);
		
		/* Write the color map */
		fwrite(&wad->textures[i]->color_map, sizeof(wad->textures[i]->color_map), 1, f);
		
		/* Write padding (2 bytes) */
		wad->textures[i]->padding = 0;
		fwrite(&wad->textures[i]->padding, sizeof(wad->textures[i]->padding), 1, f);
		
		/* Calculate the size using the current index of the file */
		wad->lumps[i]->texture_size = ftell(f) - wad->lumps[i]->texture_offset;
		wad->lumps[i]->texture_compressed_size = wad->lumps[i]->texture_size;
		
		wad->lumps[i]->texture_type = WAD_TEXTURE_TYPE_MIPTEX;
		wad->lumps[i]->compression_type = 0;
		wad->lumps[i]->padding = 0;
		
		memcpy(wad->lumps[i]->texture_name, wad->textures[i]->texture_name, sizeof(wad->textures[i]->texture_name));
	}
	
	wad->lumps_offset = ftell(f);
	
	for (i=0; i < wad->textures_count; i++) {
		fwrite(&wad->lumps[i]->texture_offset, sizeof(wad->lumps[i]->texture_offset), 1, f);
		
		fwrite(&wad->lumps[i]->texture_compressed_size, sizeof(wad->lumps[i]->texture_compressed_size), 1, f);
		fwrite(&wad->lumps[i]->texture_size, sizeof(wad->lumps[i]->texture_size), 1, f);
		
		fwrite(&wad->lumps[i]->texture_type, sizeof(wad->lumps[i]->texture_type), 1, f);
		fwrite(&wad->lumps[i]->compression_type, sizeof(wad->lumps[i]->compression_type), 1, f);
		
		fwrite(&wad->lumps[i]->padding, sizeof(wad->lumps[i]->padding), 1, f);
		
		fwrite(&wad->lumps[i]->texture_name, sizeof(wad->lumps[i]->texture_name), 1, f);
	}
	
	fseek(f, 0x08, SEEK_SET);
	fwrite(&wad->lumps_offset, sizeof(wad->lumps_offset), 1, f);
	
	fclose(f);
	
	return 0;
}
