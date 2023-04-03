#ifndef WAD_H_INCLUDED
#define WAD_H_INCLUDED

#include <stdint.h>

#define WAD_MAGIC "WAD3"

#define WAD_TEXTURE_TYPE_QPIC 	0x42
#define WAD_TEXTURE_TYPE_MIPTEX 0x43
#define WAD_TEXTURE_TYPE_FONT 	0x45

typedef struct {
	uint32_t texture_offset;
	
	uint32_t texture_compressed_size;
	uint32_t texture_size;

	uint8_t texture_type;
	uint8_t compression_type;
	
	uint16_t padding;
	
	char texture_name[16];
} WAD_LUMP_ITEM;

typedef struct {
	char texture_name[16];

	uint32_t width;
	uint32_t height;
	
	uint32_t image_offset;   /* Offset relative to imageData */
	uint32_t mipmap_1_offset; /* Offset relative to mipmap1Data */ 
	uint32_t mipmap_2_offset; /* Offset relative to mipmap2Data */
	uint32_t mipmap_3_offset; /* Offset relative to mipmap3Data */
	
	uint8_t *image_data;   /* size = width * height */ 
	uint8_t *mipmap_1_data; /* size = (width / 2) * (height / 2) */ 
	uint8_t *mipmap_2_data; /* size = (width / 4) * (height / 4) */ 
	uint8_t *mipmap_3_data; /* size = (width / 8) * (height / 8) */ 
	
	uint8_t unknown_byte_1; /* Unknown byte, always 0x00 */
	uint8_t unknown_byte_2; /* Unknown byte, always 0x01 */
	
	uint8_t color_map[256*3];
	
	uint16_t padding;
} WAD_TEXTURE;

typedef struct {
	uint32_t textures_count;
	uint32_t lumps_offset;
	WAD_TEXTURE **textures;
	WAD_LUMP_ITEM **lumps;
} WAD;

void wad_init(WAD *wad);

void wad_free(WAD *wad);

int wad_save_file(WAD *wad, char *path);

#endif // WAD_H_INCLUDED
