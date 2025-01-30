#define FUSE_USE_VERSION 31

#define _FILE_OFFSET_BITS 64

#include <errno.h>
#include <fuse.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "fat.h"

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

static void *fat_init(struct fuse_conn_info *conn, struct fuse_config *cfg) {
    printf("fat_init\n");

    fat_fuse *ff = malloc(sizeof(fat_fuse));

    ff->fp = fopen(options.filename, "rb");
    if (!ff->fp) {
        fprintf(stderr, "error: unable to open device file\n");
        goto set_err;
    }

    BPB bpb;
    if (!fread(&bpb, sizeof(BPB), 1, ff->fp) || (bpb.BPB_FATSz16 == 0)) {
        fprintf(stderr, "error: invalid fat16 filesystem\n");
        goto set_err;
    }

    size_t RootDirSectors =
        ((bpb.BPB_RootEntCnt * 32) + (bpb.BPB_BytsPerSec - 1)) /
        bpb.BPB_BytsPerSec;
    size_t TotSec = bpb.BPB_TotSec16 ? bpb.BPB_TotSec16 : bpb.BPB_TotSec32;
    size_t DataSec =
        TotSec - (bpb.BPB_RsvdSecCnt + (bpb.BPB_NumFATs * bpb.BPB_FATSz16) +
                  RootDirSectors);
    size_t clusterCount = DataSec / bpb.BPB_SecPerClus;
    if (clusterCount < 4085 || clusterCount > 65525) {
        fprintf(
            stderr,
            "error: invalid cluster count ERROR_EXITfor fat16 filesystem\n");
        goto set_err;
    }

    uint32_t FATStartSector = bpb.BPB_RsvdSecCnt;
    uint32_t FATSectorOffset = FATStartSector * bpb.BPB_BytsPerSec;
    uint32_t FATSize = bpb.BPB_FATSz16 * bpb.BPB_NumFATs;
    uint32_t RootDirStartSector = FATStartSector + FATSize;
    uint32_t RootDirOffset = RootDirStartSector * bpb.BPB_BytsPerSec;

    ff->fat_sec = FATStartSector;
    ff->fat_sec_off = FATSectorOffset;
    ff->root_dir = RootDirStartSector;
    ff->root_dir_off = RootDirOffset;

exit:
    return ff;

set_err:
    fuse_exit(fuse_get_context()->fuse);
    goto exit;
}

static int fat_getattr(const char *path, struct stat *stbuf,
                       struct fuse_file_info *fi) {
    printf("fat_getattr path: %s\n", path);

    (void)fi;
    int res = 0;

    char *fpath = strdup(path);
    size_t count = 0;
    char **plist = parse_path(fpath, &count);

    if (count == 0) {
        res = -ENOENT;
        goto exit;
    }

    if (count == 1) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    } else {
        // TODO:
        for (int i = 1; i < count; ++i) {
            char *file_or_dir = plist[i];
        }
    }

exit:
    free(fpath);
    free(plist);
    return res;
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

static void fat_destroy(void *data) {
    printf("fat_destroy\n");

    if (data) {
        fat_fuse *ff = (fat_fuse *)data;
        if (ff->fp)
            fclose(ff->fp);

        free(ff);
    }
}

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

    char *devfile = realpath(options.filename, NULL);
    if (devfile) {
        free((void *)options.filename);
        options.filename = devfile;
    } else {
        fprintf(stderr, "error: invalid filename specified\n");
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
    return ret;
}
