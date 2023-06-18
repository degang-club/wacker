#include <dirent.h>
#include <float.h>
#include <getopt.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libafbeelding/img.h>
#include <libafbeelding/scale.h>
#include <libafbeelding/tga.h>
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

void add_img_to_wad(WAD *wad, IMAGE *img, char *name)
{
	int image_size = 0;
	int img_mipmap_1_size = 0;
	int img_mipmap_2_size = 0;
	int img_mipmap_3_size = 0;
	IMAGE img_mipmap_1;
	IMAGE img_mipmap_2;
	IMAGE img_mipmap_3;

	if (wad->textures_count == 0) {
		wad->textures = malloc(sizeof(wad->textures));
	} else {
		wad->textures = realloc(wad->textures, sizeof(wad->textures) * (wad->textures_count + 1));
	}
	
	wad->textures[wad->textures_count] = malloc(sizeof(WAD_TEXTURE));

	/* Copy the name, width and height to the WAD3 texture */
	strncpy(wad->textures[wad->textures_count]->texture_name, name, sizeof(wad->textures[wad->textures_count]->texture_name));
	wad->textures[wad->textures_count]->width = img->width;
	wad->textures[wad->textures_count]->height = img->height;

	/* Convert TGA color map to a WAD3 color map */
	for (uint16_t i=0; i < img->palette.size; i++) {
		wad->textures[wad->textures_count]->color_map[i * 3 + RED  ] = afb_rgba_get_r(img->palette.colors[i]);
		wad->textures[wad->textures_count]->color_map[i * 3 + GREEN] = afb_rgba_get_g(img->palette.colors[i]);
		wad->textures[wad->textures_count]->color_map[i * 3 + BLUE ] = afb_rgba_get_b(img->palette.colors[i]);
	}

	/* Allocate memory for image and copy from TGA file */
	image_size = wad->textures[wad->textures_count]->width * wad->textures[wad->textures_count]->height;
	wad->textures[wad->textures_count]->image_data = malloc(image_size);
	memcpy(wad->textures[wad->textures_count]->image_data, img->image_data, image_size);

	/* Copy and scale mipmap 1 */
	img_mipmap_1 = afb_copy_image(img);
	afb_scale_nearest_neighbor(&img_mipmap_1, img->width / 2, img->height / 2);
	
	/* Copy to WAD struct */
	img_mipmap_1_size = img_mipmap_1.width * img_mipmap_1.height;
	wad->textures[wad->textures_count]->mipmap_1_data = malloc(img_mipmap_1_size);
	memcpy(wad->textures[wad->textures_count]->mipmap_1_data, img_mipmap_1.image_data, img_mipmap_1_size);
	
	
	/* Copy and scale mipmap 2 */
	img_mipmap_2 = afb_copy_image(img);
	afb_scale_nearest_neighbor(&img_mipmap_2, img->width / 2, img->height / 2);
	
	/* Copy to WAD struct */
	img_mipmap_2_size = img_mipmap_2.width * img_mipmap_2.height;
	wad->textures[wad->textures_count]->mipmap_2_data = malloc(img_mipmap_2_size);
	memcpy(wad->textures[wad->textures_count]->mipmap_2_data, img_mipmap_2.image_data, img_mipmap_2_size);

	
	/* Copy and scale mipmap 3 */
	img_mipmap_3 = afb_copy_image(img);
	afb_scale_nearest_neighbor(&img_mipmap_3, img->width / 2, img->height / 2);
	
	/* Copy to WAD struct */
	img_mipmap_3_size = img_mipmap_3.width * img_mipmap_3.height;
	wad->textures[wad->textures_count]->mipmap_3_data = malloc(img_mipmap_3_size);
	memcpy(wad->textures[wad->textures_count]->mipmap_3_data, img_mipmap_3.image_data, img_mipmap_3_size);

	wad->textures_count++;
	
	afb_image_free(&img_mipmap_1);
	afb_image_free(&img_mipmap_2);
	afb_image_free(&img_mipmap_3);
}

int convert_tga_to_wad(WAD *wad, char *file_path, char *file_name) {
	IMAGE img;
	if (tga_load_file(&img, file_path) != AFB_E_SUCCESS)
		return 1;
	add_img_to_wad(wad, &img, file_name);
	return 0;
}

int main(int argc, char *argv[])
{
	int opt;
	char *dir_path = "./"; // Use root when nothing supplied
	char *output_file = "./output.wad";
	char *file_path;
	char *file_name;
	char *extension_start;

	while ((opt = getopt(argc, argv, "d:o:")) != -1) {
	switch (opt) {
		case 'd':
			dir_path = optarg;
			break;
		case 'o':
			output_file = optarg;
			break;
		case 'h': /* Add help function here */
			exit(0);
			break;
		case '?':
			exit(1);
			break;
		default:
			fprintf(stderr, "No options given\n");
			exit(1);
		}
	}

	// Always add extra '/' to complete path to file
	if (dir_path[strlen(dir_path) - 1] != '/')
		dir_path = strcat(dir_path, "/");
	printf("Dir path: %s\n\n", dir_path);
	
	// Open dir
	DIR *dir = opendir(dir_path);
	struct dirent *entry;
	if(dir == NULL) {
		printf("Could not open provided directory!");
		return 1;
	}

	WAD wad = wad_init();

	// Find all .tga files in dir
	while((entry = readdir(dir)) != NULL) {
		extension_start = strrchr(entry->d_name, '.'); // Find last '.' to get extension 
		if(extension_start == NULL) continue;

		// Load tga into wad when it is a tga file
		if(strcmp(extension_start, tga_extension) == 0){
			file_path = strcomb(2, dir_path, entry->d_name);
			file_name = substr(entry->d_name, extension_start);

			if (convert_tga_to_wad(&wad, file_path, file_name) > 0)
				printf("Could not add file %s to WAD\n", file_path);
			else
				printf("tga file added to wad: %s\n", file_path);
		}
	}

	if(closedir(dir) == -1) {
		printf("Failed to close directory!");
		return 1;
	}

	wad_update(&wad);
	wad_save_file(&wad, output_file);

	return 0;
}
