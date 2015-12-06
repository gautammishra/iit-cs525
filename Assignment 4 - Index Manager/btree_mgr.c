#include "dberror.h"
#include "btree_mgr.h"
#include "storage_mgr.h"
#include "buffer_mgr.h"
#include "tables.h"
#include "btree_implement.h"

// This structure stores the metadata for our Index Manager
BTreeManager * treeManager = NULL;

// This function initializes our Index Manager.
RC initIndexManager(void *mgmtData) {
	initStorageManager();
	//printf("\n initIndexManager SUCCESS");
	return RC_OK;
}

// This function shutdowns the Index Manager.
RC shutdownIndexManager() {
	treeManager = NULL;
	//printf("\n shutdownIndexManager SUCCESS");
	return RC_OK;
}

// This function creates a new B+ Tree with name "idxId",
// datatype of the key as "keyType" and order specified by "n".
RC createBtree(char *idxId, DataType keyType, int n) {
	int maxNodes = PAGE_SIZE / sizeof(Node);

	// Return error if we cannot accommodate a B++ Tree of that order.
	if (n > maxNodes) {
		printf("\n n = %d > Max. Nodes = %d \n", n, maxNodes);
		return RC_ORDER_TOO_HIGH_FOR_PAGE;
	}

	// Initialize the members of our B+ Tree metadata structure.
	treeManager = (BTreeManager *) malloc(sizeof(BTreeManager));
	treeManager->order = n + 2;		// Setting order of B+ Tree
	treeManager->numNodes = 0;		// No nodes initially.
	treeManager->numEntries = 0;	// No entries initially
	treeManager->root = NULL;		// No root node
	treeManager->queue = NULL;		// No node for printing
	treeManager->keyType = keyType;	// Set datatype to "keyType"

	// Initialize Buffer Manager and store in our structure.
	BM_BufferPool * bm = (BM_BufferPool *) malloc(sizeof(BM_BufferPool));
	treeManager->bufferPool = *bm;

	SM_FileHandle fileHandler;
	RC result;

	char data[PAGE_SIZE];

	// Create page file. Return error code if error occurs.
	if ((result = createPageFile(idxId)) != RC_OK)
		return result;

	// Open page file.  Return error code if error occurs.
	if ((result = openPageFile(idxId, &fileHandler)) != RC_OK)
		return result;

	// Write empty content to page.  Return error code if error occurs.
	if ((result = writeBlock(0, &fileHandler, data)) != RC_OK)
		return result;

	// Close page file.  Return error code if error occurs.
	if ((result = closePageFile(&fileHandler)) != RC_OK)
		return result;

	//printf("\n createBtree SUCCESS");
	return (RC_OK);
}

// This functions opens an existing B+ Tree from the specified page "idxId"
RC openBtree(BTreeHandle **tree, char *idxId) {
	// Retrieve B+ Tree handle and assign our metadata structure
	*tree = (BTreeHandle *) malloc(sizeof(BTreeHandle));
	(*tree)->mgmtData = treeManager;

	// Initialize a Buffer Pool using Buffer Manager
	RC result = initBufferPool(&treeManager->bufferPool, idxId, 1000, RS_FIFO, NULL);

	if (result == RC_OK) {
		//printf("\n openBtree SUCCESS");
		return RC_OK;
	}
	return result;
}

// This function closes the B+ Tree, shutdowns the buffer pool and
// de-allocates all utilized memory space.
RC closeBtree(BTreeHandle *tree) {
	// Retrieve B+ Tree's metadata information.
	BTreeManager * treeManager = (BTreeManager*) tree->mgmtData;

	// Mark page dirty so that it can written back to the disk.
	markDirty(&treeManager->bufferPool, &treeManager->pageHandler);

	// Shutdown the buffer pool.
	shutdownBufferPool(&treeManager->bufferPool);

	// Release memory space.
	free(treeManager);
	free(tree);

	//printf("\n closeBtree SUCCESS");
	return RC_OK;
}

// This method deleted the B+ Tree by deleting the associated page with it.
RC deleteBtree(char *idxId) {
	RC result;
	if ((result = destroyPageFile(idxId)) != RC_OK)
		return result;
	//printf("\n deleteBtree SUCCESS");
	return RC_OK;
}

// This function adds a new entry/record with the specified key and RID.
RC insertKey(BTreeHandle *tree, Value *key, RID rid) {
	// Retrieve B+ Tree's metadata information.
	BTreeManager *treeManager = (BTreeManager *) tree->mgmtData;
	NodeData * pointer;
	Node * leaf;

	int bTreeOrder = treeManager->order;

	//printf("\n INSERTING KEY = %d and rid = %d", key->v.intV, rid.page);
	//printf("\n INSERTING page = %d and slot = %d", rid.page, rid.slot);

	// Check is a record with the spcified key already exists.
	if (findRecord(treeManager->root, key) != NULL) {
		printf("\n insertKey :: KEY EXISTS");
		return RC_IM_KEY_ALREADY_EXISTS;
	}

	// Create a new record (NodeData) for the value RID.
	pointer = makeRecord(&rid);

	// If the tree doesn't exist yet, create a new tree.
	if (treeManager->root == NULL) {
		treeManager->root = createNewTree(treeManager, key, pointer);
		//printf("\n insertKey :: First Node created");
		//printTree(tree);
		return RC_OK;
	}

	// If the tree already exists, then find a leaf where the key can be inserted.
	leaf = findLeaf(treeManager->root, key);

	if (leaf->num_keys < bTreeOrder - 1) {
		// If the leaf has room for the new key, then insert the new key into that leaf.
		leaf = insertIntoLeaf(treeManager, leaf, key, pointer);
	} else {
		// If the leaf dows not have room for the new key, split leaf and then insert the new key into that leaf.
		treeManager->root = insertIntoLeafAfterSplitting(treeManager, leaf, key, pointer);
	}

	// Print the B+ Tree for debugging purpose.
	//printf("\n");
	//printTree(tree);
	return RC_OK;
}

// This method searches the B+ Tree for the specified key and if found stores the RID (value)
// for that key in the memory location pointed by "result" parameter.
extern RC findKey(BTreeHandle *tree, Value *key, RID *result) {
	// Retrieve B+ Tree's metadata information.
	BTreeManager *treeManager = (BTreeManager *) tree->mgmtData;

	// Search the tree for the specified key.
	NodeData * r = findRecord(treeManager->root, key);

	// If returned record is NULL, then the key does not exist in the B+ Tree.
	if (r == NULL) {
		return RC_IM_KEY_NOT_FOUND;
	} else {
		//printf("NodeData -- key %d, page %d, slot = %d \n", key->v.intV, r->rid.page, r->rid.slot);
	}

	// If not NULL, then store the value (RID) to "result"
	*result = r->rid;
	return RC_OK;
}

// This function retrieves the number of nodes present in the B+ Tree.
// The result is stored in the memory location pointed by "result" parameter.
RC getNumNodes(BTreeHandle *tree, int *result) {
	// Retrieve B+ Tree's metadata information.
	BTreeManager * treeManager = (BTreeManager *) tree->mgmtData;
	//printf("\n getNumNodes = %d", treeManager->numNodes);

	// Set the "result" content to numNodes found in our metadata.
	*result = treeManager->numNodes;
	return RC_OK;
}

// This function retrieves the number of entries (i.e. keys) present in the B+ Tree.
// The result is stored in the memory location pointed by "result" parameter.
RC getNumEntries(BTreeHandle *tree, int *result) {
	// Retrieve B+ Tree's metadata information.
	BTreeManager * treeManager = (BTreeManager *) tree->mgmtData;
	//printf("\n getNumEntries = %d", treeManager->numEntries);

	// Set the "result" content to numEntries found in our metadata.
	*result = treeManager->numEntries;
	return RC_OK;
}

// This function retrieves the datatype of the keys in the B+ Tree.
// The result is stored in the memory location pointed by "result" parameter.
RC getKeyType(BTreeHandle *tree, DataType *result) {
	// Retrieve B+ Tree's metadata information.
	BTreeManager * treeManager = (BTreeManager *) tree->mgmtData;

	// Set the "result" content to keyType which stores the datatype of the Key found in our metadata.
	*result = treeManager->keyType;
	return RC_OK;
}

// This function deletes the entry/record with the specified "key" in the B+ Tree.
RC deleteKey(BTreeHandle *tree, Value *key) {
	// Retrieve B+ Tree's metadata information.
	BTreeManager *treeManager = (BTreeManager *) tree->mgmtData;

	// Deleting the entry with the specified key.
	treeManager->root = delete(treeManager, key);
	//printTree(tree);
	return RC_OK;
}

// This function initializes the scan which is used to scan the entries in the B+ Tree.
RC openTreeScan(BTreeHandle *tree, BT_ScanHandle **handle) {
	// Retrieve B+ Tree's metadata information.
	BTreeManager *treeManager = (BTreeManager *) tree->mgmtData;

	// Retrieve B+ Tree Scan's metadata information.
	ScanManager *scanmeta = malloc(sizeof(ScanManager));

	// Allocating some memory space.
	*handle = malloc(sizeof(BT_ScanHandle));

	Node * node = treeManager->root;

	if (treeManager->root == NULL) {
		//printf("Empty tree.\n");
		return RC_NO_RECORDS_TO_SCAN;
	} else {
		//printf("\n openTreeScan() ......... Inside ELse  ");
		while (!node->is_leaf)
			node = node->pointers[0];

		// Initializing (setting) the Scan's metadata information.
		scanmeta->keyIndex = 0;
		scanmeta->totalKeys = node->num_keys;
		scanmeta->node = node;
		scanmeta->order = treeManager->order;
		(*handle)->mgmtData = scanmeta;
		//printf("\n keyIndex = %d, totalKeys = %d ", scanmeta->keyIndex, scanmeta->totalKeys);
	}
	return RC_OK;
}

// This function is used to traverse the entries in the B+ Tree.
// It stores the record details i.e. RID in the memory location pointed by "result" parameter.
RC nextEntry(BT_ScanHandle *handle, RID *result) {
	//printf("\n INSIDE nextEntry()...... ");
	// Retrieve B+ Tree Scan's metadata information.
	ScanManager * scanmeta = (ScanManager *) handle->mgmtData;

	// Retrieving all the information.
	int keyIndex = scanmeta->keyIndex;
	int totalKeys = scanmeta->totalKeys;
	int bTreeOrder = scanmeta->order;
	RID rid;

	//printf("\n keyIndex = %d, totalKeys = %d ", keyIndex, totalKeys);
	Node * node = scanmeta->node;

	// Return error if current node is empty i.e. NULL
	if (node == NULL) {
		return RC_IM_NO_MORE_ENTRIES;
	}

	if (keyIndex < totalKeys) {
		// If current key entry is present on the same leaf node.
		rid = ((NodeData *) node->pointers[keyIndex])->rid;
		//printf(" ... KEYS = %d", node->keys[keyIndex]->v.intV);
		//printf("  page = %d, slot = %d  ", rid.page, rid.slot);
		scanmeta->keyIndex++;
	} else {
		// If all the entries on the leaf node have been scanned, Go to next node...
		if (node->pointers[bTreeOrder - 1] != NULL) {
			node = node->pointers[bTreeOrder - 1];
			scanmeta->keyIndex = 1;
			scanmeta->totalKeys = node->num_keys;
			scanmeta->node = node;
			rid = ((NodeData *) node->pointers[0])->rid;
			//printf("  page = %d, slot = %d  ", rid.page, rid.slot);
		} else {
			// If no next node, it means no more enteies to be scanned..
			return RC_IM_NO_MORE_ENTRIES;
		}
	}
	// Store the record/result/RID.
	*result = rid;
	return RC_OK;
}

// This function closes the scan mechanism and frees up resources.
extern RC closeTreeScan(BT_ScanHandle *handle) {
	handle->mgmtData = NULL;
	free(handle);
	return RC_OK;
}

// This function prints the B+ Tree
extern char *printTree(BTreeHandle *tree) {
	BTreeManager *treeManager = (BTreeManager *) tree->mgmtData;
	printf("\nPRINTING TREE:\n");
	Node * n = NULL;
	int i = 0;
	int rank = 0;
	int new_rank = 0;

	if (treeManager->root == NULL) {
		printf("Empty tree.\n");
		return '\0';
	}
	treeManager->queue = NULL;
	enqueue(treeManager, treeManager->root);
	while (treeManager->queue != NULL) {
		n = dequeue(treeManager);
		if (n->parent != NULL && n == n->parent->pointers[0]) {
			new_rank = path_to_root(treeManager->root, n);
			if (new_rank != rank) {
				rank = new_rank;
				printf("\n");
			}
		}

		// Print key depending on datatype of the key.
		for (i = 0; i < n->num_keys; i++) {
			switch (treeManager->keyType) {
			case DT_INT:
				printf("%d ", (*n->keys[i]).v.intV);
				break;
			case DT_FLOAT:
				printf("%.02f ", (*n->keys[i]).v.floatV);
				break;
			case DT_STRING:
				printf("%s ", (*n->keys[i]).v.stringV);
				break;
			case DT_BOOL:
				printf("%d ", (*n->keys[i]).v.boolV);
				break;
			}
			printf("(%d - %d) ", ((NodeData *) n->pointers[i])->rid.page, ((NodeData *) n->pointers[i])->rid.slot);
		}
		if (!n->is_leaf)
			for (i = 0; i <= n->num_keys; i++)
				enqueue(treeManager, n->pointers[i]);

		printf("| ");
	}
	printf("\n");

	return '\0';
}
