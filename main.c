#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef struct {
    uint8_t compId;
    uint8_t DCHuff;
    uint8_t ACHuff;
}ComponentInfo;
//swap for little endian shitty byte thigns
void swapBytes(void *p, const size_t size) {
    uint8_t *data = (uint8_t *)p;
    for (size_t i = 0;i < size/2;i++) {
        const uint8_t temp = data[i];
        data[i] = data[size-1-i];
        data[size-1-i] = temp;
    }
}


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
        //IMAGE QUANT TABLE MANIPULATION
        if (pairMarkers == 0xFFDB) {
            const long pos = ftell(f);

            int bytesRead = 0;
            int payloadSize = L-2;

            while (bytesRead < payloadSize) {
                uint8_t infoByte;
                fread(&infoByte,sizeof(uint8_t),1,f);
                bytesRead += 1;

                uint8_t tableID = infoByte & 0x0F;

                fread(quantTable[tableID],sizeof(uint8_t),64,f);
                bytesRead += 64;

                printf("\n Saved quant table id %d",tableID);
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