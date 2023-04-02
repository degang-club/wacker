#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tga.h"
#include "wad.h"

#define COORDS_TO_INDEX(x, y, width) (y * width + x)

// Define color channels
#define RED 0
#define GREEN 1
#define BLUE 2

void sample_colormap(uint16_t *average, uint32_t pixel_index, uint32_t width, uint8_t *src, uint8_t *color_map, uint8_t color_channel) {
	*average += color_map[src[pixel_index			 ] * 3 + color_channel];
	*average += color_map[src[pixel_index + 1		 ] * 3 + color_channel];
	*average += color_map[src[pixel_index + width	 ] * 3 + color_channel];
	*average += color_map[src[pixel_index + width + 1] * 3 + color_channel];
	*average /= 4;
}

void generate_mipmap(uint8_t *dst, uint8_t *src, uint8_t *color_map, uint16_t *color_map_size, uint32_t width, uint32_t height)
{
	int i;
	uint32_t x, y;
	uint16_t red_average, green_average, blue_average;
	uint8_t smallest_distance_index;
	double current_squared_distance, previous_squared_distance;

	uint16_t mip_height = height / 2, mip_width = width / 2;

	for (y=0; y < mip_height; y++) {
		for (x=0; x < mip_width; x++) {
			red_average = green_average = blue_average = 0;

			uint32_t src_pixel_index = COORDS_TO_INDEX(x * 2, y * 2, width);
			uint32_t mip_pixel_index = COORDS_TO_INDEX(x, y, mip_width);

			sample_colormap(&red_average, 	src_pixel_index, width, src, color_map, RED);
			sample_colormap(&green_average, src_pixel_index, width, src, color_map, GREEN);
			sample_colormap(&blue_average, 	src_pixel_index, width, src, color_map, BLUE);

			// Check if color is present in the color pallete
			for (i=0; i < *color_map_size; i++) {
				if (red_average == color_map[i * 3 + RED] && green_average == color_map[i * 3 + GREEN] && blue_average == color_map[i * 3 + BLUE]) {
					dst[mip_pixel_index] = i;
					break;
				}
			}
			
			// Check for closest color in color pallete when exact color was not found and the color pallete is full
			if (i == 256) { /* Make sure that 'i' is the correct variable to check*/
				previous_squared_distance = DBL_MAX;

				for (i=0; i < *color_map_size; i++) {
					current_squared_distance =
						  pow(red_average - color_map[i * 3 + RED], 2)
						+ pow(green_average - color_map[i * 3 + GREEN], 2)
						+ pow(blue_average - color_map[i * 3 + BLUE], 2);
					if ( current_squared_distance < previous_squared_distance) {
						previous_squared_distance = current_squared_distance;
						smallest_distance_index = i;
					}
				}
				dst[mip_pixel_index] = smallest_distance_index;
			// Add color to color pallete
			} else if (i == *color_map_size) {
				color_map[*color_map_size * 3 + RED] 	= red_average;
				color_map[*color_map_size * 3 + GREEN] 	= green_average;
				color_map[*color_map_size * 3 + BLUE] 	= blue_average;
				dst[mip_pixel_index] = *color_map_size;
				*color_map_size += 1; /* Can't do *color_map_index++, because of an unused variable warning*/
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

	wad->textures = realloc(wad->textures, sizeof(&wad->textures));
	wad->textures[texture_count] = malloc(sizeof(WAD_TEXTURE));

	color_map_size = tga->mapSpec.entryLength;

	/* Copy the name, width and height to the WAD3 texture */
	strncpy(wad->textures[texture_count]->texture_name, name, sizeof(wad->textures[texture_count]->texture_name));
	wad->textures[texture_count]->width = tga->imageSpec.width;
	wad->textures[texture_count]->height = tga->imageSpec.height;

	/* Convert TGA color map to a WAD3 color map */
	for (uint8_t i=0; i < tga->mapSpec.entryLength; i++) {
		wad->textures[texture_count]->color_map[i * 3 + RED  ] = tga->colorMapData[i].red;
		wad->textures[texture_count]->color_map[i * 3 + GREEN] = tga->colorMapData[i].green;
		wad->textures[texture_count]->color_map[i * 3 + BLUE ] = tga->colorMapData[i].blue;
	}

	/* Allocate memory for image and copy from TGA file */
	image_size = wad->textures[texture_count]->width * wad->textures[texture_count]->height;
	wad->textures[texture_count]->image_data = malloc(image_size);
	memcpy(wad->textures[texture_count]->image_data, tga->imageData, image_size);

	/* Allocate memory for mipmap 1 */
	mipmap_1_size = image_size / 4;
	wad->textures[texture_count]->mipmap_1_data = malloc(mipmap_1_size);
	generate_mipmap(wad->textures[texture_count]->mipmap_1_data,
				wad->textures[texture_count]->image_data,
				wad->textures[texture_count]->color_map,
				&color_map_size,
				wad->textures[texture_count]->width,
				wad->textures[texture_count]->height);

	/* Allocate memory for mipmap 2 */
	mipmap_2_size = mipmap_1_size / 4;
	wad->textures[texture_count]->mipmap_2_data = malloc(mipmap_2_size);
	generate_mipmap(wad->textures[texture_count]->mipmap_2_data,
				wad->textures[texture_count]->mipmap_1_data,
				wad->textures[texture_count]->color_map,
				&color_map_size,
				wad->textures[texture_count]->width / 2,
				wad->textures[texture_count]->height / 2);

	/* Allocate memory for mipmap 3 */
	mipmap_3_size = mipmap_2_size / 4;
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
	//tga_print_debug(&tga);

	WAD wad;
	add_tga_to_wad(&wad, &tga, "{test");
	printf("%s\n", wad.textures[0]->texture_name);
	return 0;
}

