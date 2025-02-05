#include <asm-generic/errno.h>
#include <stdint.h>
#include <stdio.h>
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

    uint32_t FirstDataSector = bpb.BPB_RsvdSecCnt +
                               (bpb.BPB_NumFATs * bpb.BPB_FATSz16) +
                               RootDirSectors;

    ff->fat_sec = FATStartSector;
    ff->fat_sec_off = FATSectorOffset;
    ff->root_dir = RootDirStartSector;
    ff->root_dir_off = RootDirOffset;
    ff->root_dir_ent = bpb.BPB_RootEntCnt;
    ff->first_data_sec = FirstDataSector;
    ff->sec_per_clus = bpb.BPB_SecPerClus;
    ff->bytes_per_sec = bpb.BPB_BytsPerSec;

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
    } else if (count == 2) {
        fat_fuse *ff = fuse_get_context()->private_data;
        fseek(ff->fp, ff->root_dir_off, SEEK_SET);
        dir_t *dir = malloc(sizeof(dir_t) * ff->root_dir_ent);
        fread(dir, sizeof(dir_t), ff->root_dir_ent, ff->fp);
        res = -ENOENT;
        for (size_t i = 0;
             i < ff->root_dir_ent && (uint8_t)dir[i].DIR_Name[0] != 0x00; ++i) {
            if ((uint8_t)dir[i].DIR_Name[0] != 0xE5) {
                char name[12] = {'\0'};
                size_t j;
                for (j = 0; j < 8 && dir[i].DIR_Name[j] != ' '; ++j)
                    name[j] = dir[i].DIR_Name[j];
                if (dir[i].DIR_Attr != 0x10) {
                    name[j++] = '.';
                    for (int k = 8; k < 11 && dir[i].DIR_Name[k] != ' '; ++k)
                        name[j++] = dir[i].DIR_Name[k];
                }

                if (strcmp(name, plist[0]) == 0) {
                    // TODO
                    // Set proper usage flags
                    if (dir[i].DIR_Attr == 0x10) {
                        stbuf->st_mode = S_IFDIR | 0755;
                        stbuf->st_nlink = 2;
                    } else {
                        stbuf->st_mode = S_IFREG | 0644;
                        stbuf->st_nlink = 1;
                        stbuf->st_size = dir[i].DIR_FileSize;
                    }
                    res = 0;
                    break;
                }
            }
        }
        free(dir);
    } else {
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        // TODO:
        // for (int i = 1; i < count; ++i) {
        //     // char *file_or_dir = plist[i];
        // }
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
    (void)offset;
    (void)fi;
    (void)flags;
    int res = 0;

    char *fpath = strdup(path);
    size_t count = 0;
    char **plist = parse_path(fpath, &count);

    if (count == 0) {
        res = -ENOENT;
        goto exit;
    }

    fat_fuse *ff = fuse_get_context()->private_data;

    if (count == 1) {
        fseek(ff->fp, ff->root_dir_off, SEEK_SET);
        dir_t *dir = malloc(sizeof(dir_t) * ff->root_dir_ent);
        fread(dir, sizeof(dir_t), ff->root_dir_ent, ff->fp);
        for (size_t i = 0;
             i < ff->root_dir_ent && (uint8_t)dir[i].DIR_Name[0] != 0x00; ++i) {
            if ((uint8_t)dir[i].DIR_Name[0] != 0xE5) {
                char name[12] = {'\0'};
                size_t j;
                for (j = 0; j < 8 && dir[i].DIR_Name[j] != ' '; ++j)
                    name[j] = dir[i].DIR_Name[j];
                if (dir[i].DIR_Attr != 0x10) {
                    name[j++] = '.';
                    for (int k = 8; k < 11 && dir[i].DIR_Name[k] != ' '; ++k) {
                        name[j++] = dir[i].DIR_Name[k];
                    }
                }
                filler(buf, name, NULL, 0, FUSE_FILL_DIR_PLUS);
            }
        }
        free(dir);
    } else {
        // TODO
    }

exit:
    free(fpath);
    free(plist);
    return res;
}

static int fat_open(const char *path, struct fuse_file_info *fi) {
    printf("fat_open path: %s\n", path);

    int res = 0;
    char *fpath = strdup(path);
    size_t count = 0;
    char **plist = parse_path(fpath, &count);

    if (count == 1) {
        res = -ENOENT;
    } else if (count == 2) {
        fat_fuse *ff = fuse_get_context()->private_data;
        fseek(ff->fp, ff->root_dir_off, SEEK_SET);
        dir_t *dir = malloc(sizeof(dir_t) * ff->root_dir_ent);
        fread(dir, sizeof(dir_t), ff->root_dir_ent, ff->fp);
        res = -ENOENT;
        for (size_t i = 0;
             i < ff->root_dir_ent && (uint8_t)dir[i].DIR_Name[0] != 0x00; ++i) {
            if ((uint8_t)dir[i].DIR_Name[0] != 0xE5) {
                char name[12] = {'\0'};
                size_t j;
                for (j = 0; j < 8 && dir[i].DIR_Name[j] != ' '; ++j)
                    name[j] = dir[i].DIR_Name[j];
                if (dir[i].DIR_Attr != 0x10) {
                    name[j++] = '.';
                    for (int k = 8; k < 11 && dir[i].DIR_Name[k] != ' '; ++k)
                        name[j++] = dir[i].DIR_Name[k];
                }

                if (strcmp(name, plist[0]) == 0) {
                    // If file found
                    // Set to the handle to the offset
                    res = 0;
                    fi->fh = ((dir[i].DIR_FstClusLO - 2) * ff->sec_per_clus +
                              ff->first_data_sec) *
                             ff->bytes_per_sec;
                    break;
                }
            }
        }
        free(dir);
    } else {
        // TODO:
    }

    free(fpath);
    free(plist);
    return res;
}

static int fat_read(const char *path, char *buf, size_t size, off_t offset,
                    struct fuse_file_info *fi) {
    printf("fat_read path: %s\n", path);
    (void)path;

    fat_fuse *ff = fuse_get_context()->private_data;
    fseek(ff->fp, fi->fh + offset, SEEK_SET);
    int bytes_read = fread(buf, sizeof(char), size, ff->fp);
    if (bytes_read == 0)
        return -errno;

    return bytes_read;
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
