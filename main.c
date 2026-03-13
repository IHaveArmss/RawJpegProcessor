#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jpegReader.h"

int main(const int argc, char *argv[]) {

    // verificam ca userul a dat un argument, altfel ii zicem cum se foloseste
    if (argc != 2) {
        fprintf(stderr, "Folosire: %s <fisier.jpg>\n", argv[0]);
        return 1;
    }

    // deschidem fisierul in modul binar
    FILE *f = fopen(argv[1], "rb");
    if (f == NULL) {
        fprintf(stderr, "Eroare: nu pot deschide fisierul '%s'\n", argv[1]);
        return 1;
    }

    JpegImage img;
    memset(&img, 0, sizeof(JpegImage)); // initializam tot cu 0

    // citim toate segmentele pana la SOS
    if (jpeg_parse_segments(f, &img) != 0) {
        fclose(f);
        return 1;
    }

    // citim headerul SOS si lasam file pointerul la datele comprimate
    if (jpeg_read_sos(f, &img) != 0) {
        fclose(f);
        return 1;
    }

    printf("\n\nGata cu headerele. Imagine: %dx%d, %d componente.\n",
           img.width, img.height, img.numComponents);

    // TODO: jpegDecodeScan(f, &img);      - decodificam bitstream-ul huffman
    // TODO: filterPixelate(&img, 8);       - aplicam filtrul de pixelatie
    // TODO: jpegWrite("output.jpg", &img); - salvam rezultatul

    fclose(f);
    return 0;
}