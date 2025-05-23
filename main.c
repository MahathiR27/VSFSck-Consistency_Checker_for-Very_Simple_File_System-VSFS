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
    uint8_t reserved[4058];
} Superblock;

typedef struct {
    uint32_t mode;                    
    uint32_t uid;                    
    uint32_t gid;                     
    uint32_t file_size;                
    uint32_t last_access_time;       
    uint32_t creation_time;          
    uint32_t last_modification_time;
    uint32_t deletion_time;
    uint32_t hard_links;
    uint32_t data_blocks;
    uint32_t direct_block;
    uint32_t single_indirect_block;
    uint32_t double_indirect_block;
    uint32_t triple_indirect_block;
    uint8_t reserved[156];
} Inode;

int superblock_validator(Superblock *sb);
void inode_bitmap_validator(FILE *fs, uint8_t *inode_bitmap, uint32_t inode_table_start);
void data_bitmap_validator(FILE *fs, uint8_t *data_bitmap, uint32_t first_data_block, uint8_t *block_usage);
void duplicate_blocks(uint8_t *block_usage);
void bad_blocks(uint8_t *block_usage);


int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("File Input Error\n");
        return 1;
    }

    FILE *fs = fopen(argv[1], "r+b");

    Superblock sb;
    fseek(fs, 0, SEEK_SET);
    fread(&sb, sizeof(Superblock), 1, fs);  // Superblock read kore stores into the sb struct

    superblock_validator(&sb);
    if (update == 1) { // Jodi superblock e issue update hy actual superblock e upadate korte hobe
        fseek(fs, 0, SEEK_SET);
        fwrite(&sb, sizeof(Superblock), 1, fs);
        update = 0; 
        printf("Fix: Superblock updated successfully.\n");
    }

    uint8_t inode_bitmap[BLOCK_SIZE] = {0};

    fseek(fs, sb.inode_bitmap_block * BLOCK_SIZE, SEEK_SET); // inode bitmap block e seek
    fread(inode_bitmap, BLOCK_SIZE, 1, fs); 
    inode_bitmap_validator(fs, inode_bitmap, sb.inode_table_start);
    if (update == 1) { // Jodi inode bitmap e issue update hy actual inode bitmap e update korte hobe
        fseek(fs, sb.inode_bitmap_block * BLOCK_SIZE, SEEK_SET);
        fwrite(inode_bitmap, BLOCK_SIZE, 1, fs); 
        update = 0;
        printf("Fix: Inode Bitmap updated successfully.\n");
    }
    
    uint8_t data_bitmap[BLOCK_SIZE] = {0}; // Declare data bitmap
    uint8_t block_usage[TOTAL_BLOCKS] = {0}; // Declare block usage tracker

    // Read the data bitmap
    fseek(fs, sb.data_bitmap_block * BLOCK_SIZE, SEEK_SET);
    fread(data_bitmap, BLOCK_SIZE, 1, fs);

    // Validate and fix the data bitmap
    data_bitmap_validator(fs, data_bitmap, sb.first_data_block, block_usage);

    // Write back the updated data bitmap if any issues were fixed
    if (update == 1) {
        fseek(fs, sb.data_bitmap_block * BLOCK_SIZE, SEEK_SET);
        fwrite(data_bitmap, BLOCK_SIZE, 1, fs);
        update = 0; // Reset the update flag
        printf("Fix: Data Bitmap updated successfully.\n");
    }

    duplicate_blocks(block_usage);
    if (update == 1) {
        printf("Fix: Duplicate block issues updated successfully.\n");
        update = 0; 
    }

    // Check for bad blocks
    bad_blocks(block_usage);
    if (update == 1) {
        printf("Fix: Bad block issues updated successfully.\n");
        update = 0; 
    }

    fclose(fs);
    return 0;
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
  if (sb->data_bitmap_block != 2){ // Block 2 e dbitmap
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
  if (sb->inode_count > (BLOCK_SIZE * 5) / INODE_SIZE) { // Check inode count
    printf("Issue: Superblock | Inode Count exceeds maximum capacity.\n");
    sb->inode_count = (BLOCK_SIZE * 5) / INODE_SIZE;
    update = 1;
    }

  return 0;
}


void inode_bitmap_validator(FILE *fs, uint8_t *inode_bitmap, uint32_t inode_table_start) {
    for (int i = 0; i < INODE_COUNT; i++) {
        Inode inode;
        fseek(fs, inode_table_start * BLOCK_SIZE + i * INODE_SIZE, SEEK_SET);
        fread(&inode, sizeof(Inode), 1, fs);

        if (inode_bitmap[i / 8] & (1 << (i % 8))) { // inode used dekhaitese bitmap kintu ashole invalid
            if (inode.hard_links == 0 || inode.deletion_time != 0) { // inode is invalid
                printf("Issue: Inode bitmap | Inode %d marked as used but is invalid.\n", i);
                inode_bitmap[i / 8] &= ~(1 << (i % 8)); // update bitmap to unused
                update = 1;
            }
        } else { // inode unused dekhaitese bitmap kintu ashole valid
            if (inode.hard_links > 0 && inode.deletion_time == 0) {
                printf("Issue: Inode bitmap | Inode %d is valid but not marked as used in the bitmap.\n", i);
                inode_bitmap[i / 8] |= (1 << (i % 8)); // update bitmap to used
                update = 1;
            }
        }
    }
}

void data_bitmap_validator(FILE *fs, uint8_t *data_bitmap, uint32_t first_data_block, uint8_t *block_usage) {
    for (int i = 0; i < INODE_COUNT; i++) {
        Inode inode;
        fseek(fs, first_data_block * BLOCK_SIZE + i * INODE_SIZE, SEEK_SET);
        fread(&inode, sizeof(Inode), 1, fs);

        if (inode.hard_links > 0 && inode.deletion_time == 0) {
            if (inode.direct_block >= first_data_block && inode.direct_block < TOTAL_BLOCKS) {
                block_usage[inode.direct_block]++;
            }
        }
    }
    for (int i = first_data_block; i < TOTAL_BLOCKS; i++) { // Inconsistancy
        if (data_bitmap[i / 8] & (1 << (i % 8))) { // Block used in bitmap kintu no inode
            if (block_usage[i] == 0) { 
                printf("Issue: Data Bitmap | Block %d marked as used in bitmap but not referenced by any inode.\n", i);
                data_bitmap[i / 8] &= ~(1 << (i % 8)); // Unused banay
                update = 1;
            }
        } else { // Block unused in bitmap kintu yes inode
            if (block_usage[i] > 0) {
                printf("Issue: Data Bitmap | Block %d referenced by an inode but not marked as used in bitmap.\n", i);
                data_bitmap[i / 8] |= (1 << (i % 8)); 
                update = 1;
            }
        }
    }
}

void duplicate_blocks(uint8_t *block_usage) {
    for (int i = 0; i < TOTAL_BLOCKS; i++) {
        if (block_usage[i] > 1) { // Same block er jonno multiiple inode
            printf("Issue: Duplicate Block | Block %d is referenced by multiple inodes.\n", i);
            block_usage[i] = 1;
            update = 1; 
        }
    }
}

void bad_blocks(uint8_t *block_usage) {
    for (int i = 0; i < TOTAL_BLOCKS; i++) {
        if (block_usage[i] > 0 && (i < 8 || i >= TOTAL_BLOCKS)) { // Data block range er bahire
            printf("Issue: Bad Block | Block %d is outside the valid range.\n", i);
            block_usage[i] = 0; 
            update = 1; 
        }
    }
}