// Alexander Peacock
// EEL 4768 Computer Architecture

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// Constants for the cache configuration
#define BLOCK_SIZE 64 // 64 bytes as block size

// struct to hold settings defined by user input
struct settings {
  int cacheSize;
  int assoc;
  int repl; // 0 for LRU, 1 for FIFO
  int wb;   // 1 for write-back, 0 for write-through
  char *trace_file;
} settings_t;

// Variables to maintain the simulation statistics
int Hit = 0;
int Miss = 0;
int writes = 0;
int reads = 0;

// Forward declarations for LRU and FIFO version
void Update_lru(int set_num, uint64_t tag, struct settings set,
                uint64_t **tag_array, int index, char op, bool **dirty);
void Update_fifo(int set_num, uint64_t tag, struct settings set,
                 uint64_t **tag_array, int index, char op, bool **dirty);

// Function to simulate cache access
void Simulate_access(char op, uint64_t add, struct settings set, int num_sets,
                     uint64_t **tag_array, bool **dirty) {
  int set_num = (add / BLOCK_SIZE) % num_sets;
  uint64_t tag = add / BLOCK_SIZE;
  bool check = false; // check if found address

  // if write through, increment memory write with instruction
  if (!set.wb) {
    if (op == 'W') {
      writes++;
    }
  }

  // goes through associativity
  for (int i = 0; i < set.assoc; i++) {
    if (tag == tag_array[set_num][i] && check != true) {
      check = true;
      // Choose policy ( LRU or FIFO ) based on the configuration
      if (!set.repl) {
        Update_lru(set_num, tag, set, tag_array, i, op, dirty);
      } else {
        Update_fifo(set_num, tag, set, tag_array, i, op, dirty);
      }
    }
  }

  // if not found
  if (!check) {

    // Cache miss scenario
    Miss++;

    reads++; // increment read from memorys

    // if write back logic increment memory writes when dirty pushed out
    if (set.wb) {
      if (dirty[set_num][set.assoc - 1] == true) {
        writes++;
      }
    }

    // if assoc is greater than 1
    if (set.assoc > 1) {
      // shift blocks of a set to account for new item
      for (int i = set.assoc - 2; i >= 0; i--) {
        tag_array[set_num][i + 1] = tag_array[set_num][i];
        // update dirty bit array if using write-back
        if (set.wb) {
          dirty[set_num][i + 1] = dirty[set_num][i];
        }
      }
    }

    tag_array[set_num][0] = tag; // adds tag to the first block in the cache

    // if write-back, update first block depending on if read or write
    if (set.wb) {
      // block would be modified so dirty == true
      if (op == 'W') {
        dirty[set_num][0] = true;
      }
      // block would not be modified so dirty == false
      else if (op == 'R') {
        dirty[set_num][0] = false;
      }
    }
  }
}

// Update functions for different policies
// least recently used implementation, blocks move to top of stack from usage
void Update_lru(int set_num, uint64_t tag, struct settings set,
                uint64_t **tag_array, int index, char op, bool **dirty) {
  // Logic for updating LRU policy
  // if write-back, implement that otherwise do write through
  if (set.wb) {

    bool tempDirty =
        dirty[set_num]
             [index]; // stores current dirty value to be modified later

    Hit++;

    // if write a clean bit, add 1 to Hit and set true to dirty
    if (op == 'W') {
      tempDirty = true;
    }

    /*
        Since tag was found, move everything less recently used
        1 step to the right.
    */
    for (int i = index - 1; i >= 0; i--) {
      tag_array[set_num][i + 1] = tag_array[set_num][i];
      dirty[set_num][i + 1] =
          dirty[set_num][i]; // so can match with dirty array
    }

    // set MRU part to be the tag we just got
    tag_array[set_num][0] = tag;
    dirty[set_num][0] = tempDirty;

  } else {
    Hit++; // increase number of hits

    /*
        Since tag was found, move everything less recently used
        1 step to the right.
    */
    for (int i = index - 1; i >= 0; i--) {
      tag_array[set_num][i + 1] = tag_array[set_num][i];
    }

    // set MRU part to be the tag we just got
    tag_array[set_num][0] = tag;
  }
}

// first in first out implementation, blocks don't move up from usage
void Update_fifo(int set_num, uint64_t tag, struct settings set,
                 uint64_t **tag_array, int index, char op, bool **dirty) {
  // Logic for updating FIFO policy
  // if write-back, implement that otherwise do write through
  if (set.wb) {

    Hit++;

    // if write a clean bit, add 1 to Hit and set true to dirty
    if (op == 'W') {

      dirty[set_num][index] = true;
    }

  } else {
    Hit++; // increase number of hits

    /*
        Since tag was found, nothing needs to move
    */
  }
}

// main code
int main(char argc, char *argv[]) {

  // sets settings according to user input
  struct settings cache_settings = {
      .cacheSize = atoi(argv[1]), // size of cache
      .assoc = atoi(argv[2]),     // associativity
      .repl = atoi(argv[3]),      // 0 for LRU, 1 for FIFO
      .wb = atoi(argv[4]),        // 0 for write-through, 1 for write-back
      .trace_file = argv[5]       // name of trace file
  };

  // sets number of sets
  int num_sets = cache_settings.cacheSize / (BLOCK_SIZE * cache_settings.assoc);

  // allocating array to hold the tags;
  uint64_t **tag_array = (uint64_t **)malloc(sizeof(uint64_t *) * num_sets);
  for (int i = 0; i < num_sets; i++) {
    tag_array[i] = malloc(sizeof(uint64_t) * cache_settings.assoc);
  }

  // array to hold dirty bits for tags;
  bool **dirty = malloc(sizeof(bool *) * num_sets);
  for (int i = 0; i < num_sets; i++) {
    dirty[i] = malloc(sizeof(bool) * cache_settings.assoc);
  }

  // sets intial arrays to null
  for (int i = 0; i < num_sets; i++) {
    for (int j = 0; j < cache_settings.assoc; j++) {
      tag_array[i][j] = -1;
    }
  }

  // sets intial arrays to false
  for (int i = 0; i < num_sets; i++) {
    for (int j = 0; j < cache_settings.assoc; j++) {
      dirty[i][j] = false;
    }
  }

  // opens the file
  char op;
  uint64_t add;
  FILE *file = fopen(cache_settings.trace_file, "r");

  // Check if the file opened successfully
  if (!file) {
    printf("Error : Could not open the trace file.\n");
    return 1;
  }

  // Read until end of file
  while (!feof(file)) {

    // Read operation and address
    fscanf(file, "%c %llx ", &op, &add);

    // Begin the simulation for each address read
    Simulate_access(op, add, cache_settings, num_sets, tag_array, dirty);
  }

  // Print out the statistics
  printf("Miss Ratio: %lf\n", (double)Miss / (double)(Hit + Miss));
  printf("Writes : %d\n", writes);
  printf("Reads : %d\n", reads);

  // frees pointers
  for (int i = 0; i < num_sets; i++) {
    free(dirty[i]);
  }

  for (int i = 0; i < num_sets; i++) {
    free(tag_array[i]);
  }

  free(tag_array);
  free(dirty);

  return 0;
}