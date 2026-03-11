#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>


//swap for preknown byte size
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

    while (fread(&pairMarkers,sizeof(uint16_t),1,f)==1) {
        swapBytes(&pairMarkers,sizeof(uint16_t));
        if (pairMarkers==0xFFC0 || pairMarkers ==0xFFC2) {
            const long pos = ftell(f);
            fseek(f,3,SEEK_CUR);

            uint16_t imgHeight;
            fread(&imgHeight,sizeof(uint16_t),1,f);
            swapBytes(&imgHeight,sizeof(imgHeight));

            uint16_t imgWidth;
            fread(&imgWidth,sizeof(uint16_t),1,f);
            swapBytes(&imgWidth,sizeof(imgWidth));

            printf("\n%d %d",imgWidth,imgHeight);
            fseek(f,pos,SEEK_SET);
        }
        if (pairMarkers==0xFFDA) {
            break;
        }
        fread(&L,sizeof(uint16_t),1,f);
        swapBytes(&L,sizeof(L));
        fseek(f,L-2,SEEK_CUR);
    }
    fread(&pairMarkers,sizeof(uint16_t),1,f);
    printf("\n%x",pairMarkers);
    fclose(f);
    return 0;
}