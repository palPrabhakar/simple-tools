#define FUSE_USE_VERSION 31

#define _FILE_OFFSET_BITS 64

#include <fuse.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// command-line options
static struct options {
    const char *filename;
    int help;
} options;

// clang-format off
#define OPTION(t, p) {t, offsetof(struct options, p), 1}
static const struct fuse_opt option_spec[] = {
    OPTION("--name=%s", filename),
    OPTION("--help", help),
    OPTION("-h", help),
    FUSE_OPT_END
};
// clang-format on

static void show_help(const char *program) {
    printf("usage: %s [options] <mountpoint>\n\n", program);
    printf("File-system specific options:\n"
           "    --name=<s>          Name of the \"device\" file\n"
           "\n");
}

static char *devfile = NULL;
static FILE *fp = NULL;

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
} __attribute__((__packed__)) Dir_t;

void read_file() {

    BPB bpb;
    printf("reading file: %lu\n", sizeof(BPB));

    printf("---------------------------------\n");

    if (fread(&bpb, sizeof(BPB), 1, fp)) {
        cprintf("BS_OEMName: %.*s\n", (int)sizeof(bpb.BS_OEMName),
                bpb.BS_OEMName);
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
        cprintf("BS_VolLab: %.*s\n", (int)sizeof(bpb.BS_VolLab), bpb.BS_VolLab);
        cprintf("BS_FilSysType: %.*s\n", (int)sizeof(bpb.BS_FilSysType),
                bpb.BS_FilSysType);
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

    size_t DataSec = TotSec - (bpb.BPB_RsvdSecCnt + (bpb.BPB_NumFATs * FATSz) +
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
    fread(dir, sizeof(Dir_t), dir_size, fp);
    for (size_t i = 0; i < dir_size; ++i) {
        cprintf("DIR_Name[%lu]: %.*s\n", i, (int)sizeof(dir[i].DIR_Name),
                dir[i].DIR_Name);
        printf("DIR_Attr[%lu]: %d\n", i, dir[i].DIR_Attr);
        printf("DIR_NTRes[%lu]: %d\n", i, dir[i].DIR_NTRes);
        printf("DIR_CrtTimeTenth[%lu]: %d\n", i, dir[i].DIR_CrtTimeTenth);
        printf("DIR_FstClusHI[%lu]: %d\n", i, dir[i].DIR_FstClusHI);
        printf("DIR_FstClusLO[%lu]: %d\n", i, dir[i].DIR_FstClusLO);
        printf("DIR_FileSize[%lu]: %d\n", i, dir[i].DIR_FileSize);
        printf("\n");
    }

    printf("---------------------------------\n");
    // read file
    uint32_t FirstDataSector =
        bpb.BPB_RsvdSecCnt + (bpb.BPB_NumFATs * FATSz) + RootDirSectors;
    uint32_t FirstSectorOfCluster =
        (dir[1].DIR_FstClusLO - 2) * bpb.BPB_SecPerClus + FirstDataSector;
    uint32_t FirstSectorOfClusterOffset =
        FirstSectorOfCluster * bpb.BPB_BytsPerSec;
    fseek(fp, FirstSectorOfClusterOffset, SEEK_SET);
    char file_buffer[dir[1].DIR_FileSize];
    fread(file_buffer, sizeof(char), dir[1].DIR_FileSize, fp);
    printf("\nFile Contents: %s\n", file_buffer);

    printf("---------------------------------\n");

    printf("done reading\n");
}

static void *fat_init(struct fuse_conn_info *conn, struct fuse_config *cfg) {
    printf("fat_init\n");
    devfile = realpath(options.filename, NULL);
    return NULL;
}

static int fat_getattr(const char *path, struct stat *stbuf,
                       struct fuse_file_info *fi) {
    printf("fat_getattr path: %s\n", path);
    stbuf->st_mode = S_IFDIR | 0755;
    stbuf->st_nlink = 2;
    return 0;
}

static int fat_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                       off_t offset, struct fuse_file_info *fi,
                       enum fuse_readdir_flags flags) {
    printf("fat_readdir path: %s\n", path);
    return 0;
}

static int fat_open(const char *path, struct fuse_file_info *fi) {
    printf("fat_open path: %s\n", path);
    return 0;
}

static int fat_read(const char *path, char *buf, size_t size, off_t offset,
                    struct fuse_file_info *fi) {
    printf("fat_read path: %s\n", path);
    return 0;
}

static int fat_write(const char *path, const char *buf, size_t size,
                     off_t offset, struct fuse_file_info *fi) {
    printf("fat_write path: %s\n", path);
    return 0;
}

static void fat_destroy(void *data) { printf("fat_destroy\n"); }

// clang-format off
static const struct fuse_operations fat_operations = {
    .init = fat_init,
    .getattr = fat_getattr,
    .readdir = fat_readdir,
    .open = fat_open,
    .read = fat_read,
    .write = fat_write,
    .destroy = fat_destroy
};
// clang-format on

int main(int argc, char **argv) {
    int ret;
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

    if (fuse_opt_parse(&args, &options, option_spec, NULL) == -1)
        return 1;

    if (!options.filename) {
        fprintf(stderr, "error: no filename specified\n");
        ret = 1;
        goto exit;
    }

    if (options.help) {
        show_help(argv[0]);
        if (fuse_opt_add_arg(&args, "--help") != 0) {
            ret = 1;
            goto exit;
        }
        args.argv[0][0] = '\0';
    }

    ret = fuse_main(args.argc, args.argv, &fat_operations, NULL);

exit:
    fuse_opt_free_args(&args);
    free(devfile); // calling free with null ptr is safe!
    return ret;
}
