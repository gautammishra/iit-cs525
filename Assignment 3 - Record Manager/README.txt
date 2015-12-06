RUNNING THE SCRIPT
=======================================

1) Go to Project root (assign3) using Terminal.

2) Type ls to list the files and check that we are in the correct directory.

3) Type "make clean" to delete old compiled .o files.

4) Type "make" to compile all project files including "test_assign3_1.c" file 

5) Type "make run" to run "test_assign3_1.c" file.

6) Type "make test_expr" to compile test expression related files including "test_expr.c".

7) Type "make run_expr" to run "test_expr.c" file.


SOLUTION DESCRIPTION
=======================================

MakeFile was made using following tutorial -
http://mrbook.org/blog/tutorials/make/

We have ensured proper memory management while making this record manager by freeing any reserved space wherever possible and minimizing the use of variables as much as possible. Along with the functions declared in record_mgr.h, we have also mplemented the Tombstone mechanism.

1. TABLE AND RECORD MANAGER FUNCTIONS
=======================================

The record manager related functions are used to initialize and shutdown the record manager. The table related functions are used to create, open, close and delete a table. We make the use of Buffer Manager (Assignment 2) to access pages via Buffer Pool using a page replacement policy. Also, the Storage Manager (Assignment 1) is used indirectly to perform operations on page file on disk.

initRecordManager (...)
--> This function initializes the record manager.
--> We call initStorageManager(...) function of Storage Manager to initialize the storage manager. 

shutdownRecordManager(...)
--> This function shutsdown the record manager and de-allocates all the resources allocated to the record manager.
--> It free up all resources/memory space being used by the Record Manager.
--> We set the recordManager data structure pointer to NULL and call C function free() to de-allocate memory space

createTable(...)
--> This function opens the table having name specified by the paramater 'name.
--> It initializes the Buffer Pool by calling initBufferPool(...). We use LRU page replacment policy.
--> It initializes all the values of the table and also sets the attributes (name, datatype and size) of the table.
--> It then creates a page file, opens that page file, writes the block containing the table in the page file and closes the page file.

openTable(...)
--> This function creates a table with name as specified in the parameter 'name' in the schema specified in the parameter 'schema'.
--> It 

closeTable(...)
--> This function closes the table as pointed by the parameter 'rel'.
--> It does so by calling BUffer Manager's function shutdownBufferPool(...).
--> Before shutting the buffer pool, the buffer manager writes the changes made to the table in the page file.

deleteTable(...)
--> This function deletes the table with name specified by the parameter 'name'.
--> It calls the Storage Manager's function destroyPageFile(...).
--> destroyPageFile(...) function deletes the page from disk and de-allocates ane memory space allocated for that mechanism.

getNumTuples(...)
--> This function returns the number of tuples in the table referenced by parameter 'rel'.
--> It returns the value of the variable [tuplesCount] which is defined in our custom data structure which we use for storing table's meta-data.


2. RECORD FUNCTIONS
=======================================

These functions are used to retrieve a record with a certain RID, to delete a record with a certain RID, to insert a new record, and to update an existing record with new values.

insertRecord(...)
--> This function inserts a record in the table and updates the 'record' parameter with the Record ID passed in the insertRecord() function.
--> We set the Record ID for the record being inserted.
--> We pin the page which has an empty slot. Once we get an empty slot, we locate the data pointer and add a '+' to denote that this is a newly added record.
--> Also we mark the page dirty so that the Buffer Manager writes the content the page back to the disk.
--> We copy the record's data (passed through parameter 'record') into the new record using memcpy() C function and then unpin the page.

deleteRecord(...)
--> This function deletes a record having Record ID 'id' passed through the parameter from the table referenced by the parameter 'rel'.
--> We set our table's meta-data freePage to the Page ID of this page whose record is to be deleted so that this space can be used by a new record later.
--> We pin the page and navigate to the data pointer of the record and set the first character to '-' which denotes that this record is deleted and no longer needed.
--> Finally, we mark the page dirty so that the BUffer Manager can save the contents of the page back to disk and then we unpin the page.

updateRecord(...)
--> This function updates a record referenced by the parameter "record" in the table referenced by the parameter "rel".
--> It finds the page where the record is located by table's meta-data and pins that page in the buffer pool.
--> It sets the Record ID, navigates to the location where the record's data is stored.
--> We copy the record's data (passed through parameter 'record') into the new record using memcpy() C function, mark the page dirty and then unpin the page.

getRecord(....)
--> This function retrieves a record having Record ID "id" passed in the paramater in the table referenced by "rel" which is also passed in the parameter. The result record is stored in the location referenced by the parameter "record".
--> It finds the page where the record is located by table's meta-data and using the 'id' of the record, it pins that page in the buffer pool.
--> It sets the Record ID of the 'record' parameter with the id of the record which exists in the page and copies the data too.
--> It then unpins the page.


3. SCAN FUNCTIONS
=======================================

The Scan related functions are used to retreieve all tuples from a table that fulfill a certain condition (represented as an Expr). Starting a scan initializes the RM_ScanHandle data structure passed as an argument to startScan. Afterwards, calls to the next method is made which returns the next tuple that fulfills the scan condition. If NULL is passed as a scan condition, it returns RC_SCAN_CONDITION_NOT_FOUND. next returns RC_RM_NO_MORE_TUPLES once the scan is completed and RC_OK otherwise (unless an error occurs).

startScan(...)
--> This function starts a scan by getting data from the RM_ScanHandle data structure which is passed as an argument to startScan() function.
--> We initialize our custom data structure's scan related variables.
--> If condition iS NULL, we return error code RC_SCAN_CONDITION_NOT_FOUND

next(...)
--> This function returns the next tuple which satisfies the condition (test expression).
--> If condition iS NULL, we return error code RC_SCAN_CONDITION_NOT_FOUND
--> If there are no tuples in the table, we return error code RC_RM_NO_MORE_TUPLES
--> We iterate through the tuples in the table. Pin the page having that tuple, navigate to the location where data is stored, copy data into a temporary buffer and then evaluate the test expression by calling eval(....)
--> If the result (v.boolV) of the test expression is TRUE, it means the tuple fulfills the condition. We then unpin the page and return RC_OK
--> If none of the tuples fulfill the condition, then we return error code RC_RM_NO_MORE_TUPLES

closeScan(...) 
--> his function closes the scan operation.
--> We check if the scan was incomplete by checking the scanCount value of the table's metadata. If it is greater than 0, it means the scan was incomplete.
--> If the scan was incomplete, we unpin the page and reset all scan mechanism related variables in our table's meta-data (custom data structure).
--> We then free (de-allocate) the space occupied by the metadata.


4. SCHEMA FUNCTIONS
=========================================

These functions are used to return the size in bytes of records for a given schema and create a new schema. 

getRecordSize(...)
--> This function returns the size of a record in the specified schema.
--> We iterate through the attributes of the schema. We iteratively add the size (space in bytes) required by each attribute to the variable 'size'. 
--> The value of the variable 'size' is the size of the record.

freeSchema(...)
--> This function removes the schema specified by the parameter 'schema' from the memory.
--> The variable (field) refNum in each page frame serves this purpose. refNum keeps a count of of the page frames being accessed by the client.
--> We use the C function free(...) to de-allocate the memory space occupied by the schema, thereby removing it from the memory.

createSchema(...)
--> This function create a new schema with the specified parameters in memory.
--> numAttr specifies the number of parameters. attrNames specifies the name of the attributes. datatypes specifies the datatype of the attributes. typeLength specifies the length of the attribute (example: length of STRING).
--> We create a schema object and allocate memory space to the object. We finally set te schema's parameters to the parameters passed in the createSchema(...)


5. ATTRIBUTE FUNCTIONS
=========================================

These functions are used to get or set the attribute values of a record and create a new record for a given schema. Creating a new record should allocate enough memory to the data field to hold the binary representations for all attributes of this record as determined by the schema.  

createRecord(...)
--> This function creates a new record in the schema passed by parameter 'schema' and passes the new record to the 'record' paramater in the createRecord() function.
--> We allocate proper memory space to the new record. Also we give memory space for the data of the record which is the record size.
--> Also, we add a '-' to the first position and append '\0' which NULL in C. '-' denotes that this is a new blank record.
--> Finally, we assign this new record to the 'record' passed through the parameter.

attrOffset(...)
--> This function sets the offset (in bytes) from initial position to the specified attribute of the record into the 'result' parameter passed through the function.
--> We iterate through the attributes of the schema till the specified attribute number. We iteratively add the size (space in bytes) required by each attribute to the pointer *result. 

freeRecord(...)
--> This function de-allocates the memory space allocated to the 'record' passed through the parameter.
--> We use the C function free() to de-allocate the memory space (free up space) used by the record.

getAttr(...)
--> This function retrieves an attribute from the given record in the specified schema.
--> The record, schema and attribute number whose data is to be retrieved is passed through the parameter. The attribute details are stored back to the location referenced by 'value' passed through the parameter.
--> We go to the location of the attribute using the attrOffset(...) function. We then depending on the datatype of the attribute, copy the attribute's datatype and value to the '*value' parameter.

setAttr(...)
--> This function sets the attribute value in the record in the specified schema. The record, schema and attribute number whose data is to be retrieved is passed through the parameter.
--> The data which are to be stored in the attribute is passed by 'value' parameter.
--> We go to the location of the attribute using the attrOffset(...) function. We then depending on the datatype of the attribute, copy the data in the '*value' parameter to the attributes datatype and value both.