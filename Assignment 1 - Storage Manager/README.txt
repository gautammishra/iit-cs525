RUNNING THE SCRIPT
=========================

1) Go to Project root (assign1) using Terminal.

2) Type ls to list the files and check that we are in the correct directory.

3) Type "make clean" to delete old compiled .o files.

4) Type "make" to compile all project files including "test_assign1_1.c" file 

5) Type "make run_test1" to run "test_assign1_1.c" file.

6) Type "make test2" to compile Custom test file "test_assign1_2.c".

7) Type "make run_test2" to run "test_assign1_2.c" file.


SOLUTION DESCRIPTION
===========================

MakeFile was made using following tutorial -
http://mrbook.org/blog/tutorials/make/

We have ensured proper memory management while making this storage manager by closing the file stream whenever required and freeing any reserved space.

1. FILE RELATED FUNCTIONS
===========================

The file related functions manage files i.e. creat,open and close file.
Once the file is created, we can use read and write methods for storage manager.

initStorageManager()
--> This function initializes the storage manager. We reference the file stream object to NULL in this method.

createPageFile(...)
--> This function creates a page file having file name as specified in the parameter.
--> We use fopen() C function to create a file. We use 'w+' mode which creates a new file and opens it for reading and writing.
--> We return RC_FILE_NOT_FOUND if file could not be created and RC_OK if everything goes well.

openPageFile(...)
--> We use the fopen() C function to open the file and 'r' mode to open the file in read only mode.
--> Also, we initialise struct FileHandle's curPagePos and totalPageNum.
--> We use the linux fstat() C function which gives various information regarding the file. We retrieve the size of file in bats using fstat() function.
--> We return RC_FILE_NOT_FOUND if file could not be open and RC_OK if everything goes well.

closePageFile(...)
--> We set the page file pointer to NULL.

destroyPageFile:
--> We check if the file is present in memory and if present, we use the remove() C function to remove it from memory.


2. READ RELATED FUNCTIONS
==========================

The read related functions are used to read blocks of data from the page file into the disk (memory). Also it navigates through the blocks easily.
C functions - fseek(..) and fread(..) are used.

readBlock(...)
--> We check whether the page number is valid or not. Page number should be greater than 0 and less than total number of pages.
--> Check if the pointer to the page file is available.
--> Using the valid file pointer we navigate to the given location using fseek()
--> If fseek() is successful, we read the data from the page number spcified in the paramter and store into the memPage passed in the paramter.

getBlockPos(...)
--> This function returns the current page position which is retrieved from FileHandle's curPagePos.

readFirstBlock(...)
--> We call the readBlock(...) function by providing the pageNum argument as 0

readPreviousBlock(....)
--> We call the readBlock(...) function by providing the pageNum argument as (current page position - 1)

readCurrentBlock:
--> We call the readBlock(...) function by providing the pageNum argument as (current page position)

readNextBlock:
--> We call the readBlock(...) function by providing the pageNum argument as (current page position + 1)

readLastBlock:
--> We call the readBlock(...) function by providing the pageNum argument as (total number of pages - 1)


3. WRITE RELATED FUNCTIONS
===========================

The write related functions are used to write blocks of data from the disk (memory) to the page file.
C functions - fseek(..) and fwrite(..) are used.

writeBlock(...)
--> We check whether the page number is valid or not. Page number should be greater than 0 and less than total number of pages.
--> Check if the pointer to the page file is available.
--> Using the valid file pointer we navigate to the given location using fseek()
--> If fseek() is successful, we write the data to the appropriate location using fwrite() C function and store into the memPage passed in the paramter.

writeCurrentBlock(...)
--> We call writeBlock(...) function with pageNum = current page position as the paramter.

appendEmptyBlock(...) 
--> We create an empty block the having size = PAGE_SIZE
--> We move the cursor(pointer)  of the file stream to the last page.
--> Write the empty block data and update the total number of pages by1 since we just added a new page

ensureCapacity(...)
--> Check number of pages required is greater than the total number of pages i.e. we require more pages.
--> Calculate number of pages required and add that much number of empty blocks.
--> Empty blocks are added using the appendEmptyBlock(...) function

 
TEST CASES 2
===============
We have added additional test cases in source file test_assign_2.c. The instructions to run these test cases is mentioned in this README file.