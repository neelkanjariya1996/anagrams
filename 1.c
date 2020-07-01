#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>

#define MAX 128               // max ascii char
#define THREAD_COUNT 4        // num of threads
#define BLOCK_SIZE 1024       // block size
#define SUCCESS 0

pthread_mutex_t count_mutex;  // mutex to access the main count array
int64_t count[MAX];           // main count array
char *ptr1 = NULL;            // pointer to mmap for file_1
char *ptr2 = NULL;            // pointer to mmap for file_2
size_t s1_sz;                 // size of file_1
size_t s2_sz;                 // size of file_2

/*
 * thread function to read the file blocks
 */
void*
thread_read_file_block (void *thread_num)
{

  long t_no = 0;              // thread number
  t_no = (long) thread_num;
  unsigned int i = 0;
  int64_t thread_count[MAX];  // thread count array
  int j = 0;

  memset(thread_count, 0, MAX * sizeof(int64_t));

  /*
   * The outer loop iterates over the block which this 
   * threas needs to access.
   * The inner thread iterates character by character 
   * of the block to increment/decrement the 
   * thread_count array
   */
  for(i = (unsigned int)(t_no) * BLOCK_SIZE; 
      i < s1_sz; i += ((THREAD_COUNT) * BLOCK_SIZE)) {

    for(j = i; j < (i + BLOCK_SIZE) && j < s1_sz; j++) {
      
      thread_count[ptr1[j]]++;
      thread_count[ptr2[j]]--;
    }
  }

  /*
   * acquire the mutex to access the
   * main count array
   */
  pthread_mutex_lock(&count_mutex);

  for(i = 0; i < MAX; i++)
    count[i] += thread_count[i];

  /*
   * release the mutex so other threads 
   * can access it
   */
  pthread_mutex_unlock(&count_mutex);

  pthread_exit((void *)0);
}

/*
 * The function checks whether the count array is zero or not
 * if any count is not zero, than it means the files are not
 * anagrams
 */
bool
is_anagram (void)
{

  for(int i = 0; i < MAX; i++) {
  
    if(count[i] != 0) {
    
      return false;
    }
  }

  return true;
}

int
main (int argc, char *argv[])
{

  long t = 0;
  int rc = 0;
  void *status = NULL;

  pthread_t threads[THREAD_COUNT];
  pthread_attr_t attr;
  
  int fd1 = 0;
  int fd2 = 0;
  struct stat s1;
  struct stat s2;
 
  /*
   * open the files
   */

  fd1 = open(argv[1], O_RDONLY);
  if(fd1 < 0) {
    
    printf("Could not open file: %s\n", argv[1]);
    return -1;
  }

  fd2 = open(argv[2], O_RDONLY);
  if(fd2 < 0) {
    
    printf("Could not open file: %s\n", argv[2]);
    return -1;
  }
 
  /*
   * get the file size
   */
  rc = fstat(fd1, &s1);
  if(rc < 0) {
    
    printf("Failed to get status for %s\n", argv[1]);
    return -1;
  }
  s1_sz = s1.st_size;

  rc = fstat(fd2, &s2);
  if(rc < 0) {
    
    printf("Failed to get status for %s\n", argv[2]);
    return -1;
  }
  s2_sz = s2.st_size;

  if(s1_sz != s2_sz) {

    printf("Files are not anagrams\n");
    return 0;
  }

  /*
   * map both the files in memory
   */
  ptr1 = mmap (0, s1_sz, PROT_READ, MAP_PRIVATE, fd1, 0);
  if(ptr1 == MAP_FAILED) {
    
    printf("Could not map file %s\n", argv[1]);
    return -1;
  }

  ptr2 = mmap (0, s2_sz, PROT_READ, MAP_PRIVATE, fd2, 0);
  if(ptr2 == MAP_FAILED) {
    
    printf("Could not map file %s\n", argv[2]);
    return -1;
  }

  /* 
   * initialize count array 
   */
  memset(count, 0, MAX * sizeof(int64_t));

  /*
   * initialize the attribute
   */
  rc = pthread_attr_init(&attr);
  if(rc != SUCCESS) {

    printf("ERR: initializing attr with err_no: %d\n", rc);
    return -1;
  }

  /*
   * set the attribute joinable
   */
  rc = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  if(rc != SUCCESS) {

    printf("ERR: setting attribute to joinable with err_no: %d\n", rc);
    return -1;
  }

  /*
   * create the threads
   */
  for(t = 0; t < THREAD_COUNT; t++) {

    rc = pthread_create(&threads[t], &attr,
                        thread_read_file_block, (void *)t);
    if (rc != SUCCESS) {

      printf("ERR: creating thread: %ld with err_no: %d\n", t, rc);
      return -1;
    }
  }

  /*
   * destroy the attribute
   */
  rc = pthread_attr_destroy(&attr);
  if (rc != SUCCESS) {

    printf("ERR: destroying attribute with err_no: %d\n", rc);
    return -1;
  }

  /*
   * join the threads
   */
  for (t = 0; t < THREAD_COUNT; t++) {

    rc = pthread_join(threads[t], &status);
    if (rc != SUCCESS) {

      printf("ERR: joining reducer thread: %ld to main with err_no: %d\n", t, rc);
      return -1;
    }
  }

  /*
   * close the files
   */
  if(close(fd1) < 0) {
  
    printf("Could not close file: %s\n", argv[1]);
    return -1;
  }

  if(close(fd2) < 0) {
  
    printf("Could not close file: %s\n", argv[2]);
    return -1;
  }

  /*
   * unmap the files from memory
   */

  munmap(ptr1, s1_sz);
  munmap(ptr2, s2_sz);

  /*
   * check whether files are anagrams
   */
  if(is_anagram()) {
  
    printf("Files are anagrams\n");
  } else {
  
    printf("Files are not anagrams\n");
  }

  pthread_exit(NULL);
  return 0;
}
