#include <stdio.h>
#include <stdlib.h>
#include <math.h>

typedef struct
{
    unsigned char red, green, blue;
} PPMPixel;

typedef struct
{
    PPMPixel pix;
    int i;
} PPMPixel2;

typedef struct
{
    int x, y;
    PPMPixel *data;
} PPMImage;

PPMPixel *color_palette;
int n_color_pallete = 0;
int *color_map_index;

void cut(PPMPixel *box, int start, int end, int colors_per_box);
static PPMImage *readPPM(const char *filename);
int longest_dim(PPMPixel *pixel_array, int start, int end);
PPMPixel *sort(PPMPixel *data, int start, int end, int dim);
int comparator_red(const void *p, const void *q);
int comparator_green(const void *p, const void *q);
int comparator_blue(const void *p, const void *q);
PPMPixel mean(PPMPixel *box, int start, int end);
void write_ppm(PPMImage *image);

#define RGB_COMPONENT_COLOR 255

int main()
{
    const char *filename = "BK_image_2020.ppm";
    int n_colors_out = 256;
    int colors_per_box;
    int n_pixels;
    static PPMImage *image;

    color_palette = (PPMPixel *)malloc(sizeof(PPMPixel) * n_colors_out);

    image = readPPM(filename);
    n_pixels = image->x * image->y;

    color_map_index = (int *)malloc(sizeof(int) * n_pixels);
    for (int i = 0; i < n_pixels; i++)
        color_map_index[i] = i;

    printf("%d %d\n", image->x, image->y);
    colors_per_box = n_pixels / n_colors_out;

    PPMPixel *pixel_array = (PPMPixel *)malloc(sizeof(PPMPixel) * n_pixels);
    for (int i = 0; i < n_pixels; i++)
        pixel_array[i] = image->data[i];

    cut(pixel_array, 0, n_pixels, colors_per_box);

    for (int i = 0; i < n_color_pallete; i++)
        printf("%d %d %d\n", color_palette[i].red,
               color_palette[i].blue, color_palette[i].green);

    for (int i = 0; i < n_pixels; i++)
    {
        int dist_min = 1000, dist_j = 0;
        for (int j = 0; j < n_color_pallete; j++)
        {
            int dist = sqrt((image->data[i].red - color_palette[j].red) *
                                (image->data[i].red - color_palette[j].red) +
                            (image->data[i].green - color_palette[j].green) *
                                (image->data[i].green - color_palette[j].green) +
                            (image->data[i].blue - color_palette[j].blue) *
                                (image->data[i].blue - color_palette[j].blue));

            //printf("%d %d : %d ", i, j, dist);
            if (dist < dist_min)
            {
                dist_min = dist;
                dist_j = j;
            }
        }
        image->data[i] = color_palette[dist_j];
    }

    FILE *out = fopen("output.txt", "w");
    for (int i = 0; i < n_pixels; i++)
        fprintf(out, "%d %d %d\n", image->data[i].red,
                image->data[i].green, image->data[i].blue);
    fclose(out);
    write_ppm(image);

    return 0;
}

void write_ppm(PPMImage *image)
{
    const int dimx = image->x, dimy = image->y;
    int i, j;
    FILE *fp = fopen("256color.ppm", "wb"); /* b - binary mode */
    (void)fprintf(fp, "P6\n%d %d\n255\n", dimx, dimy);
    for (i = 0; i < dimy * dimx; ++i)
    {
        static unsigned char color[3];
        color[0] = image->data[i].red;   /* red */
        color[1] = image->data[i].green; /* green */
        color[2] = image->data[i].blue;  /* blue */
        (void)fwrite(color, 1, 3, fp);
    }
    (void)fclose(fp);

    /*FILE *out = fopen("output.ppm", "wb");

    fprintf(out, "P6\n%d %d\n255\n", image->x, image->y);

    for (int i = 0; i < image->x * image->y; i++)
        fwrite(image->data, sizeof(PPMPixel), 1, out);

    fclose(out);*/
}

void cut(PPMPixel *box, int start, int end, int colors_per_box)
{
    if ((end - start) <= colors_per_box)
    {
        PPMPixel mean_pixel = mean(box, start, end);
        color_palette[n_color_pallete++] = mean_pixel;
        for (int i = start; i < end; i++)
            box[i] = mean_pixel;
        printf("%d\n", n_color_pallete);
    }
    else
    {
        int dim = longest_dim(box, start, end);
        box = sort(box, start, end, dim);
        cut(box, start, (end + start) / 2, colors_per_box);
        cut(box, ((end + start) / 2) + 1, end, colors_per_box);
    }
}

PPMPixel mean(PPMPixel *box, int start, int end)
{
    int redMean = 0, greenMean = 0, blueMean = 0;
    for (int i = start; i < end; i++)
    {
        redMean += box[i].red;
        greenMean += box[i].green;
        blueMean += box[i].blue;
    }
    redMean /= (end - start);
    greenMean /= (end - start);
    blueMean /= (end - start);
    PPMPixel meanPixel;
    meanPixel.red = (unsigned char)redMean;
    meanPixel.green = (unsigned char)greenMean;
    meanPixel.blue = (unsigned char)blueMean;

    return meanPixel;
}

PPMPixel *sort(PPMPixel *data, int start, int end, int dim)
{
    PPMPixel2 *new_array;
    new_array = (PPMPixel2 *)malloc(sizeof(PPMPixel2) * (end - start));
    int k = 0;

    for (int i = start; i < end; i++)
    {
        new_array[k].pix = data[i];
        new_array[k++].i = color_map_index[i];
    }

    switch (dim)
    {
    case 0:
        qsort(new_array, k, sizeof(new_array[0]), comparator_red);
        break;

    case 1:
        qsort(new_array, k, sizeof(new_array[0]), comparator_green);
        break;

    case 2:
        qsort(new_array, k, sizeof(new_array[0]), comparator_blue);
        break;
    }

    for (int i = start; i < end; i++)
    {
        data[i] = new_array[i - start].pix;
        color_map_index[i] = new_array[i - start].i;
    }
    return data;
}

int comparator_red(const void *p, const void *q)
{
    return ((((PPMPixel2 *)p)->pix).red - (((PPMPixel2 *)q)->pix).red);
}

int comparator_green(const void *p, const void *q)
{
    //return (((PPMPixel *)p)->green - ((PPMPixel *)q)->green);
    return ((((PPMPixel2 *)p)->pix).green - (((PPMPixel2 *)q)->pix).green);
}

int comparator_blue(const void *p, const void *q)
{
    //return (((PPMPixel *)p)->blue - ((PPMPixel *)q)->blue);
    return ((((PPMPixel2 *)p)->pix).blue - (((PPMPixel2 *)q)->pix).blue);
}

int longest_dim(PPMPixel *pixel_array, int start, int end)
{
    int Rmin = 255, Gmin = 255, Bmin = 255;
    int Rmax = 0, Gmax = 0, Bmax = 0;

    for (int i = start; i < end; i++)
    {
        //find Rmin, Gmin, Bmin
        Rmin = fmin(Rmin, pixel_array[i].red);
        Gmin = fmin(Gmin, pixel_array[i].green);
        Bmin = fmin(Bmin, pixel_array[i].blue);

        //find Rmax, Gmax, Bmax
        Rmax = fmax(Rmax, pixel_array[i].red);
        Gmax = fmax(Gmax, pixel_array[i].blue);
        Bmax = fmax(Bmax, pixel_array[i].green);
    }

    int Rrange = Rmax - Rmin, Grange = Gmax - Gmin, Brange = Bmax - Bmin;

    if (Rrange >= Grange && Rrange >= Brange)
    {
        return 0;
    }
    else if (Grange >= Rrange && Grange >= Brange)
    {
        return 1;
    }
    else
    {
        return 2;
    }
}

static PPMImage *readPPM(const char *filename)
{
    char buff[16];
    PPMImage *img;
    FILE *fp;
    int c, rgb_comp_color;
    //open PPM file for reading
    fp = fopen(filename, "rb");
    if (!fp)
    {
        fprintf(stderr, "Unable to open file '%s'\n", filename);
        exit(1);
    }

    //read image format
    if (!fgets(buff, sizeof(buff), fp))
    {
        perror(filename);
        exit(1);
    }

    //check the image format
    if (buff[0] != 'P' || buff[1] != '6')
    {
        fprintf(stderr, "Invalid image format (must be 'P6')\n");
        exit(1);
    }

    //alloc memory form image
    img = (PPMImage *)malloc(sizeof(PPMImage));
    if (!img)
    {
        fprintf(stderr, "Unable to allocate memory\n");
        exit(1);
    }

    //check for comments
    c = getc(fp);
    while (c == '#')
    {
        while (getc(fp) != '\n')
            ;
        c = getc(fp);
    }

    ungetc(c, fp);
    //read image size information
    if (fscanf(fp, "%d %d", &img->x, &img->y) != 2)
    {
        fprintf(stderr, "Invalid image size (error loading '%s')\n", filename);
        exit(1);
    }

    //read rgb component
    if (fscanf(fp, "%d", &rgb_comp_color) != 1)
    {
        fprintf(stderr, "Invalid rgb component (error loading '%s')\n", filename);
        exit(1);
    }

    //check rgb component depth
    if (rgb_comp_color != RGB_COMPONENT_COLOR)
    {
        fprintf(stderr, "'%s' does not have 8-bits components\n", filename);
        exit(1);
    }

    while (fgetc(fp) != '\n')
        ;
    //memory allocation for pixel data
    img->data = (PPMPixel *)malloc(img->x * img->y * sizeof(PPMPixel));

    if (!img)
    {
        fprintf(stderr, "Unable to allocate memory\n");
        exit(1);
    }

    //read pixel data from file
    if (fread(img->data, 3 * img->x, img->y, fp) != img->y)
    {
        fprintf(stderr, "Error loading image '%s'\n", filename);
        exit(1);
    }

    fclose(fp);
    return img;
}