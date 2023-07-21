#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "mdadm.h"
#include "jbod.h"
#include "cache.h"

// Global variables
bool MOUNTED = false;

// Mounts the file system
int mdadm_mount(void)
{
  if (MOUNTED)
  {
    return -1; // Already mounted, return error code
  }

  jbod_operation(JBOD_MOUNT, NULL); // Mount the file system using JBOD operation
  MOUNTED = true;                   // Set mounted flag to true
  return 1;                         // Return success code
}

// Unmounts the file system
int mdadm_unmount(void)
{
  if (!MOUNTED)
  {
    return -1; // Not mounted, return error code
  }

  jbod_operation(JBOD_UNMOUNT, NULL); // Unmount the file system using JBOD operation
  MOUNTED = false;                    // Set mounted flag to false
  return 1;                           // Return success code
}

// Helper function to construct the JBOD operation command
uint32_t op(uint32_t diskID, uint32_t blockID, uint32_t command)
{
  return ((diskID << 28) | (blockID << 20) | (command << 14));
}

// Reads data from the file system
int mdadm_read(uint32_t start_addr, uint32_t read_len, uint8_t *read_buf)
{
  int MAX_READ_LEN = 1024;
  int MAX_BOUNDS = JBOD_NUM_DISKS * JBOD_DISK_SIZE;

  // Check if the file system is mounted, read length is within bounds, and read buffer is valid
  if (!MOUNTED || (read_len != 0 && (read_len > MAX_READ_LEN || read_buf == NULL)) || start_addr + read_len > MAX_BOUNDS)
    return -1; // Return error code

  int current_address = start_addr;
  int current_disk = start_addr / 65536;
  int current_block = (start_addr % 65536) / 256;
  int offset = current_address % 256;
  int read_bytes = 0;
  uint8_t temp_buf[256];
  int remaining_bytes = 0;

  if (cache_enabled())
  {
    uint8_t temp_buf[JBOD_BLOCK_SIZE];

    for (uint32_t i = 0; i < read_len; i += JBOD_BLOCK_SIZE)
    {
      // Calculate the disk and block number for the current read
      current_disk = current_address / 65536;
      current_block = (current_address % 65536) / 256;

      // Look up the block in the cache
      if (cache_lookup(current_disk, current_block, temp_buf) == -1)
      {
        // Cache miss: Read the block from the disk
        int jbod_disk = op(current_disk, 0, JBOD_SEEK_TO_DISK);
        jbod_operation(jbod_disk, NULL);
        int jbod_block = op(0, current_block, JBOD_SEEK_TO_BLOCK);
        jbod_operation(jbod_block, NULL);
        int jbod_read = op(0, 0, JBOD_READ_BLOCK);
        jbod_operation(jbod_read, temp_buf);

        // Insert the block into the cache
        cache_insert(current_disk, current_block, temp_buf);
      }

      // Copy data from temp_buf to read_buf
      #define min(a, b) (((a) < (b)) ? (a) : (b))
      int bytes_to_copy = min(read_len - i, JBOD_BLOCK_SIZE);
      memcpy(read_buf + i, temp_buf, bytes_to_copy);
      }
      return read_len;
    
      } else {

    while (read_bytes < read_len)
    {
      // Seek to the current disk and block
      int jbod_disk = op(current_disk, 0, JBOD_SEEK_TO_DISK);
      jbod_operation(jbod_disk, NULL);
      int jbod_block = op(0, current_block, JBOD_SEEK_TO_BLOCK);
      jbod_operation(jbod_block, NULL);
      int jbod_read = op(0, 0, JBOD_READ_BLOCK);
      jbod_operation(jbod_read, temp_buf);

      remaining_bytes = read_len - read_bytes;
      if (offset + remaining_bytes < 256)
      {
        memcpy(read_buf + read_bytes, temp_buf + offset, remaining_bytes);
        read_bytes += remaining_bytes;
      }
      else
      {
        memcpy(read_buf + read_bytes, temp_buf + offset, 256 - offset);
        read_bytes += 256 - offset;
      }

      current_address += read_bytes;
      current_disk = current_address / 65536;
      current_block = (current_address % 65536) / 256;

      int update_disk = op(current_disk, 0, JBOD_SEEK_TO_DISK);
      jbod_operation(update_disk, NULL);
      int update_block = op(0, current_block, JBOD_SEEK_TO_BLOCK);
      jbod_operation(update_block, NULL);
      offset = 0;
    }

    return read_bytes;
  }
}

int mdadm_write(uint32_t start_addr, uint32_t write_len, const uint8_t *write_buf) {
  // Check mount status and validity of start_addr and write_len
  if (!MOUNTED || start_addr + write_len > 1048576 || write_len > 1024)
    return -1;

  // Check for special cases where write_len is 0 and write_buf is NULL
  if (write_len == 0 && write_buf == NULL)
    return 0;

  // Check if write_len is non-zero but write_buf is NULL
  if (write_len != 0 && write_buf == NULL)
    return -1;

  int written_bytes = 0;
  uint8_t temp_buf[256];

  while (written_bytes < write_len) {
    int current_address = start_addr + written_bytes;
    int current_disk = current_address / 65536;
    int current_block = (current_address % 65536) / 256;
    int offset = current_address % 256;

    // Seek to the appropriate disk
    int jbod_disk = op(current_disk, 0, JBOD_SEEK_TO_DISK);
    jbod_operation(jbod_disk, NULL);

    // Seek to the appropriate block
    int jbod_block = op(0, current_block, JBOD_SEEK_TO_BLOCK);
    jbod_operation(jbod_block, NULL);

    int remaining_bytes = write_len - written_bytes;
    int bytes_to_write = (offset != 0) ? (256 - offset) : ((remaining_bytes < 256) ? remaining_bytes : 256);

    // Copy data to temp_buf
    memcpy(temp_buf + offset, write_buf + written_bytes, bytes_to_write);

    // Write the block using jbod_operation
    int write_the_block = op(0, 0, JBOD_WRITE_BLOCK);
    jbod_operation(write_the_block, temp_buf);

    written_bytes += bytes_to_write;
  }

  return write_len;
}