#include "RKImage.h"
#include "RKBoot.h"
#include "DefineHeader.h"
extern UINT CRC_32(unsigned char* pData, UINT size);
extern unsigned short CRC_16(unsigned char* pData, UINT size);
extern void P_RC4(unsigned char* buf, unsigned short len);


#define SECTOR_SIZE 512

static bool MakeSector0(PBYTE pSector, USHORT usFlashDataSec, USHORT usFlashBootSec, bool rc4Flag)
{
    PRK28_IDB_SEC0 pSec0;
    memset(pSector, 0, SECTOR_SIZE);
    pSec0 = (PRK28_IDB_SEC0)pSector;

    pSec0->dwTag = 0x0FF0AA55;
    pSec0->uiRc4Flag = rc4Flag;
    pSec0->usBootCode1Offset = 0x4;
    pSec0->usBootCode2Offset = 0x4;
    pSec0->usBootDataSize = usFlashDataSec;
    pSec0->usBootCodeSize = usFlashDataSec + usFlashBootSec;
    return true;
}

static bool MakeSector1(PBYTE pSector)
{
    PRK28_IDB_SEC1 pSec1;
    memset(pSector, 0, SECTOR_SIZE);
    pSec1 = (PRK28_IDB_SEC1)pSector;

    pSec1->usSysReservedBlock = 0xC;
    pSec1->usDisk0Size = 0xFFFF;
    pSec1->uiChipTag = 0x38324B52;
    return true;
}

static bool MakeSector2(PBYTE pSector)
{
    PRK28_IDB_SEC2 pSec2;
    memset(pSector, 0, SECTOR_SIZE);
    pSec2 = (PRK28_IDB_SEC2)pSector;

    strcpy(pSec2->szVcTag, "VC");
    strcpy(pSec2->szCrcTag, "CRC");
    return true;
}

static bool MakeSector3(PBYTE pSector)
{
    memset(pSector, 0, SECTOR_SIZE);
    return true;
}

static int MakeIDBlockData(PBYTE pDDR, PBYTE pLoader, PBYTE lpIDBlock, USHORT usFlashDataSec, USHORT usFlashBootSec, DWORD dwLoaderDataSize, DWORD dwLoaderSize, bool rc4Flag)
{
    RK28_IDB_SEC0 sector0Info;
    RK28_IDB_SEC1 sector1Info;
    RK28_IDB_SEC2 sector2Info;
    RK28_IDB_SEC3 sector3Info;
    UINT i;
    MakeSector0((PBYTE)&sector0Info, usFlashDataSec, usFlashBootSec, rc4Flag);
    MakeSector1((PBYTE)&sector1Info);
    if (!MakeSector2((PBYTE)&sector2Info))
        return -6;
    if (!MakeSector3((PBYTE)&sector3Info))
        return -7;
    sector2Info.usSec0Crc = CRC_16((PBYTE)&sector0Info, SECTOR_SIZE);
    sector2Info.usSec1Crc = CRC_16((PBYTE)&sector1Info, SECTOR_SIZE);
    sector2Info.usSec3Crc = CRC_16((PBYTE)&sector3Info, SECTOR_SIZE);

    memcpy(lpIDBlock, &sector0Info, SECTOR_SIZE);
    memcpy(lpIDBlock + SECTOR_SIZE, &sector1Info, SECTOR_SIZE);
    memcpy(lpIDBlock + SECTOR_SIZE * 3, &sector3Info, SECTOR_SIZE);

    if (rc4Flag) {
        for (i = 0; i < dwLoaderDataSize/SECTOR_SIZE; i++)
            P_RC4(pDDR + i * SECTOR_SIZE, SECTOR_SIZE);
        for (i = 0; i < dwLoaderSize/SECTOR_SIZE; i++)
            P_RC4(pLoader + i * SECTOR_SIZE, SECTOR_SIZE);
    }

    memcpy(lpIDBlock + SECTOR_SIZE * 4, pDDR, dwLoaderDataSize);
    memcpy(lpIDBlock + SECTOR_SIZE * (4 + usFlashDataSec), pLoader, dwLoaderSize);

    sector2Info.uiBootCodeCrc = CRC_32((PBYTE)(lpIDBlock + SECTOR_SIZE * 4), sector0Info.usBootCodeSize * SECTOR_SIZE);
    memcpy(lpIDBlock + SECTOR_SIZE * 2, &sector2Info, SECTOR_SIZE);
    for(i = 0; i < 4; i++) {
        if(i == 1) {
            continue;
        } else {
            P_RC4(lpIDBlock + SECTOR_SIZE * i, SECTOR_SIZE);
        }
    }
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        printf("Usage: %s <loader.bin> <output_idb.bin>\n", argv[0]);
        return 1;
    }
    bool ok;
    CRKImage img(argv[1], ok);
    if (!ok) {
        fprintf(stderr, "Failed to open loader %s\n", argv[1]);
        return 1;
    }

    CRKBoot *boot = (CRKBoot *)img.m_bootObject;
    if (!boot) {
        fprintf(stderr, "Invalid loader file\n");
        return 1;
    }

    char loaderCodeName[] = "FlashBoot";
    char loaderDataName[] = "FlashData";
    char loaderHeadName[] = "FlashHead";
    DWORD dwLoaderSize=0, dwLoaderDataSize=0, dwLoaderHeadSize=0, dwDelay;
    PBYTE loaderCodeBuffer = NULL;
    PBYTE loaderDataBuffer = NULL;
    PBYTE loaderHeadBuffer = NULL;
    bool hasHead = false;

    signed char index = boot->GetIndexByName(ENTRYLOADER, loaderCodeName);
    if (index == -1 || !boot->GetEntryProperty(ENTRYLOADER, index, dwLoaderSize, dwDelay)) {
        fprintf(stderr, "Failed to get loader code\n");
        return 1;
    }
    loaderCodeBuffer = new BYTE[dwLoaderSize];
    memset(loaderCodeBuffer, 0, dwLoaderSize);
    boot->GetEntryData(ENTRYLOADER, index, loaderCodeBuffer);

    index = boot->GetIndexByName(ENTRYLOADER, loaderDataName);
    if (index == -1 || !boot->GetEntryProperty(ENTRYLOADER, index, dwLoaderDataSize, dwDelay)) {
        fprintf(stderr, "Failed to get loader data\n");
        delete[] loaderCodeBuffer;
        return 1;
    }
    loaderDataBuffer = new BYTE[dwLoaderDataSize];
    memset(loaderDataBuffer, 0, dwLoaderDataSize);
    boot->GetEntryData(ENTRYLOADER, index, loaderDataBuffer);

    index = boot->GetIndexByName(ENTRYLOADER, loaderHeadName);
    if (index != -1 && boot->GetEntryProperty(ENTRYLOADER, index, dwLoaderHeadSize, dwDelay)) {
        loaderHeadBuffer = new BYTE[dwLoaderHeadSize];
        memset(loaderHeadBuffer, 0, dwLoaderHeadSize);
        boot->GetEntryData(ENTRYLOADER, index, loaderHeadBuffer);
        hasHead = true;
    }

    USHORT usFlashDataSec = (ALIGN(dwLoaderDataSize, 2048)) / SECTOR_SIZE;
    USHORT usFlashBootSec = (ALIGN(dwLoaderSize, 2048)) / SECTOR_SIZE;
    USHORT usFlashHeadSec = 0;
    DWORD dwSectorNum;

    if (hasHead) {
        usFlashHeadSec = (ALIGN(dwLoaderHeadSize, 2048)) / SECTOR_SIZE;
        dwSectorNum = usFlashHeadSec + usFlashDataSec + usFlashBootSec;
    } else {
        dwSectorNum = 4 + usFlashDataSec + usFlashBootSec;
    }

    PBYTE idb = new BYTE[dwSectorNum * SECTOR_SIZE];
    memset(idb, 0, dwSectorNum * SECTOR_SIZE);

    if (hasHead) {
        if (boot->Rc4DisableFlag) {
            for (UINT i = 0; i < dwLoaderHeadSize/SECTOR_SIZE; i++)
                P_RC4(loaderHeadBuffer + SECTOR_SIZE*i, SECTOR_SIZE);
            for (UINT i = 0; i < dwLoaderDataSize/SECTOR_SIZE; i++)
                P_RC4(loaderDataBuffer + SECTOR_SIZE*i, SECTOR_SIZE);
            for (UINT i = 0; i < dwLoaderSize/SECTOR_SIZE; i++)
                P_RC4(loaderCodeBuffer + SECTOR_SIZE*i, SECTOR_SIZE);
        }
        memcpy(idb, loaderHeadBuffer, dwLoaderHeadSize);
        memcpy(idb + SECTOR_SIZE*usFlashHeadSec, loaderDataBuffer, dwLoaderDataSize);
        memcpy(idb + SECTOR_SIZE*(usFlashHeadSec+usFlashDataSec), loaderCodeBuffer, dwLoaderSize);
    } else {
        if (MakeIDBlockData(loaderDataBuffer, loaderCodeBuffer, idb, usFlashDataSec, usFlashBootSec, dwLoaderDataSize, dwLoaderSize, boot->Rc4DisableFlag) != 0) {
            fprintf(stderr, "Failed to make id block\n");
            delete[] idb;
            delete[] loaderCodeBuffer;
            delete[] loaderDataBuffer;
            if (loaderHeadBuffer) delete[] loaderHeadBuffer;
            return 1;
        }
    }

    FILE *out = fopen(argv[2], "wb");
    if (!out) {
        perror("fopen");
        delete[] idb;
        delete[] loaderCodeBuffer;
        delete[] loaderDataBuffer;
        if (loaderHeadBuffer) delete[] loaderHeadBuffer;
        return 1;
    }
    fwrite(idb, 1, dwSectorNum * SECTOR_SIZE, out);
    fclose(out);

    delete[] idb;
    delete[] loaderCodeBuffer;
    delete[] loaderDataBuffer;
    if (loaderHeadBuffer) delete[] loaderHeadBuffer;

    return 0;
}

