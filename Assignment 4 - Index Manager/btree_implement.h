#ifndef BTREE_IMPLEMENT_H
#define BTREE_IMPLEMENT_H

#include "btree_mgr.h"
#include "buffer_mgr.h"

// Structure that holds the actual data of an entry
typedef struct NodeData {
	RID rid;
} NodeData;

// Structure that represents a node in the B+ Tree
typedef struct Node {
	void ** pointers;
	Value ** keys;
	struct Node * parent;
	bool is_leaf;
	int num_keys;
	struct Node * next; // Used for queue.
} Node;

// Structure that stores additional information of B+ Tree
typedef struct BTreeManager {
	BM_BufferPool bufferPool;
	BM_PageHandle pageHandler;
	int order;
	int numNodes;
	int numEntries;
	Node * root;
	Node * queue;
	DataType keyType;
} BTreeManager;

//Structure that faciltates the scan operation on the B+ Tree
typedef struct ScanManager {
	int keyIndex;
	int totalKeys;
	int order;
	Node * node;
} ScanManager;

// Functions to find an element (record) in the B+ Tree
Node * findLeaf(Node * root, Value * key);
NodeData * findRecord(Node * root, Value * key);

// Functions to support printing of the B+ Tree
void enqueue(BTreeManager * treeManager, Node * new_node);
Node * dequeue(BTreeManager * treeManager);
int path_to_root(Node * root, Node * child);

// Functions to support addition of an element (record) in the B+ Tree
NodeData * makeRecord(RID * rid);
Node * insertIntoLeaf(BTreeManager * treeManager, Node * leaf, Value * key, NodeData * pointer);
Node * createNewTree(BTreeManager * treeManager, Value * key, NodeData * pointer);
Node * createNode(BTreeManager * treeManager);
Node * createLeaf(BTreeManager * treeManager);
Node * insertIntoLeafAfterSplitting(BTreeManager * treeManager, Node * leaf, Value * key, NodeData * pointer);
Node * insertIntoNode(BTreeManager * treeManager, Node * parent, int left_index, Value * key, Node * right);
Node * insertIntoNodeAfterSplitting(BTreeManager * treeManager, Node * parent, int left_index, Value * key, Node * right);
Node * insertIntoParent(BTreeManager * treeManager, Node * left, Value * key, Node * right);
Node * insertIntoNewRoot(BTreeManager * treeManager, Node * left, Value * key, Node * right);
int getLeftIndex(Node * parent, Node * left);

// Functions to support deleting of an element (record) in the B+ Tree
Node * adjustRoot(Node * root);
Node * mergeNodes(BTreeManager * treeManager, Node * n, Node * neighbor, int neighbor_index, int k_prime);
Node * redistributeNodes(Node * root, Node * n, Node * neighbor, int neighbor_index, int k_prime_index, int k_prime);
Node * deleteEntry(BTreeManager * treeManager, Node * n, Value * key, void * pointer);
Node * delete(BTreeManager * treeManager, Value * key);
Node * removeEntryFromNode(BTreeManager * treeManager, Node * n, Value * key, Node * pointer);
int getNeighborIndex(Node * n);

// Functions to support KEYS of multiple datatypes.
bool isLess(Value * key1, Value * key2);
bool isGreater(Value * key1, Value * key2);
bool isEqual(Value * key1, Value * key2);

#endif // BTREE_IMPLEMENT_H
