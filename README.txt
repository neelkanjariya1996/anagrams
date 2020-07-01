To compile the program:
  gcc anagrams.c -pthread -o anagrams

To run the program:
  ./anagrams file1_name file2_name

The basic idea of the algorithm is to keep a count of each character which 
appears in the file. We will iterate through both the files, for first file
we increment the count and for the second file we decrement the count for each
character. This will help in checking if the files are anagrams, i.e. the count
for each individual characters is same in both the files. 

we first open both the files and map it in memory through mmap. To make the
program efficient, multiple threads are created. Each thread will access the
file in a block by block fashion. For example, if the file is 20 blocks long 
and number of threads is 4 then each thread will process the following blocks 
of the files:

Thread_1: 1, 5, 9, 13, 17
Thread_2: 2, 6, 10, 14, 18
Thread_3: 3, 7, 11, 15, 19
Thread_4: 4, 8, 12, 16, 20

Each thread has a count array of its own. So, for each block processed by the
thread, its count array is incremented for file_1 characters and decremented 
for file_2 character. In the end, when the thread has completed the processing 
of all the blocks assigned to it, it adds the thread count array to the main
count array. Access to the main count array is protected by a mutex, as it
is shared by all the threads. 

After every thread has added its count array to the main count array, 
we check whether the elements of main count array are 0 or not. If all the
elements are 0, then the files are anagrams or else not.
