#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "cache.h"

static cache_entry_t *cache = NULL; // Pointer to the cache entries
static int cache_size = 0;          // Number of cache entries in the cache
static int num_queries = 0;         // Total number of cache queries made
static int num_hits = 0;            // Total number of cache hits (successful cache queries)

int cacheEnabled = 0; // Flag to indicate if the cache is enabled or not
int fifoIndex = 0;    // Index used for implementing FIFO (First-In-First-Out) replacement policy in the cache

// Function to create the cache with a specified number of entries
int cache_create(int num_entries)
{
  // Check for invalid cache size or if the cache is already enabled
  if (num_entries < 2 || num_entries > 4096 || cacheEnabled == 1)
    return -1;

  cache_size = num_entries;
  cacheEnabled = 1;
  cache = calloc(num_entries, sizeof(cache_entry_t)); // Allocate memory for cache entries

  for (int i = 0; i < cache_size; i++)
  {
    cache[i].valid = false; // Initialize all cache entries as invalid (empty)
  }

  return 1;
}

// Function to destroy the cache and free allocated memory
int cache_destroy(void)
{
  if (cache == NULL || cacheEnabled == 0)
    return -1;

  free(cache); // Free the memory used by cache entries
  cache = NULL;

  cache_size = 0;
  fifoIndex = 0;
  num_hits = 0;
  cacheEnabled = 0;
  num_queries = 0;

  return 1;
}

// Function to look up data in the cache
int cache_lookup(int disk_num, int block_num, uint8_t *buf)
{
  num_queries += 1; // Increment the total number of cache queries

  if (cache == NULL || buf == NULL || disk_num < 0 || disk_num > 15 || block_num < 0 || block_num > 255 || cacheEnabled == 0)
    return -1; // Invalid parameters or cache not enabled

  for (int i = 0; i < cache_size; i++)
  {
    // Check if the cache entry matches the requested disk_num and block_num and is valid (contains data)
    if ((cache[i].disk_num == disk_num) && (cache[i].block_num == block_num) && cache[i].valid == true)
    {
      num_hits += 1; // Increment the cache hit count
      memcpy(buf, cache[i].block, JBOD_BLOCK_SIZE); // Copy the data from the cache entry to the provided buffer
      return 1; // Cache hit
    }
  }
  return -1; // Cache miss
}

// Function to update data in the cache
void cache_update(int disk_num, int block_num, const uint8_t *buf)
{
  for (int i = 0; i < cache_size; i++)
  {
    // Find the cache entry matching the requested disk_num and block_num and update its data
    if ((cache[i].disk_num == disk_num) && (cache[i].block_num == block_num))
      memcpy(cache[i].block, buf, JBOD_BLOCK_SIZE);
  }
}

// Function to insert data into the cache
int cache_insert(int disk_num, int block_num, const uint8_t *buf)
{
  if (cache == NULL || disk_num < 0 || disk_num > 15 || buf == NULL || block_num < 0 || block_num > 255 || cacheEnabled == 0)
    return -1; // Invalid parameters or cache not enabled

  for (int i = 0; i < cache_size; i++)
  {
    // Check if the cache entry already contains the requested data (avoid duplicates)
    if ((cache[i].disk_num == disk_num) && (cache[i].block_num == block_num) && (cache[i].valid == true))
      return -1; // Data already exists in the cache
  }

  // Implement FIFO replacement policy by using fifoIndex as the index to replace the next entry
  if (fifoIndex >= cache_size)
    fifoIndex = 0;

  // Insert the new data into the cache entry pointed by fifoIndex
  cache[fifoIndex].valid = true;
  cache[fifoIndex].disk_num = disk_num;
  cache[fifoIndex].block_num = block_num;
  memcpy(cache[fifoIndex].block, buf, JBOD_BLOCK_SIZE);

  fifoIndex += 1;
  return 1; // Data successfully inserted into the cache
}

// Function to check if the cache is enabled
bool cache_enabled(void)
{
  return cacheEnabled == 1;
}

// Function to print the cache hit rate
void cache_print_hit_rate(void)
{
  fprintf(stderr, "Hit rate: %5.1f%%\n", 100 * (float)num_hits / num_queries);
}