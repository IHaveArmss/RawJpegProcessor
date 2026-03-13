#ifndef JPEG_READER_H
#define JPEG_READER_H

#include <stdio.h>
#include <stdint.h>

// ---- Structuri de date -------------------------------------------------------

typedef struct {
    uint8_t compId;   // id-ul componentei
    uint8_t DCHuff;   // tabelul dc huffman
    uint8_t ACHuff;   // tabelul ac huffman
} ComponentInfo;

typedef struct {
    uint16_t code;    // codul huffman
    uint8_t  length;  // lungimea codului in biti
} HuffmanCode;

typedef struct {
    uint16_t      width;         // latimea imaginii
    uint16_t      height;        // inaltimea imaginii
    uint8_t       numComponents; // numar de componente (de obicei 3 pt color)
    ComponentInfo components[4]; // info per componenta

    uint16_t      quantTable[4][64]; // tabele de cuantizare

    HuffmanCode   dcCodes[4][256]; // coduri huffman dc
    HuffmanCode   acCodes[4][256]; // coduri huffman ac

    uint8_t       dcHuffmanLengths[4][16];  // lungimi dc
    uint8_t       dcHuffmanValues[4][256];  // valori dc
    uint8_t       acHuffmanLengths[4][16];  // lungimi ac
    uint8_t       acHuffmanValues[4][256];  // valori ac
} JpegImage;

// ---- Declaratii de functii --------------------------------------------------

/* intoarce bytes la adresa p, pt ca jpeg e big endian si noi suntem little endian */
void swapBytes(void *p, size_t size);

/* citeste toate segmentele jpeg (SOF, DHT, DQT etc) pana la SOS si umple structura img */
int jpeg_parse_segments(FILE *f, JpegImage *img);

/* citeste headerul SOS dupa markerul 0xFFDA, lasa file pointerul la datele comprimate */
int jpeg_read_sos(FILE *f, JpegImage *img);

#endif /* JPEG_READER_H */
