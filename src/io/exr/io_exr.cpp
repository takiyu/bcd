//
// Replacing OpenEXR with tinyexr
//

#define TINYEXR_IMPLEMENTATION
#include "tinyexr.h"

#include <stdlib.h>
#include <stdio.h>

namespace {

float *readSingleExr(const char *name, int *nx, int *ny) {
    // 1. Read EXR version.
    EXRVersion exr_version;
    int ret = ParseEXRVersionFromFile(&exr_version, name);
    if (ret != 0) {
        printf("Error reading file: %s (header)\n", name);
        exit(-1);
    }
    if (exr_version.multipart) {
        // must be multipart flag is false.
        printf("Error reading file: %s (must not be multipart)\n", name);
        exit(-1);
    }

    // 2. Read EXR header
    EXRHeader exr_header;
    InitEXRHeader(&exr_header);
    const char* err;
    ret = ParseEXRHeaderFromFile(&exr_header, &exr_version, name, &err);
    if (ret != 0) {
        printf("Error reading file: %s (header, %s)\n", name, err);
        exit(-1);
    }

    // Read HALF channel as FLOAT.
    for (int i = 0; i < exr_header.num_channels; i++) {
        if (exr_header.pixel_types[i] == TINYEXR_PIXELTYPE_HALF) {
            exr_header.requested_pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT;
        }
    }

    // 3. Read EXR image
    EXRImage exr_image;
    InitEXRImage(&exr_image);
    ret = LoadEXRImageFromFile(&exr_image, &exr_header, name, &err);
    if (ret != 0) {
        printf("Error reading file: %s (image, %s)\n", name, err);
        exit(-1);
    }

    // 4. Find RGB channels
    int idxR = -1, idxG = -1, idxB = -1;
    for (int c = 0; c < exr_header.num_channels; c++) {
        if (strcmp(exr_header.channels[c].name, "R") == 0) {
            idxR = c;
        } else if (strcmp(exr_header.channels[c].name, "G") == 0) {
            idxG = c;
        } else if (strcmp(exr_header.channels[c].name, "B") == 0) {
            idxB = c;
        }
    }
    if (idxR == -1) {
        printf("Error reading file: %s (no R channel)\n", name);
        exit(-1);
    }
    if (idxG == -1) {
        printf("Error reading file: %s (no G channel)\n", name);
        exit(-1);
    }
    if (idxB == -1) {
        printf("Error reading file: %s (no B channel)\n", name);
        exit(-1);
    }

    if (exr_header.tiled) {
        printf("Error reading file: %s (tiled format is not implemented yet)\n",
               name);
        exit(-1);
    }

    // 5. Parse
    const int width = exr_image.width;
    const int height = exr_image.height;
    float *out_data = new float[3 * width * height];
    for (int i = 0; i < width * height; i++) {
        out_data[i + 0 * width * height] =
            reinterpret_cast<float **>(exr_image.images)[idxB][i];
        out_data[i + 1 * width * height] =
            reinterpret_cast<float **>(exr_image.images)[idxG][i];
        out_data[i + 2 * width * height] =
            reinterpret_cast<float **>(exr_image.images)[idxR][i];
    }

    // 6. Free image data
    FreeEXRHeader(&exr_header);
    FreeEXRImage(&exr_image);

    // 7. Return
    *nx = width;
    *ny = height;
    return out_data;
}

float *readMultiExr(const char *name, int *nx, int *ny, int *nch) {
    // 1. Read EXR version.
    EXRVersion exr_version;
    int ret = ParseEXRVersionFromFile(&exr_version, name);
    if (ret != 0) {
        printf("Error reading file: %s (header)\n", name);
        exit(-1);
    }
    if (exr_version.multipart) {
        // must be multipart flag is false.
        printf("Error reading file: %s (must not be multipart)\n", name);
        exit(-1);
    }

    // 2. Read EXR header
    EXRHeader exr_header;
    InitEXRHeader(&exr_header);
    const char* err;
    ret = ParseEXRHeaderFromFile(&exr_header, &exr_version, name, &err);
    if (ret != 0) {
        printf("Error reading file: %s (header, %s)\n", name, err);
        exit(-1);
    }

    // Read HALF channel as FLOAT.
    for (int i = 0; i < exr_header.num_channels; i++) {
        if (exr_header.pixel_types[i] == TINYEXR_PIXELTYPE_HALF) {
            exr_header.requested_pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT;
        }
    }

    // 3. Read EXR image
    EXRImage exr_image;
    InitEXRImage(&exr_image);
    ret = LoadEXRImageFromFile(&exr_image, &exr_header, name, &err);
    if (ret != 0) {
        printf("Error reading file: %s (image, %s)\n", name, err);
        exit(-1);
    }

    if (exr_header.tiled) {
        printf("Error reading file: %s (tiled format is not implemented yet)\n",
               name);
        exit(-1);
    }

    if (exr_header.tiled) {
        printf("Error reading file: %s (tiled format is not implemented yet)\n",
               name);
        exit(-1);
    }

    // 4. Parse
    const int width = exr_image.width;
    const int height = exr_image.height;
    const int n_ch = exr_header.num_channels;
    float *out_data = new float[width * height * n_ch];
    for (int i = 0; i < width * height; i++) {
        for (int c = 0; c < n_ch; c++) {
            // Search input channel
            char ch_name[10];
            sprintf(ch_name, "Bin_%04d", c);
            int src_c = 0;
            for (; src_c < n_ch; src_c++) {
                if (strcmp(exr_header.channels[c].name, ch_name) == 0) {
                    break;
                }
            }
            if (src_c == n_ch) {
                printf("Error reading file: %s (no %s channel)\n", ch_name);
                exit(-1);
            }
            // Copy
            out_data[i + c * width * height] =
                reinterpret_cast<float **>(exr_image.images)[src_c][i];
        }
    }

    // 6. Free image data
    FreeEXRHeader(&exr_header);
    FreeEXRImage(&exr_image);

    // 7. Return
    *nx = width;
    *ny = height;
    *nch = n_ch;
    return out_data;
}


void writeSingleExr(const float* r_pixels, const float* g_pixels,
                    const float* b_pixels, int width, int height,
                    const char* filename) {
    EXRHeader header;
    InitEXRHeader(&header);

    EXRImage image;
    InitEXRImage(&image);

    image.num_channels = 3;

    const float* image_ptr[3] = {b_pixels, g_pixels, r_pixels};

    image.images = (unsigned char**)image_ptr;
    image.width = width;
    image.height = height;

    header.num_channels = 3;
    header.channels =
        (EXRChannelInfo *)malloc(sizeof(EXRChannelInfo) * header.num_channels);
    // Must be (A)BGR order, since most of EXR viewers expect this channel order.
    strncpy(header.channels[0].name, "B\0", 255);
    strncpy(header.channels[1].name, "G\0", 255);
    strncpy(header.channels[2].name, "R\0", 255);

    header.pixel_types = (int *)malloc(sizeof(int) * header.num_channels);
    header.requested_pixel_types =
        (int *)malloc(sizeof(int) * header.num_channels);
    for (int i = 0; i < header.num_channels; i++) {
        // pixel type of input image
        header.pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT;
        // pixel type of output image to be stored in .EXR
        header.requested_pixel_types[i] = TINYEXR_PIXELTYPE_HALF;
    }

    const char* err;
    int ret = SaveEXRImageToFile(&image, &header, filename, &err);
    if (ret != TINYEXR_SUCCESS) {
        fprintf(stderr, "Save EXR err: %s\n", err);
        exit(-1);
    }
    printf("Saved exr file. [ %s ] \n", filename);

    free(header.channels);
    free(header.pixel_types);
    free(header.requested_pixel_types);
}

void writeMultiExr(const float* const* pixel_ptrs, int width, int height, int n_ch,
                   const char* filename) {
    EXRHeader header;
    InitEXRHeader(&header);

    EXRImage image;
    InitEXRImage(&image);

    image.num_channels = n_ch;

    image.images = (unsigned char**)pixel_ptrs;
    image.width = width;
    image.height = height;

    header.num_channels = n_ch;
    header.channels =
        (EXRChannelInfo *)malloc(sizeof(EXRChannelInfo) * header.num_channels);

    header.pixel_types = (int *)malloc(sizeof(int) * header.num_channels);
    header.requested_pixel_types =
        (int *)malloc(sizeof(int) * header.num_channels);
    for (int i = 0; i < header.num_channels; i++) {
        // pixel type of input image
        header.pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT;
        // pixel type of output image to be stored in .EXR
        header.requested_pixel_types[i] = TINYEXR_PIXELTYPE_HALF;

        // Name
        sprintf(header.channels[i].name, "Bin_%04d", i);
    }

    const char* err;
    int ret = SaveEXRImageToFile(&image, &header, filename, &err);
    if (ret != TINYEXR_SUCCESS) {
        fprintf(stderr, "Save EXR err: %s\n", err);
        exit(-1);
    }
    printf("Saved exr file. [ %s ] \n", filename);

    free(header.channels);
    free(header.pixel_types);
    free(header.requested_pixel_types);
}


} // namespace


float *readImageEXR(const char *name, int *nx, int *ny) {
    return readSingleExr(name, nx, ny);
}

void writeImageEXR(const char *name, const float *pixels, int xRes, int yRes) {
    const int offset = xRes * yRes;
    const float *b_pixels = pixels;
    const float *g_pixels = pixels + offset;
    const float *r_pixels = pixels + offset * 2;
    writeSingleExr(r_pixels, g_pixels, b_pixels, xRes, yRes, name);
}

void writeImageEXR(const char *name, const float * const * pixels, int xRes,
                   int yRes) {
    const float *b_pixels = pixels[0];
    const float *g_pixels = pixels[1];
    const float *r_pixels = pixels[2];
    writeSingleExr(r_pixels, g_pixels, b_pixels, xRes, yRes, name);
}

void writeMultiImageEXR(const char *fileName, const float *zPixels, int width,
                        int height, int nchan) {
    std::vector<const float *> pixel_ptrs(nchan);
    for (int i = 0; i < nchan; i++) {
        pixel_ptrs[i] = &zPixels[width * height * i];
    }
    writeMultiExr(&pixel_ptrs[0], width, height, nchan, fileName);
}

void writeMultiImageEXR(const char *fileName, const float * const * zPixels,
                        int width, int height, int nchan) {
    writeMultiExr(zPixels, width, height, nchan, fileName);
}

float *readMultiImageEXR(const char fileName[], int *width, int *height,
                         int *ncha) {
    return readMultiExr(fileName, width, height, ncha);
}
