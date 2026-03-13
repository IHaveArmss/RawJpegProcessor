#include "jpegReader.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// ---- Utilitare --------------------------------------------------------------

// intoarce ordinea bytes din memorie, jpeg e big endian da noi suntem little
void swapBytes(void *p, const size_t size) {
    uint8_t *data = (uint8_t *)p;
    for (size_t i = 0; i < size / 2; i++) {
        const uint8_t temp  = data[i];
        data[i]             = data[size - 1 - i];
        data[size - 1 - i]  = temp;
    }
}

// ---- Functii interne --------------------------------------------------------

// citeste segmentul SOF (start of frame) pt latime, inaltime, nr componente
static void parseSof(FILE *f, uint16_t L, JpegImage *img) {
    const long pos = ftell(f);

    fseek(f, 1, SEEK_CUR); // sarim un byte de precizie nu ne trebuie acum

    fread(&img->height, sizeof(uint16_t), 1, f);
    swapBytes(&img->height, sizeof(img->height));

    fread(&img->width, sizeof(uint16_t), 1, f);
    swapBytes(&img->width, sizeof(img->width));

    fread(&img->numComponents, sizeof(uint8_t), 1, f);

    printf("\nSOF -> width: %d, height: %d, componente: %d",
           img->width, img->height, img->numComponents);

    fseek(f, pos, SEEK_SET); // intoarcem la inceput ca sarim cu L-2 mai incolo
}

// citeste tabelele huffman (pot fi mai multe in acelas segment)
static void parseDht(FILE *f, uint16_t L, JpegImage *img) {
    const long pos         = ftell(f);
    int        bytesRead   = 0;
    const int  payloadSize = L - 2;

    while (bytesRead < payloadSize) {
        uint8_t infoByte;
        fread(&infoByte, sizeof(uint8_t), 1, f);
        bytesRead += 1;

        const uint8_t tableClass = (infoByte & 0xF0) >> 4; // 0 = DC, 1 = AC
        const uint8_t tableId    = (infoByte & 0x0F);       // id-ul tabelului 0-3

        uint8_t *lengthsArr = (tableClass == 0)
                                  ? img->dcHuffmanLengths[tableId]
                                  : img->acHuffmanLengths[tableId];
        uint8_t *valuesArr  = (tableClass == 0)
                                  ? img->dcHuffmanValues[tableId]
                                  : img->acHuffmanValues[tableId];

        fread(lengthsArr, sizeof(uint8_t), 16, f); // 16 bytes pt lungimi
        bytesRead += 16;

        // numaram cate simboluri sunt total
        int totalSymbols = 0;
        for (int i = 0; i < 16; i++) {
            totalSymbols += lengthsArr[i];
        }

        fread(valuesArr, sizeof(uint8_t), totalSymbols, f);
        bytesRead += totalSymbols;

        printf("\nDHT -> clasa: %d, id: %d, simboluri: %d", tableClass, tableId, totalSymbols);

        // construim mapa de coduri huffman din lungimi si valori
        HuffmanCode *codeMap = (tableClass == 0)
                                   ? img->dcCodes[tableId]
                                   : img->acCodes[tableId];
        uint16_t code  = 0;
        int      index = 0;
        for (uint8_t len = 1; len <= 16; len++) {
            for (int i = 0; i < lengthsArr[len - 1]; i++) {
                codeMap[valuesArr[index]].code   = code;
                codeMap[valuesArr[index]].length = len;
                code++;
                index++;
            }
            code <<= 1; // shift stanga pt urmatoarea lungime
        }
    }

    fseek(f, pos, SEEK_SET);
}

// citeste tabelele de cuantizare, pot fi 8 sau 16 biti
static void parseDqt(FILE *f, uint16_t L, JpegImage *img) {
    const long pos         = ftell(f);
    int        bytesRead   = 0;
    const int  payloadSize = L - 2;

    while (bytesRead < payloadSize) {
        uint8_t infoByte;
        fread(&infoByte, sizeof(uint8_t), 1, f);
        bytesRead += 1;

        const uint8_t precision = infoByte >> 4;   // 0 = 8bit, 1 = 16bit
        const uint8_t tableId   = infoByte & 0x0F; // id tabel

        if (precision == 0) {
            // 8 biti, citim direct
            for (int i = 0; i < 64; i++) {
                uint8_t val;
                fread(&val, sizeof(uint8_t), 1, f);
                img->quantTable[tableId][i] = val;
            }
            bytesRead += 64;
        } else {
            // 16 biti, trebuie sa facem swap
            for (int i = 0; i < 64; i++) {
                uint16_t val;
                fread(&val, sizeof(uint16_t), 1, f);
                swapBytes(&val, sizeof(uint16_t));
                img->quantTable[tableId][i] = val;
            }
            bytesRead += 128;
        }

        printf("\nDQT -> id: %d, precizie: %d biti", tableId, (precision == 0) ? 8 : 16);
    }

    fseek(f, pos, SEEK_SET);
}

// ---- API public -------------------------------------------------------------

int jpeg_parse_segments(FILE *f, JpegImage *img) {

    // verificam ca e intr-adevar un fisier jpeg cu markerul de inceput
    uint16_t soi;
    fread(&soi, sizeof(uint16_t), 1, f);
    swapBytes(&soi, sizeof(uint16_t));
    if (soi != 0xFFD8) {
        fprintf(stderr, "Eroare: nu e un fisier jpeg valid (marker SOI gresit)\n");
        return 1;
    }
    printf("Marker SOI gasit: 0x%X\n", soi);

    uint16_t marker;
    uint16_t L;

    while (fread(&marker, sizeof(uint16_t), 1, f) == 1) {
        swapBytes(&marker, sizeof(uint16_t));

        if (marker == 0xFFDA) { // SOS marker, ne oprim, jpeg_read_sos se ocupa
            break;
        }

        fread(&L, sizeof(uint16_t), 1, f);
        swapBytes(&L, sizeof(L));

        switch (marker) {
            case 0xFFC0: // SOF0 - baseline dct
            case 0xFFC2: // SOF2 - progressive dct
                parseSof(f, L, img);
                break;

            case 0xFFC4: // DHT - tabel huffman
                parseDht(f, L, img);
                break;

            case 0xFFDB: // DQT - tabel cuantizare
                parseDqt(f, L, img);
                break;

            default:
                printf("\nSarim marker: 0x%X (lungime %d)", marker, L);
                break;
        }

        fseek(f, L - 2, SEEK_CUR); // sarim tot segmentul, L include si cei 2 bytes pt lungime
    }

    return 0;
}

int jpeg_read_sos(FILE *f, JpegImage *img) {
    // citim lungimea headerului SOS
    uint16_t sosLen;
    fread(&sosLen, sizeof(uint16_t), 1, f);
    swapBytes(&sosLen, sizeof(uint16_t));
    printf("\nSOS lungime: 0x%X", sosLen);

    uint8_t numComp;
    fread(&numComp, sizeof(uint8_t), 1, f);
    printf("\nSOS componente: %d", numComp);

    for (int i = 0; i < numComp; i++) {
        fread(&img->components[i].compId, sizeof(uint8_t), 1, f);

        uint8_t huffByte;
        fread(&huffByte, sizeof(uint8_t), 1, f);
        img->components[i].DCHuff = (huffByte & 0xF0) >> 4; // nibble sus = DC
        img->components[i].ACHuff = (huffByte & 0x0F);      // nibble jos = AC

        printf("\n  Componenta %d -> DC: %d, AC: %d",
               img->components[i].compId,
               img->components[i].DCHuff,
               img->components[i].ACHuff);
    }

    // Ss, Se, Ah/Al - nu ne trebuie acum, sarim peste ei
    uint8_t dummy;
    fread(&dummy, sizeof(uint8_t), 1, f);
    fread(&dummy, sizeof(uint8_t), 1, f);
    fread(&dummy, sizeof(uint8_t), 1, f);

    return 0; // file pointerul e acum la datele comprimate huffman
}
