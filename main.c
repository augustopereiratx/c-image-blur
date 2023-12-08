#include <stdio.h>
#include <stdlib.h>
#include <png.h>
#include <string.h>

#define STR_STD_SIZE 100

enum program_errors
{
    error_input,
    error_choice,
    usage_error,
    file_read_error,
    even_number_error
};

void error_msgs(int error_type, char *str)
{
    switch (error_type)
    {
    case error_input:
        fprintf(stderr, "Error reading user input\n");
        break;
    case error_choice:
        fprintf(stderr, "Invalid choice\n");
        break;
    case usage_error:
        fprintf(stderr, "Usage: %s <file.png>\n", str);
        break;
    case file_read_error:
        fprintf(stderr, "Error opening file %s\n", str);
        break;
    case even_number_error:
        fprintf(stderr, "This filter does not support an even number as a strength value\n");
        break;
    default:
        break;
    }
}

int input(char *text, char *str, int max_len)
{
    strcpy(str, "");
    printf("%s", text);
    printf("");
    if (fgets(str, max_len, stdin) != NULL)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

int compare_int(const void *a, const void *b)
{
    return (*(int *)a - *(int *)b);
}

int median(int arr[], int n)
{
    qsort(arr, n, sizeof(int), compare_int);

    if (n % 2 == 0)
    {
        return (arr[n / 2 - 1] + arr[n / 2]) / 2;
    }
    else
    {
        return arr[n / 2];
    }
}

int getWindowSize(int *window_size)
{
    char *str = malloc(STR_STD_SIZE * sizeof(char));
    if (input("Filter strength (e.g 5, 7, 9, etc):\n-> ", str, STR_STD_SIZE))
    {
        *window_size = atoi(str);
    }
    else
    {
        error_msgs(error_input, "");
        return error_input;
    }
    return 0;
}

int apply_median_filter(png_bytep *row_pointers, int width, int height)
{
    int window_size = 3;
    while (1)
    {
        getWindowSize(&window_size);
        if (window_size % 2 == 0)
        {
            error_msgs(even_number_error, "");
        }
        else
        {
            break;
        }
    }

    for (int y = window_size / 2; y < height - window_size / 2; y++)
    {
        for (int x = window_size / 2; x < width - window_size / 2; x++)
        {
            int values_R[window_size * window_size];
            int values_G[window_size * window_size];
            int values_B[window_size * window_size];
            int values_A[window_size * window_size];

            int index = 0;
            for (int i = -window_size / 2; i <= window_size / 2; i++)
            {
                for (int j = -window_size / 2; j <= window_size / 2; j++)
                {
                    png_bytep px = &(row_pointers[y + i][4 * (x + j)]);
                    values_R[index] = (int)px[0];
                    values_G[index] = (int)px[1];
                    values_B[index] = (int)px[2];
                    values_A[index] = (int)px[3];
                    index++;
                }
            }

            png_bytep px = &(row_pointers[y][4 * x]);
            px[0] = (png_byte)median(values_R, window_size * window_size);
            px[1] = (png_byte)median(values_G, window_size * window_size);
            px[2] = (png_byte)median(values_B, window_size * window_size);
            px[3] = (png_byte)median(values_A, window_size * window_size);
        }
    }
    return 0;
}

int apply_average_blur(png_bytep *row_pointers, int width, int height)
{
    int window_size = 3;
    while (1)
    {
        getWindowSize(&window_size);
        if (window_size % 2 == 0)
        {
            error_msgs(even_number_error, "");
        }
        else
        {
            break;
        }
    }

    for (int y = window_size / 2; y < height - window_size / 2; y++)
    {
        for (int x = window_size / 2; x < width - window_size / 2; x++)
        {
            int sum_R = 0, sum_G = 0, sum_B = 0, sum_A = 0;

            for (int i = -window_size / 2; i <= window_size / 2; i++)
            {
                for (int j = -window_size / 2; j <= window_size / 2; j++)
                {
                    png_bytep px = &(row_pointers[y + i][4 * (x + j)]);
                    sum_R += (int)px[0];
                    sum_G += (int)px[1];
                    sum_B += (int)px[2];
                    sum_A += (int)px[3];
                }
            }

            int num_pixels = window_size * window_size;
            png_bytep px = &(row_pointers[y][4 * x]);
            px[0] = (png_byte)(sum_R / num_pixels);
            px[1] = (png_byte)(sum_G / num_pixels);
            px[2] = (png_byte)(sum_B / num_pixels);
            px[3] = (png_byte)(sum_A / num_pixels);
        }
    }
    return 0;
}

void write_png_file(const char *file_name, png_bytep *row_pointers, int width, int height)
{
    FILE *fp = fopen(file_name, "wb");
    if (!fp)
        abort();

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png)
        abort();

    png_infop info = png_create_info_struct(png);
    if (!info)
        abort();

    if (setjmp(png_jmpbuf(png)))
        abort();

    png_init_io(png, fp);

    png_set_IHDR(
        png,
        info,
        width, height,
        8,
        PNG_COLOR_TYPE_RGBA,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT);

    png_write_info(png, info);

    png_write_image(png, row_pointers);
    png_write_end(png, NULL);

    fclose(fp);
    png_destroy_write_struct(&png, &info);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        error_msgs(usage_error, argv[0]);
        return usage_error;
    }

    FILE *fp = fopen(argv[1], "rb");
    if (!fp)
    {
        error_msgs(file_read_error, argv[1]);
        return file_read_error;
    }

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop info = png_create_info_struct(png);
    png_init_io(png, fp);
    png_read_info(png, info);

    int width = png_get_image_width(png, info);
    int height = png_get_image_height(png, info);
    png_bytep *row_pointers = (png_bytep *)malloc(sizeof(png_bytep) * height);
    for (int y = 0; y < height; y++)
    {
        row_pointers[y] = (png_byte *)malloc(png_get_rowbytes(png, info));
    }
    png_read_image(png, row_pointers);

    char *str = malloc(STR_STD_SIZE * sizeof(char));
    if (input("Choose filter method:\n1 - Median filter\n2 - Average blur\n-> ", str, STR_STD_SIZE))
    {
        switch (atoi(str))
        {
        case 1:
            apply_median_filter(row_pointers, width, height);
            break;
        case 2:
            apply_average_blur(row_pointers, width, height);
            break;
        default:
            error_msgs(error_choice, "");
            return error_choice;
        }
    }
    else
    {
        error_msgs(error_input, "");
        return error_input;
    }
    write_png_file("out.png", row_pointers, width, height);

    fclose(fp);
    for (int y = 0; y < height; y++)
    {
        free(row_pointers[y]);
    }
    free(row_pointers);
    png_destroy_read_struct(&png, &info, NULL);
    free(str);

    return 0;
}
