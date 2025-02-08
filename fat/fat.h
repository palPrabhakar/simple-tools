#ifndef FAT_H
#define FAT_H

#include <stdint.h>
#include <stdio.h>

#define FAT16

#define cprintf(format, ...) printf(format, __VA_ARGS__);

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
} __attribute__((__packed__)) dir_t;

typedef struct fat_fuse {
    FILE *fp;
    size_t root_dir_ent;
    unsigned int first_data_sec;
    size_t sec_per_clus;
    size_t bytes_per_sec;
    dir_t *root_dir;
    uint16_t *fat_dir;
    size_t fat_ent;
} fat_fuse;

#define GET_SECTOR_OFFSET(cluster, ff)                                         \
    ((((cluster - 2) * ff->sec_per_clus) + ff->first_data_sec) *               \
     ff->bytes_per_sec)

char **parse_path(char *, size_t *);
int read_dir(FILE *, size_t, size_t, dir_t *);
int get_dir(char **, size_t, size_t, fat_fuse *, dir_t *, size_t, dir_t *);
int read_dir_entries(fat_fuse *, dir_t, dir_t **, size_t *);
#endif
