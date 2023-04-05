#include <dirent.h>
#include <float.h>
#include <stdarg.h>
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

const char tga_extension[] = ".tga";

char* strcomb(unsigned int num, ...)
{
    va_list valist;

    char *combined_str;
    unsigned int combined_str_s = 1;

    char *tmp_str;
    unsigned int i;

    va_start(valist, num);
    combined_str = calloc(1, sizeof(char));

    for (i = 0; i < num; i++) {
        tmp_str = va_arg(valist, char*);

        combined_str_s += strlen(tmp_str);
        combined_str = realloc(combined_str, combined_str_s);

        strcat(combined_str, tmp_str);
    }
    va_end(valist);

    return combined_str;
}

char *substr(char *start, char *end) {
	int length = end - start;
	char *sub = calloc(length + 1, sizeof(char));
	strncpy(sub, start, length);
	sub[length] = '\0';
	return sub;
}


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

	if (texture_count == 0) {
        wad->textures = malloc(sizeof(&wad->textures));
    } else {
        wad->textures = realloc(wad->textures, sizeof(&wad->textures));
    }
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

void convert_tga_to_wad(WAD *wad, char *file_path, char *file_name) {
	TGA tga;
	tga_load_file(&tga, file_path);
	add_tga_to_wad(wad, &tga, file_name);
}

int main(int argc, char *argv[])
{
	//if(argc > 2) return 1; // Do not call when more than 2 args

	// Use root when nothing supplied
	char *dir_path = "./";

	// Use path as argument
	if(argc == 2) { 
		dir_path = strcat(argv[1], "/"); // Always add extra '/' to complete path to file
	}
	printf("Dir path: %s\n\n", dir_path);
	
	// Open dir
	DIR *dir = opendir(dir_path);
	struct dirent *entry;
	if(dir == NULL) {
		printf("Could not open provided directory!");
		return 1;
	}

	char *file_path;
	char *file_name;
	char *extension_start;

	WAD wad = wad_init();

	// Find all .tga files in dir
	while((entry = readdir(dir)) != NULL) {
		extension_start = strrchr(entry->d_name, '.'); // Find last '.' to get extension 
		if(extension_start == NULL) continue;

		// Load tga into wad when it is a tga file
		if(strcmp(extension_start, tga_extension) == 0){
			file_path = strcomb(2, dir_path, entry->d_name);
			file_name = substr(entry->d_name, extension_start);

			convert_tga_to_wad(&wad, file_path, file_name);
			printf("tga file added to wad: %s\n", file_path);
		}
	}

	if(closedir(dir) == -1) {
		printf("Failed to close directory!");
		return 1;
	}

	wad_update(&wad);
	wad_save_file(&wad, "./output.wad");

	return 0;
}