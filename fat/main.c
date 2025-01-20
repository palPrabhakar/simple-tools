#define FUSE_USE_VERSION 31

#define _FILE_OFFSET_BITS 64

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static struct fuse_operations myfs_ops = {};

static char *devfile = NULL;
static FILE *fp = NULL;

#define FAT16

#define cprintf(format, ptr) printf(format, (int)sizeof(ptr), ptr);

typedef struct {
    uint32_t BS_jmpBoot : 24;
    char BS_OEMName[8];
    uint16_t BPB_BytsPerSec;
    uint8_t BPB_SecPerClus;
    uint16_t BPB_RsvdSecCnt;
    uint8_t BPB_NumFATs;
    uint16_t BPB_RootEntCnt;
    uint16_t BPB_TotSec16;
    uint8_t BPB_Media;
    uint16_t BPB_FATSz16;
    uint16_t BPB_SecPerTrk;
    uint16_t BPB_NumHeads;
    uint32_t BPB_HiddSec;
    uint32_t BPB_TotSec32;
#ifdef FAT16
    uint8_t BS_DrvNum;
    uint8_t BS_Reserved1;
    uint8_t BS_BootSig;
    uint32_t BS_VolID;
    char BS_VolLab[11];
    char BS_FilSysType[8];
#endif
} __attribute__((__packed__)) BPB;

typedef struct {
    char DIR_Name[11];
    uint8_t DIR_Attr;
    uint8_t DIR_NTRes;
    uint8_t DIR_CrtTimeTenth;
    uint16_t DIR_CrtTime;
    uint16_t DIR_CrtDate;
    uint16_t DIR_LstAccDate;
    uint16_t DIR_FstClusHI;
    uint16_t DIR_WrtTime;
    uint16_t DIR_WrtDate;
    uint16_t DIR_FstClusLO;
    uint32_t DIR_FileSize;
} __attribute__((__packed__)) Dir_t;

void read_file() {

    BPB bpb;
    printf("reading file: %lu\n", sizeof(BPB));

    printf("---------------------------------\n");

    if (fread(&bpb, sizeof(BPB), 1, fp)) {
        cprintf("BS_OEMName: %.*s\n", bpb.BS_OEMName);
        printf("BS_BytsPerSec: %d\n", bpb.BPB_BytsPerSec);
        printf("BPB_SecPerClus: %d\n", bpb.BPB_SecPerClus);
        printf("BPB_RsvdSecCnt: %d\n", bpb.BPB_RsvdSecCnt);
        printf("BPB_NumFATs: %d\n", bpb.BPB_NumFATs);
        printf("BPB_RootEntCnt: %d\n", bpb.BPB_RootEntCnt);
        printf("BPB_TotSec16: %d\n", bpb.BPB_TotSec16);
        printf("BPB_Media: %d\n", bpb.BPB_Media);
        printf("BPB_FATSz16: %d\n", bpb.BPB_FATSz16);
        printf("BPB_SecPerTrk: %d\n", bpb.BPB_SecPerTrk);
        printf("BPB_NumHeads: %d\n", bpb.BPB_NumHeads);
        printf("BPB_HiddSec: %d\n", bpb.BPB_HiddSec);
        printf("BPB_TotSec32: %d\n", bpb.BPB_TotSec32);
        printf("BS_VolID: %u\n", bpb.BS_VolID);
        cprintf("BS_VolLab: %.*s\n", bpb.BS_VolLab);
        cprintf("BS_FilSysType: %.*s\n", bpb.BS_FilSysType);
    } else {
        printf("Error Reading\n");
        exit(1);
    }

    printf("---------------------------------\n");

    size_t RootDirSectors =
        ((bpb.BPB_RootEntCnt * 32) + (bpb.BPB_BytsPerSec - 1)) /
        bpb.BPB_BytsPerSec;

    size_t FATSz;
    size_t TotSec;
    if (bpb.BPB_FATSz16 != 0) {
        FATSz = bpb.BPB_FATSz16;
    } else {
        // TODO:
        // Read Specification
    }

    if (bpb.BPB_TotSec16 != 0) {
        TotSec = bpb.BPB_TotSec16;
    } else {
        TotSec = bpb.BPB_TotSec32;
    }

    size_t DataSec = TotSec - (bpb.BPB_RsvdSecCnt + (bpb.BPB_NumFATs * FATSz)
    +
                               RootDirSectors);

    size_t clusterCount = DataSec / bpb.BPB_SecPerClus;

    printf("Cluster Count: %lu\n", clusterCount);

    if (clusterCount > 4085 && clusterCount < 65525) {
        printf("FAT Type: FAT16\n");
    }

    printf("---------------------------------\n");

    uint32_t FATStartSector = bpb.BPB_RsvdSecCnt;
    uint32_t FATSectorOffset = FATStartSector * bpb.BPB_BytsPerSec;
    uint32_t FATSize = FATSz * bpb.BPB_NumFATs;

    uint16_t value;
    fseek(fp, FATSectorOffset, SEEK_SET);
    fread(&value, sizeof(uint16_t), 1, fp);
    printf("FAT[0] - Expected: %d, Found: %d\n", 0xFFF8, value);
    fread(&value, sizeof(uint16_t), 1, fp);
    printf("FAT[1] - Expected: %d, Found: %d\n", 0xFFFF, value);

    printf("---------------------------------\n");

    uint32_t RootDirStartSector = FATStartSector + FATSize;
    uint32_t RootDirOffset = RootDirStartSector * bpb.BPB_BytsPerSec;
    printf("sizeof(Dir_t): %lu\n", sizeof(Dir_t));
    printf("Root Directory Sectors: %lu\n", RootDirSectors);
    fseek(fp, RootDirOffset, SEEK_SET);
    size_t dir_size = 4;
    Dir_t dir[dir_size];
    fread(&dir, sizeof(Dir_t), dir_size, fp);
    cprintf("DIR_Name[0]: %.*s\n", dir[0].DIR_Name);
    printf("DIR_Attr[0]: %d\n", dir[0].DIR_Attr);
    printf("DIR_NTRes[0]: %d\n", dir[0].DIR_NTRes);
    printf("DIR_CrtTimeTenth[0]: %d\n", dir[0].DIR_CrtTimeTenth);

    cprintf("DIR_Name[1]: %.*s\n", dir[1].DIR_Name);
    printf("DIR_Attr[1]: %d\n", dir[1].DIR_Attr);
    printf("DIR_NTRes[1]: %d\n", dir[1].DIR_NTRes);
    printf("DIR_CrtTimeTenth[1]: %d\n", dir[1].DIR_CrtTimeTenth);

    cprintf("DIR_Name[2]: %.*s\n", dir[2].DIR_Name);
    printf("DIR_Attr[2]: %d\n", dir[2].DIR_Attr);
    printf("DIR_NTRes[2]: %d\n", dir[2].DIR_NTRes);
    printf("DIR_CrtTimeTenth[2]: %d\n", dir[2].DIR_CrtTimeTenth);

    cprintf("DIR_Name[3]: %.*s\n", dir[3].DIR_Name);
    printf("DIR_Attr[3]: %d\n", dir[3].DIR_Attr);
    printf("DIR_NTRes[3]: %d\n", dir[3].DIR_NTRes);
    printf("DIR_CrtTimeTenth[3]: %d\n", dir[3].DIR_CrtTimeTenth);

    printf("---------------------------------\n");

    printf("done reading\n");
}

int main(int argc, char **argv) {
    // get the device or image filename from arguments
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] != '-') {
            devfile = realpath(argv[i], NULL);
            memcpy(&argv[i], &argv[i + 1], (argc - i) * sizeof(argv[0]));
            argc--;
            break;
        }
    }

    fp = fopen(devfile, "rb");
    if (!fp) {
        exit(1);
    }

    read_file();
    fclose(fp);

    return fuse_main(argc, argv, &myfs_ops, NULL);
}
