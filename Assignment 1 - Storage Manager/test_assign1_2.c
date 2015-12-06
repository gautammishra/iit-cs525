#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "storage_mgr.h"
#include "dberror.h"
#include "test_helper.h"

// test name
char *testName;

/* test output files */
#define TESTPF "test_pagefile.bin"

/* prototypes for test functions */
static void testAdditionalCases(void);

/* main function running all tests */
int main (void)
{
	testName = "";
  
	initStorageManager();
	testAdditionalCases();

	return 0;
}

/* Try to create, open, and close a page file */
void testAdditionalCases(void)
{
	SM_FileHandle fh;
	SM_PageHandle ph;
	int i;

	testName = "test single page content";

	ph = (SM_PageHandle) malloc(PAGE_SIZE);

	// create a new page file
	TEST_CHECK(createPageFile (TESTPF));
	TEST_CHECK(openPageFile (TESTPF, &fh));
	printf("created and opened file\n");
  
	// read first page into handle
	TEST_CHECK(readFirstBlock (&fh, ph));
	// the page should be empty (zero bytes)
  
	for (i=0; i < PAGE_SIZE; i++)		
		ASSERT_TRUE((ph[i] == 0), "expected zero byte in first page of freshly initialized page");

	printf("first block was empty\n");

	// change ph to be a string and write that one to disk
	for (i=0; i < PAGE_SIZE; i++)
		ph[i] = (i % 10) + '0';
	
	// Writing string ph to first page of file.
	TEST_CHECK(writeBlock(0, &fh, ph));
	printf("writing first block successful \n");
  
  	// Writing string ph to current position of file.
  	TEST_CHECK(writeCurrentBlock(&fh, ph));
  	printf("write current block \n");
  
  	// Writing string ph to first page of file.
  	TEST_CHECK(writeBlock(1, &fh, ph));
  	printf("write in second block \n");

 	// Writing string ph to current position of file.
  	TEST_CHECK(writeCurrentBlock(&fh, ph));
  	printf("write current block \n");

	// Writing string ph to fourth page of file.
	TEST_CHECK(writeBlock(3, &fh, ph))
  	printf("write in fourth block \n");


  	// Reading the content of the page containing the string and checking whether it is correct
  	TEST_CHECK(readFirstBlock (&fh, ph));
  	for (i=0; i < PAGE_SIZE; i++)
    		ASSERT_TRUE((ph[i] == (i % 10) + '0'), "character in page read from disk is the one we expected.");
  	printf("reading first block \n");

  	// Reading the previous block from file.
  	TEST_CHECK(readPreviousBlock (&fh, ph));
  	for (i=0; i < PAGE_SIZE; i++)
    		ASSERT_TRUE((ph[i] == (i % 10) + '0'), "character in page read from disk is the one we expected.");
  	printf("reading previous block\n");

	// Reading the next block from file.
  	TEST_CHECK(readNextBlock (&fh, ph));
  	for (i=0; i < PAGE_SIZE; i++)
    		ASSERT_TRUE((ph[i] == (i % 10) + '0'), "character in page read from disk is the one we expected.");
  	printf("reading Next block\n");
  
  	// Reading the current block from file.
  	TEST_CHECK(readCurrentBlock (&fh, ph));
  	for (i=0; i < PAGE_SIZE; i++)
    		ASSERT_TRUE((ph[i] == (i % 10) + '0'), "character in page read from disk is the one we expected.");
  	printf("reading Current block\n");
  
  	// Reading the specific block (2nd block in this case) from file.	
  	TEST_CHECK(readBlock(2,&fh, ph));
  	for (i=0; i < PAGE_SIZE; i++)
    		ASSERT_TRUE((ph[i] == (i % 10) + '0'), "character in page read from disk is the one we expected.");

  	// Reading the last block from file.
  	TEST_CHECK(readLastBlock (&fh, ph));
  	for (i=0; i < PAGE_SIZE; i++)
    		ASSERT_TRUE((ph[i] == (i % 10) + '0'), "character in page read from disk is the one we expected.");
 
  	// Testing ensureCapacity function.
  	TEST_CHECK(ensureCapacity(6,&fh));
  
  	// Destroy new page file
  	TEST_CHECK(destroyPageFile (TESTPF));  
 
  	TEST_DONE();
}
