#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tga.h"
#include "wad.h"

#define COORDS_TO_INDEX(x, y, width) (y * width + x)

void generate_mipmap(uint8_t *dst, uint8_t *src, uint8_t *color_map, uint16_t *color_map_size, uint32_t width, uint32_t height)
{
	int i;
	uint32_t x, y;
	uint16_t red_average, green_average, blue_average;
	uint8_t smallest_distance_index;
	double current_squared_distance, previous_squared_distance = DBL_MAX;

	for (y=0; y < height / 2; y++) {
		for (x=0; x < width / 2; x++) {
			red_average = green_average = blue_average = 0;
			
			red_average += color_map[src[COORDS_TO_INDEX(x * 2,     y * 2,     width)] * 3]; // 0,0
			red_average += color_map[src[COORDS_TO_INDEX(x * 2 + 1, y * 2,     width)] * 3]; // 1,0
			red_average += color_map[src[COORDS_TO_INDEX(x * 2,     y * 2 + 1, width)] * 3]; // 0,1
			red_average += color_map[src[COORDS_TO_INDEX(x * 2 + 1, y * 2 + 1, width)] * 3]; // 1,1
			red_average = red_average / 4;
			
			green_average += color_map[src[COORDS_TO_INDEX(x * 2,     y * 2,     width)] * 3 + 1]; // 0,0
			green_average += color_map[src[COORDS_TO_INDEX(x * 2 + 1, y * 2,     width)] * 3 + 1]; // 1,0
			green_average += color_map[src[COORDS_TO_INDEX(x * 2,     y * 2 + 1, width)] * 3 + 1]; // 0,1
			green_average += color_map[src[COORDS_TO_INDEX(x * 2 + 1, y * 2 + 1, width)] * 3 + 1]; // 1,1
			green_average = green_average / 4;
			
			blue_average += color_map[src[COORDS_TO_INDEX(x * 2,     y * 2,     width)] * 3 + 2]; // 0,0
			blue_average += color_map[src[COORDS_TO_INDEX(x * 2 + 1, y * 2,     width)] * 3 + 2]; // 1,0
			blue_average += color_map[src[COORDS_TO_INDEX(x * 2,     y * 2 + 1, width)] * 3 + 2]; // 0,1
			blue_average += color_map[src[COORDS_TO_INDEX(x * 2 + 1, y * 2 + 1, width)] * 3 + 2]; // 1,1
			blue_average = blue_average / 4;
			
			/* TODO Add color to color map if not present and find closest color if color map is full */
			for (i=0; i < *color_map_size; i++) {
				if (red_average == color_map[i * 3] && green_average == color_map[i * 3 + 1] && blue_average == color_map[i * 3 + 2]) {
					dst[COORDS_TO_INDEX(x, y, width / 2)] = i;
					break;
				}
			}
			
			if (i == 3) { /* Make sure that 'i' is the correct variable to check*/
				for (i=0; i < *color_map_size; i++) {
					current_squared_distance = pow(red_average - color_map[i * 3], 2)
						+ pow(green_average - color_map[i * 3 + 1], 2)
						+ pow(blue_average - color_map[i * 3 + 2], 2);
					if ( current_squared_distance < previous_squared_distance) {
						previous_squared_distance = current_squared_distance;
						smallest_distance_index = i;
					}
				}
				dst[COORDS_TO_INDEX(x, y, width / 2)] = smallest_distance_index;
			} else if (i == *color_map_size) {
				color_map[*color_map_size * 3] = red_average;
				color_map[*color_map_size * 3 + 1] = green_average;
				color_map[*color_map_size * 3 + 2] = blue_average;
				dst[COORDS_TO_INDEX(x, y, width / 2)] = *color_map_size;
				*color_map_size = *color_map_size + 1; /* Can't do *color_map_index++, because of an unused variable warning*/
			}
		}
	}
}

void add_tga_to_wad(WAD *wad, TGA *tga, char *name)
{
	uint16_t color_map_size = 0;
	int image_size = 0;
	int mipmap_1_size = 0;
	int mipmap_2_size = 0;
	int mipmap_3_size = 0;
	static int texture_count = 0;

	wad->textures[texture_count] = malloc(sizeof(WAD_TEXTURE));
	
	color_map_size = tga->mapSpec.entryLength;

	/* Copy the name, width and height to the WAD3 texture */
	strncpy(wad->textures[texture_count]->texture_name, name, sizeof(wad->textures[texture_count]->texture_name));
	wad->textures[texture_count]->width = tga->imageSpec.width;
	wad->textures[texture_count]->height = tga->imageSpec.height;

	/* Convert TGA color map to a WAD3 color map */
	for (uint8_t i=0; i < tga->mapSpec.entryLength; i++) {
		wad->textures[texture_count]->color_map[i * 3] = tga->colorMapData[i].red;
		wad->textures[texture_count]->color_map[i * 3 + 1] = tga->colorMapData[i].green;
		wad->textures[texture_count]->color_map[i * 3 + 2] = tga->colorMapData[i].blue;
	}

	/* Allocate memory for image and copy from TGA file */
	image_size = wad->textures[texture_count]->width * wad->textures[texture_count]->height; 
	wad->textures[texture_count]->image_data = malloc(image_size);
	memcpy(wad->textures[texture_count]->image_data, tga->imageData, image_size);
	
	/* Allocate memory for mipmap 1 */
	mipmap_1_size = (wad->textures[texture_count]->width / 2) * (wad->textures[texture_count]->height / 2) ;
	wad->textures[texture_count]->mipmap_1_data = malloc(mipmap_1_size);
	generate_mipmap(wad->textures[texture_count]->mipmap_1_data,
				wad->textures[texture_count]->image_data,
				wad->textures[texture_count]->color_map,
				&color_map_size,
				wad->textures[texture_count]->width,
				wad->textures[texture_count]->height);
		
	/* Allocate memory for mipmap 2 */
	mipmap_2_size = (wad->textures[texture_count]->width / 4) * (wad->textures[texture_count]->height / 4) ;
	wad->textures[texture_count]->mipmap_2_data = malloc(mipmap_2_size);
	generate_mipmap(wad->textures[texture_count]->mipmap_2_data,
				wad->textures[texture_count]->mipmap_1_data,
				wad->textures[texture_count]->color_map,
				&color_map_size,
				wad->textures[texture_count]->width / 2,
				wad->textures[texture_count]->height / 2);
		
	/* Allocate memory for mipmap 3 */
	mipmap_3_size = (wad->textures[texture_count]->width / 8) * (wad->textures[texture_count]->height / 8) ;
	wad->textures[texture_count]->mipmap_3_data = malloc(mipmap_3_size);
	generate_mipmap(wad->textures[texture_count]->mipmap_3_data,
				wad->textures[texture_count]->mipmap_2_data,
				wad->textures[texture_count]->color_map,
				&color_map_size,
				wad->textures[texture_count]->width / 4,
				wad->textures[texture_count]->height / 4);
	
	texture_count++;
	wad->textures_count = texture_count;
}

int main()
{
	TGA tga;
	tga_load_file(&tga, "test_3.tga");
	// tga_print_debug(&tga);

	WAD wad;
	add_tga_to_wad(&wad, &tga, "{test");
	printf("%s\n", wad.textures[0]->texture_name);
	return 0;
}

