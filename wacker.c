#include <errno.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libafbeelding/color_quantization.h>
#include <libafbeelding/img.h>
#include <libafbeelding/scale.h>
#include <libafbeelding/format-tga.h>
#include "wad.h"

// Define color channels
#define RED 0
#define GREEN 1
#define BLUE 2

int add_img_to_wad(WAD *wad, char *img_file_path, char *name)
{
	int image_size = 0;
	int img_mipmap_1_size = 0;
	int img_mipmap_2_size = 0;
	int img_mipmap_3_size = 0;
	AFB_PALETTE pal = afb_palette_init();
	AFB_IMAGE img;
	AFB_IMAGE img_mipmap_1;
	AFB_IMAGE img_mipmap_2;
	AFB_IMAGE img_mipmap_3;

	if (afb_format_tga_load(&img, img_file_path) != AFB_E_SUCCESS)
		return 1;

	if (img.image_type == GRAYSCALE) {
		// We can pass NULL here since we're 100% sure that this image
		// is grayscale
		image_to_pal(&img, NULL);
	} else if (img.image_type == TRUECOLOR) {
		pal = afb_quantize_median_cut(img, 256);
		image_to_pal(&img, &pal);
	}

	if (wad->textures_count == 0) {
		wad->textures = malloc(sizeof(wad->textures));
	} else {
		wad->textures = realloc(wad->textures, sizeof(wad->textures) * (wad->textures_count + 1));
	}
	
	wad->textures[wad->textures_count] = malloc(sizeof(WAD_TEXTURE));

	/* Copy the name, width and height to the WAD3 texture */
	strncpy(wad->textures[wad->textures_count]->texture_name, name, sizeof(wad->textures[wad->textures_count]->texture_name));
	wad->textures[wad->textures_count]->width = img.width;
	wad->textures[wad->textures_count]->height = img.height;

	/* Convert color map to a WAD3 color map */
	for (uint16_t i=0; i < img.palette.size; i++) {
		wad->textures[wad->textures_count]->color_map[i * 3 + RED  ] = afb_rgba_get_r(img.palette.colors[i]);
		wad->textures[wad->textures_count]->color_map[i * 3 + GREEN] = afb_rgba_get_g(img.palette.colors[i]);
		wad->textures[wad->textures_count]->color_map[i * 3 + BLUE ] = afb_rgba_get_b(img.palette.colors[i]);
	}

	/* Allocate memory for image and copy from TGA file */
	image_size = wad->textures[wad->textures_count]->width * wad->textures[wad->textures_count]->height;
	wad->textures[wad->textures_count]->image_data = malloc(image_size);
	memcpy(wad->textures[wad->textures_count]->image_data, img.image_data, image_size);

	/* Copy and scale mipmap 1 */
	img_mipmap_1 = afb_copy_image(&img);
	afb_scale_nearest_neighbor(&img_mipmap_1, img.width / 2, img.height / 2);
	
	/* Copy to WAD struct */
	img_mipmap_1_size = img_mipmap_1.width * img_mipmap_1.height;
	wad->textures[wad->textures_count]->mipmap_1_data = malloc(img_mipmap_1_size);
	memcpy(wad->textures[wad->textures_count]->mipmap_1_data, img_mipmap_1.image_data, img_mipmap_1_size);
	
	
	/* Copy and scale mipmap 2 */
	img_mipmap_2 = afb_copy_image(&img);
	afb_scale_nearest_neighbor(&img_mipmap_2, img.width / 4, img.height / 4);
	
	/* Copy to WAD struct */
	img_mipmap_2_size = img_mipmap_2.width * img_mipmap_2.height;
	wad->textures[wad->textures_count]->mipmap_2_data = malloc(img_mipmap_2_size);
	memcpy(wad->textures[wad->textures_count]->mipmap_2_data, img_mipmap_2.image_data, img_mipmap_2_size);

	
	/* Copy and scale mipmap 3 */
	img_mipmap_3 = afb_copy_image(&img);
	afb_scale_nearest_neighbor(&img_mipmap_3, img.width / 8, img.height / 8);
	
	/* Copy to WAD struct */
	img_mipmap_3_size = img_mipmap_3.width * img_mipmap_3.height;
	wad->textures[wad->textures_count]->mipmap_3_data = malloc(img_mipmap_3_size);
	memcpy(wad->textures[wad->textures_count]->mipmap_3_data, img_mipmap_3.image_data, img_mipmap_3_size);

	wad->textures_count++;
	
	afb_image_free(&img_mipmap_1);
	afb_image_free(&img_mipmap_2);
	afb_image_free(&img_mipmap_3);

	return 0;
}

void pack_wad(char *output_file, char **input_files, unsigned int input_files_size)
{
	char name[16];
	char *filename = NULL;
	unsigned int s = 0;
	WAD wad = wad_init();

	for (unsigned int i = 0; i < input_files_size; i++) {
		/* Go through string and find first '/' */
		for (s = strlen(input_files[i]); s != 0; s--) {
			if (input_files[i][s] == '/') {
				s++;
				break;
			}
		}
		filename = &input_files[i][s];

		/* Find extension of filename */
		for (s = 0; s < 16; s++) {
			if (filename[s] == '.') {
				break;
			}
		}

		/* Copy filename without extension (or 16 bytes of filename) */
		s = (strlen(filename) < s ? 16 : s);
		strncpy(name, filename, s);

		if (add_img_to_wad(&wad, input_files[i], name) > 0)
			printf("Could not add file %s to WAD\n", input_files[i]);
		else
			printf("File added to wad: %s\n", input_files[i]);
	}

	if ((wad_update(&wad)) != 0 )
		printf("failed to update wad\n");

	if ((wad_save_file(&wad, output_file)) != 0)
		printf("save file error\n");
}

void list_wad(char *input_file)
{
	WAD wad = wad_init();
	wad_open_file(&wad, input_file);

	for (unsigned int i=0; i < wad.textures_count; i++) {
		printf("NAME: %s\n\tW: %d\tH: %d\n\n", wad.textures[i]->texture_name,
			   wad.textures[i]->width, wad.textures[i]->height);
	}

	wad_free(&wad);
}

typedef enum {
	OP_NONE,
	OP_EXTRACT,
	OP_PACK,
	OP_LIST_FILES,
} OPERATION;

void print_help(char *program_name)
{
	fprintf(stderr, "usage: %s [-h] [-e|-p|-l] [-i input] [-o output] file ...\n", program_name);
}

int main(int argc, char *argv[])
{
	int opt;
	char *input_file = NULL;
	char *output_file = NULL;
	char **input_files = NULL;
	unsigned int input_files_size = 0;

	OPERATION op = OP_NONE;

	while ((opt = getopt(argc, argv, "i:o:eplh")) != -1) {
		switch (opt) {
		case 'i':
			input_file = optarg;
			break;
		case 'o':
			output_file = optarg;
			break;
		case 'e':
			op = OP_EXTRACT;
			break;
		case 'p':
			op = OP_PACK;
			break;
		case 'l':
			op = OP_LIST_FILES;
			break;
		case 'h':
			print_help(argv[0]);
			exit(0);
			break;
		case ':': /* Fallthrough */
		case '?':
			printf("err\n");
			print_help(argv[0]);
			exit(EINVAL);
			break;
		default:
			fprintf(stderr, "No options given\n");
			print_help(argv[0]);
			exit(EINVAL);
		}
	}

	/* Put all of the extra arguments into an array */
	for ( ; optind < argc; optind++) {
		if (input_files == NULL)
			input_files = malloc(sizeof(char **));
		else
			input_files = realloc(input_files, sizeof(char **) * (input_files_size + 1));
		input_files[input_files_size] = malloc(strlen(argv[optind]) + 1);
		strcpy(input_files[input_files_size], argv[optind]);
		input_files_size++;
	}

	switch (op) {
	case OP_EXTRACT:
		printf("Not implemented\n");
		break;
	case OP_PACK:
		if (output_file == NULL) {
			fprintf(stderr, "No output file specified\n");
			print_help(argv[0]);
			exit(EINVAL);
		}

		if (input_files_size == 0) {
			fprintf(stderr, "No input files given\n");
			print_help(argv[0]);
			exit(EINVAL);
		}

		pack_wad(output_file, input_files, input_files_size);

		for (unsigned int i = 0; i < input_files_size; i++) {
			free(input_files[i]);
		}

		free(input_files);
		break;
	case OP_LIST_FILES:
		if (input_file == NULL) {
			fprintf(stderr, "No input files given\n");
			print_help(argv[0]);
			exit(EINVAL);
		}

		list_wad(input_file);
		break;
	case OP_NONE:
	default:
		/* Message about no operation */
		fprintf(stderr, "Please specify an operation\n");
		print_help(argv[0]);
		exit(EINVAL);
	}

	return 0;
}
