#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define BLOCK_SIZE 4096
#define TOTAL_BLOCKS 64
#define SUPERBLOCK_MAGIC 0xD34D
#define INODE_COUNT 128
#define INODE_SIZE 256

int update = 0;

typedef struct {
    uint16_t magic;
    uint32_t block_size;
    uint32_t total_blocks;
    uint32_t inode_bitmap_block;
    uint32_t data_bitmap_block;
    uint32_t inode_table_start;
    uint32_t first_data_block;
    uint32_t inode_size;
    uint32_t inode_count;
} Superblock;

int superblock_validator(Superblock *sb);

int main(int argc, char *argv[]){
if (argc != 2){
  printf("File Input Error\n");
  return 1;
}

FILE *fd = fopen(argv[1], "r+b");
Superblock sb;
fread(&sb, sizeof(Superblock), 1, fd); // Superblock read kore stores into the sb struct

if (update == 1){ // Jodi superblock e issue update hy actual superblock e upadate korte hobe
  fseek(fd, 0, SEEK_SET);
  fwrite(&sb, sizeof(Superblock), 1, fd); 
  update = 0;
  printf("Fix: Superblock updated successfully.\n");
}


int superblock_validator(Superblock *sb){
  if (sb->magic != SUPERBLOCK_MAGIC){ // Should be 0xD34D
    printf("Issue: Superblock | Magic Number is incorrect.\n");
    sb->magic = SUPERBLOCK_MAGIC;
    update = 1;
  }
  if (sb->block_size != BLOCK_SIZE){ // Should be 4096
    printf("Issue: Superblock | Block Size is incorrect.\n");
    sb->block_size = BLOCK_SIZE;
    update = 1;
  }
  if (sb->total_blocks != TOTAL_BLOCKS){ // Should be 64
    printf("Issue: Superblock | Total number block is incorrect.\n");
    sb->total_blocks = TOTAL_BLOCKS;
    update = 1;
  }
  if (sb->inode_bitmap_block != 1){ // Block 1 e ibitmap
    printf("Issue: Superblock | Inode Bitmap block is incorrect.\n");
    sb->inode_bitmap_block = 1;
    update = 1;
  }
  if (sb->data_bitmap_block != 1){ // Block 2 e dbitmap
    printf("Issue: Superblock | Data Bitmap block is incorrect.\n");
    sb->data_bitmap_block = 2;
    update = 1;
  }
  if (sb->inode_table_start != 3){ // Block 3 e inode table start
    printf("Issue: Superblock | Inode Table Start is incorrect.\n");
    sb->inode_table_start = 3;
    update = 1;
  }
  if (sb->first_data_block != 8){ // Block 8 e data block start
    printf("Issue: Superblock | First Data Block is incorrect.\n");
    sb->first_data_block = 8;
    update = 1;
  }
  if (sb->inode_size != INODE_SIZE){ // Should be 256
    printf("Issue: Superblock | Inode Size is incorrect.\n");
    sb->inode_size = INODE_SIZE;
    update = 1;
  }

  return 0;
}