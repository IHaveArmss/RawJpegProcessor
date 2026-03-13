#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef struct {
    uint8_t compId;
    uint8_t DCHuff;
    uint8_t ACHuff;
}ComponentInfo;

typedef struct {
    uint16_t code;
    uint8_t length;
} HuffmanNode;

//swap for little endian shitty byte thigns
void swapBytes(void *p, const size_t size) {
    uint8_t *data = (uint8_t *)p;
    for (size_t i = 0;i < size/2;i++) {
        const uint8_t temp = data[i];
        data[i] = data[size-1-i];
        data[size-1-i] = temp;
    }
}

uint8_t dcHuffmanLengths[4][16];
uint8_t dcHuffmanValues[4][256];

uint8_t acHuffmanLengths[4][16];
uint8_t acHuffmanValues[4][256];

int main(const int argc, char *argv[]) {

    if (argc !=2) {
        printf("not enough arguments");
        return 1;
    }
    FILE *f = fopen(argv[1],"rb");

    if (f == NULL) {
        printf("Error opening file\n");
        return 1;
    }
    uint16_t jpgStructure;
    fread(&jpgStructure,sizeof(uint16_t),1,f);

    printf("%x\n",jpgStructure);
    swapBytes(&jpgStructure,sizeof(uint16_t));
    printf("%x",jpgStructure);
    uint16_t pairMarkers;
    uint16_t L;

    uint8_t quantTable[4][64];
    // MAIN WHILE
    while (fread(&pairMarkers,sizeof(uint16_t),1,f)==1) {

        swapBytes(&pairMarkers,sizeof(uint16_t));

        if (pairMarkers==0xFFDA) {
            break;
        }

        fread(&L,sizeof(uint16_t),1,f);
        swapBytes(&L,sizeof(L));

        if (pairMarkers==0xFFC0 || pairMarkers ==0xFFC2) {
            const long pos = ftell(f);
            fseek(f,1,SEEK_CUR);

            uint16_t imgHeight;
            fread(&imgHeight,sizeof(uint16_t),1,f);
            swapBytes(&imgHeight,sizeof(imgHeight));

            uint16_t imgWidth;
            fread(&imgWidth,sizeof(uint16_t),1,f);
            swapBytes(&imgWidth,sizeof(imgWidth));

            printf("\n%d %d",imgWidth,imgHeight);
            fseek(f,pos,SEEK_SET);
        }
        //HUFFMAN TABLES
        if (pairMarkers == 0XFFC4) {
            const long pos = ftell(f);

            int bytesRead = 0;
            int payloadSize = L-2;

            while (bytesRead<payloadSize) {
                uint8_t infoByte;
                fread(&infoByte, sizeof(uint8_t),1,f);
                bytesRead +=1;

                uint8_t tableClass = (infoByte & 0xF0) >>4;
                uint8_t tableID = (infoByte & 0x0F);

                uint8_t *lenghtsArray = (tableClass==0) ? dcHuffmanLengths[tableID] : acHuffmanLengths[tableID];
                uint8_t *valuesArray = (tableClass ==0) ? dcHuffmanValues[tableID] : acHuffmanValues[tableID];

                fread(lenghtsArray,sizeof(uint8_t),16,f);
                bytesRead += 16;

                int totalSymbols = 0;
                for (int i=0;i <16 ;i++) {
                    totalSymbols = totalSymbols + lenghtsArray[i];
                }

                fread(valuesArray, sizeof(uint8_t),totalSymbols,f);
                bytesRead = bytesRead + totalSymbols;
                printf("\nSaved Huffman Table Class: %d, ID: %d, Total symbols: %d", tableClass, tableID, totalSymbols);
            }
            fseek(f,pos,SEEK_SET);
        }
        //IMAGE QUANT TABLE MANIPULATION
        if (pairMarkers == 0xFFDB) {
            const long pos = ftell(f);

            int bytesRead = 0;
            int payloadSize = L-2;

            while (bytesRead < payloadSize) {
                uint8_t infoByte;
                bytesRead += 1;

                fread(&infoByte,sizeof(uint8_t),1,f);

                uint8_t precision = infoByte >> 4;
                uint8_t tableID = infoByte & 0x0F;

                if (precision == 0) {
                    for (int i = 0; i<64; i++) {
                        uint8_t val;
                        fread(&val, sizeof(uint8_t), 1, f);
                        quantTable[tableID][i] = val;
                    }
                    bytesRead += 64;
                } else {
                    // Precizie 16biti: citim 64 de valori pe 2 octeti (128 octeti total)
                    for (int i = 0; i < 64; i++) {
                        uint16_t val;
                        fread(&val,sizeof(uint16_t), 1, f);
                        swapBytes(&val,sizeof(uint16_t));
                        quantTable[tableID][i] = val;
                    }
                    bytesRead += 128;
                }

                printf("\n Saved quant table id %d (Precision: %d)", tableID, precision);
            }

            fseek(f,pos,SEEK_SET);
        }

        fseek(f,L-2,SEEK_CUR);
    }
    //after finding the ffda marker break the while
    uint16_t sosLenght;
    fread(&sosLenght,sizeof(uint16_t),1,f);
    swapBytes(&sosLenght,sizeof(uint16_t));
    printf("\n%x",sosLenght);

    uint8_t numberComponents;
    fread(&numberComponents,sizeof(uint8_t),1,f);
    printf("\n%d" ,numberComponents);

    ComponentInfo components[4];
    uint8_t trashVariable;
    for (int i=0;i<numberComponents;i++) {

        fread(&components[i].compId,sizeof(uint8_t),1,f);


        fread(&trashVariable,sizeof(uint8_t),1,f);

        const uint8_t mask1 = 0xf0;
        components[i].DCHuff = (trashVariable&mask1)>>4 ;

        components[i].ACHuff = (trashVariable&0x0F);

        }
    fread(&trashVariable,sizeof(uint8_t),1,f);
    printf("\n %x",trashVariable);
    fread(&trashVariable,sizeof(uint8_t),1,f);
    printf("\n %x",trashVariable);
    fread(&trashVariable,sizeof(uint8_t),1,f);
    printf("\n %x",trashVariable);
    //de aici citimi huffman bytestuffing hell



    fclose(f);
    return 0;
}