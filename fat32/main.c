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
} __attribute__((__packed__)) BPB;

void read_file() {
  BPB bpb;
  printf("reading file: %lu\n", sizeof(BPB));
  if (fread(&bpb, sizeof(BPB), 1, fp)) {
      printf("BS_OEMName: %s\n", bpb.BS_OEMName);
      printf("BS_BytsPerSec: %d\n", bpb.BPB_BytsPerSec);
      printf("BPB_SecPerClus: %d\n", bpb.BPB_SecPerClus);
      printf("BPB_RsvdSecCnt: %d\n", bpb.BPB_RsvdSecCnt);
      printf("BPB_NumFATs: %d\n", bpb.BPB_NumFATs);
      printf("BPB_RootEntCnt: %d\n", bpb.BPB_RootEntCnt);
      printf("BPB_TotSec16: %d\n", bpb.BPB_TotSec16);
  } else {
    printf("Error Reading\n");
    exit(1);
  }

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

  return fuse_main(argc, argv, &myfs_ops, NULL);
}
