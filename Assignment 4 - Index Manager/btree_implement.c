#include "btree_implement.h"
#include "dt.h"
#include "string.h"

/*********** INSERTION *************/

// Creates a new record (NodeData) to hold the value to which a key refers.
// Returns the record structure.
NodeData * makeRecord(RID * rid) {
	NodeData * record = (NodeData *) malloc(sizeof(NodeData));
	if (record == NULL) {
		perror("NodeData creation.");
		exit(RC_INSERT_ERROR);
	} else {
		//((RID *) new_NodeData->rid)->page = 0;//rid->page;
		//((RID *) new_NodeData->rid)->slot = 0;//rid->slot;
		record->rid.page = rid->page;
		record->rid.slot = rid->slot;
	}
	//printf("  New NodeData values =%d ..... ", record->rid.page);
	return record;
}

// Creates a new tree when the first element (NodeData) is inserted.
Node * createNewTree(BTreeManager * treeManager, Value * key, NodeData * pointer) {

	Node * root = createLeaf(treeManager);
	int bTreeOrder = treeManager->order;
	//printf("... insertKEY POINTER page = %d  slot = %d \n", pointer->rid.page, pointer->rid.slot);

	root->keys[0] = key;
	root->pointers[0] = pointer;
	root->pointers[bTreeOrder - 1] = NULL;
	root->parent = NULL;
	root->num_keys++;

	treeManager->numEntries++;
	//printf("\n START NEW NODE END \n");

	return root;
}

// Inserts a new pointer to the record (NodeData) and its corresponding key into a leaf.
// Returns the altered leaf.
Node * insertIntoLeaf(BTreeManager * treeManager, Node * leaf, Value * key, NodeData * pointer) {
	//printf("... insertKEY POINTER page = %d  slot = %d \n", pointer->rid.page, pointer->rid.slot);

	int i, insertion_point;
	treeManager->numEntries++;

	insertion_point = 0;
	while (insertion_point < leaf->num_keys && isLess(leaf->keys[insertion_point], key))
		insertion_point++;

	for (i = leaf->num_keys; i > insertion_point; i--) {
		leaf->keys[i] = leaf->keys[i - 1];
		leaf->pointers[i] = leaf->pointers[i - 1];
	}
	leaf->keys[insertion_point] = key;
	leaf->pointers[insertion_point] = pointer;
	leaf->num_keys++;
	return leaf;
}

// Inserts a new key and pointer to a new record (NodeData) into a leaf so as to exceed the tree's order,
// causing the leaf to be split in half.
Node * insertIntoLeafAfterSplitting(BTreeManager * treeManager, Node * leaf, Value * key, NodeData * pointer) {

	//printf("... insertIntoLeafAfterSplitting POINTER page = %d  slot = %d \n", pointer->rid.page, pointer->rid.slot);

	Node * new_leaf;
	Value ** temp_keys;
	void ** temp_pointers;
	int insertion_index, split, new_key, i, j;

	new_leaf = createLeaf(treeManager);
	int bTreeOrder = treeManager->order;

	temp_keys = malloc(bTreeOrder * sizeof(Value));
	if (temp_keys == NULL) {
		perror("Temporary keys array.");
		exit(RC_INSERT_ERROR);
	}

	temp_pointers = malloc(bTreeOrder * sizeof(void *));
	if (temp_pointers == NULL) {
		perror("Temporary pointers array.");
		exit(RC_INSERT_ERROR);
	}

	//printf("\n Splitting initialized.... ");
	insertion_index = 0;
	while (insertion_index < bTreeOrder - 1 && isLess(leaf->keys[insertion_index], key))
		insertion_index++;

	for (i = 0, j = 0; i < leaf->num_keys; i++, j++) {
		if (j == insertion_index)
			j++;
		temp_keys[j] = leaf->keys[i];
		temp_pointers[j] = leaf->pointers[i];
	}

	temp_keys[insertion_index] = key;
	temp_pointers[insertion_index] = pointer;

	leaf->num_keys = 0;

	// Splitting
	if ((bTreeOrder - 1) % 2 == 0)
		split = (bTreeOrder - 1) / 2;
	else
		split = (bTreeOrder - 1) / 2 + 1;

	for (i = 0; i < split; i++) {
		leaf->pointers[i] = temp_pointers[i];
		leaf->keys[i] = temp_keys[i];
		leaf->num_keys++;
	}

	for (i = split, j = 0; i < bTreeOrder; i++, j++) {
		new_leaf->pointers[j] = temp_pointers[i];
		new_leaf->keys[j] = temp_keys[i];
		new_leaf->num_keys++;
	}

	free(temp_pointers);
	free(temp_keys);

	new_leaf->pointers[bTreeOrder - 1] = leaf->pointers[bTreeOrder - 1];
	leaf->pointers[bTreeOrder - 1] = new_leaf;

	for (i = leaf->num_keys; i < bTreeOrder - 1; i++)
		leaf->pointers[i] = NULL;
	for (i = new_leaf->num_keys; i < bTreeOrder - 1; i++)
		new_leaf->pointers[i] = NULL;

	new_leaf->parent = leaf->parent;
	new_key = new_leaf->keys[0];
	treeManager->numEntries++;
	return insertIntoParent(treeManager, leaf, new_key, new_leaf);
}

// Inserts a new key and pointer to a node into a node, causing the node's size to exceed
// the order, and causing the node to split into two.
Node * insertIntoNodeAfterSplitting(BTreeManager * treeManager, Node * old_node, int left_index, Value * key, Node * right) {

	int i, j, split, k_prime;
	Node * new_node, *child;
	Value ** temp_keys;
	Node ** temp_pointers;

	int bTreeOrder = treeManager->order;

	/* First we create a temporary set of keys and pointers
	 * to hold everything in order, including
	 * the new key and pointer, inserted in their
	 * correct places.
	 * Then create a new node and copy half of the
	 * keys and pointers to the old node and
	 * the other half to the new.
	 */

	temp_pointers = malloc((bTreeOrder + 1) * sizeof(Node *));
	if (temp_pointers == NULL) {
		perror("Temporary pointers array for splitting nodes.");
		exit(RC_INSERT_ERROR);
	}
	temp_keys = malloc(bTreeOrder * sizeof(Value *));
	if (temp_keys == NULL) {
		perror("Temporary keys array for splitting nodes.");
		exit(RC_INSERT_ERROR);
	}

	for (i = 0, j = 0; i < old_node->num_keys + 1; i++, j++) {
		if (j == left_index + 1)
			j++;
		temp_pointers[j] = old_node->pointers[i];
	}

	for (i = 0, j = 0; i < old_node->num_keys; i++, j++) {
		if (j == left_index)
			j++;
		temp_keys[j] = old_node->keys[i];
	}

	temp_pointers[left_index + 1] = right;
	temp_keys[left_index] = key;

	/* Create the new node and copy
	 * half the keys and pointers to the
	 * old and half to the new.
	 */
	if ((bTreeOrder - 1) % 2 == 0)
		split = (bTreeOrder - 1) / 2;
	else
		split = (bTreeOrder - 1) / 2 + 1;

	new_node = createNode(treeManager);
	old_node->num_keys = 0;
	for (i = 0; i < split - 1; i++) {
		old_node->pointers[i] = temp_pointers[i];
		old_node->keys[i] = temp_keys[i];
		old_node->num_keys++;
	}
	old_node->pointers[i] = temp_pointers[i];
	k_prime = temp_keys[split - 1];
	for (++i, j = 0; i < bTreeOrder; i++, j++) {
		new_node->pointers[j] = temp_pointers[i];
		new_node->keys[j] = temp_keys[i];
		new_node->num_keys++;
	}
	new_node->pointers[j] = temp_pointers[i];
	free(temp_pointers);
	free(temp_keys);
	new_node->parent = old_node->parent;
	for (i = 0; i <= new_node->num_keys; i++) {
		child = new_node->pointers[i];
		child->parent = new_node;
	}

	/* Insert a new key into the parent of the two
	 * nodes resulting from the split, with
	 * the old node to the left and the new to the right.
	 */

	treeManager->numEntries++;
	return insertIntoParent(treeManager, old_node, k_prime, new_node);
}

// Inserts a new node (leaf or internal node) into the B+ tree.
// Returns the root of the tree after insertion.
Node * insertIntoParent(BTreeManager * treeManager, Node * left, Value * key, Node * right) {

	//printf("\n Inserting into Parent.... \n");
	int left_index;
	Node * parent = left->parent;
	int bTreeOrder = treeManager->order;

	// Checking if it is the new root.
	if (parent == NULL)
		return insertIntoNewRoot(treeManager, left, key, right);

	// In case its a leaf or node, find the parent's pointer to the left node.
	left_index = getLeftIndex(parent, left);

	// If the new key can accommodate in the node.
	if (parent->num_keys < bTreeOrder - 1) {
		return insertIntoNode(treeManager, parent, left_index, key, right);
	}

	// In case it cannot accomodate, then split the node preserving the B+ Tree properties.
	return insertIntoNodeAfterSplitting(treeManager, parent, left_index, key, right);
}

// Function used in insertIntoParent to findRecord the index of the parent's pointer to
// the node to the left of the key to be inserted.
int getLeftIndex(Node * parent, Node * left) {
	int left_index = 0;
	while (left_index <= parent->num_keys && parent->pointers[left_index] != left)
		left_index++;
	return left_index;
}

// Inserts a new key and pointer to a node into a node into which these can fit without violating the B+ tree properties.
Node * insertIntoNode(BTreeManager * treeManager, Node * parent, int left_index, Value * key, Node * right) {
	int i;
	for (i = parent->num_keys; i > left_index; i--) {
		parent->pointers[i + 1] = parent->pointers[i];
		parent->keys[i] = parent->keys[i - 1];
	}

	parent->pointers[left_index + 1] = right;
	parent->keys[left_index] = key;
	parent->num_keys++;

	return treeManager->root;
}

// Creates a new root for two subtrees and inserts the appropriate key into the new root.
Node * insertIntoNewRoot(BTreeManager * treeManager, Node * left, Value * key, Node * right) {
	Node * root = createNode(treeManager);
	root->keys[0] = key;
	root->pointers[0] = left;
	root->pointers[1] = right;
	root->num_keys++;
	root->parent = NULL;
	left->parent = root;
	right->parent = root;
	return root;
}

// Creates a new general node, which can be adapted to serve as either a leaf or an internal node.
Node * createNode(BTreeManager * treeManager) {
	//printf("\n Inside createNode...  ");
	treeManager->numNodes++;
	int bTreeOrder = treeManager->order;

	Node * new_node = malloc(sizeof(Node));
	if (new_node == NULL) {
		perror("Node creation.");
		exit(RC_INSERT_ERROR);
	}
	//printf("\n SIZE allocated for NODE  ");

	new_node->keys = malloc((bTreeOrder - 1) * sizeof(Value *));
	if (new_node->keys == NULL) {
		perror("New node keys array.");
		exit(RC_INSERT_ERROR);
	}
	//printf("\n SIZE allocated for VALUE  ");

	new_node->pointers = malloc(bTreeOrder * sizeof(void *));
	if (new_node->pointers == NULL) {
		perror("New node pointers array.");
		exit(RC_INSERT_ERROR);
	}
	//printf("\n SIZE allocated for POINTERS  ");

	new_node->is_leaf = false;
	new_node->num_keys = 0;
	new_node->parent = NULL;
	new_node->next = NULL;
	return new_node;
}

// Creates a new leaf by creating a node.
Node * createLeaf(BTreeManager * treeManager) {
	Node * leaf = createNode(treeManager);
	leaf->is_leaf = true;
	return leaf;
}

//Searches for the key from root to the leaf.
// Returns the leaf containing the given key.
Node * findLeaf(Node * root, Value * key) {
	//printf(" INside findLeaf... ");
	int i = 0;
	Node * c = root;
	if (c == NULL) {
		//printf("Empty tree.\n");
		return c;
	}
	while (!c->is_leaf) {
		i = 0;
		while (i < c->num_keys) {
			//printf("\nfindLeaf()... key = %s, c->keys[i] = %s \n", key->v.stringV, c->keys[i]->v.stringV);
			if (isGreater(key, c->keys[i]) || isEqual(key, c->keys[i])) {
				i++;
			} else
				break;
		}
		c = (Node *) c->pointers[i];
	}
	return c;
}

// Finds and returns the record (NodeData) to which a key refers.
NodeData * findRecord(Node * root, Value *key) {
	//printf(" Inside findRecord... ");
	int i = 0;
	Node * c = findLeaf(root, key);
	if (c == NULL)
		return NULL;
	for (i = 0; i < c->num_keys; i++) {
		//printf(" .. c->keys[i] = %s, key = %s .. ", c->keys[i]->v.stringV, key->v.stringV);
		if (isEqual(c->keys[i], key))
			break;
	}
	if (i == c->num_keys)
		return NULL;
	else
		return (NodeData *) c->pointers[i];
}

/*********** DELETION *************/

// Returns the index of a node's nearest neighbor (sibling) to the left if one exists.
// If not (the node is the leftmost child), returns -1 to signify this special case.
int getNeighborIndex(Node * n) {

	int i;

	/* Return the index of the key to the left
	 * of the pointer in the parent pointing
	 * to n.
	 * If n is the leftmost child, this means
	 * return -1.
	 */
	for (i = 0; i <= n->parent->num_keys; i++)
		if (n->parent->pointers[i] == n)
			return i - 1;

	// Error state.
	//printf("Search for nonexistent pointer to node in parent.\n");
	printf("Node:  %#lx\n", (unsigned long) n);
	exit(RC_ERROR);
}

// Remove a record having the specified key from the the specified node.
Node * removeEntryFromNode(BTreeManager * treeManager, Node * n, Value * key, Node * pointer) {

	//printf("\n INSIDE removeEntryFromNode KEY\n");
	int i, num_pointers;
	int bTreeOrder = treeManager->order;

	// Remove the key and shift other keys accordingly.
	i = 0;

	while (!isEqual(n->keys[i], key))
		i++;

	for (++i; i < n->num_keys; i++)
		n->keys[i - 1] = n->keys[i];

	// Remove the pointer and shift other pointers accordingly.
	// First determine number of pointers.
	num_pointers = n->is_leaf ? n->num_keys : n->num_keys + 1;
	i = 0;
	while (n->pointers[i] != pointer)
		i++;
	for (++i; i < num_pointers; i++)
		n->pointers[i - 1] = n->pointers[i];

	// One key fewer.
	n->num_keys--;
	treeManager->numEntries--;

	// Set the other pointers to NULL for tidiness.
	// A leaf uses the last pointer to point to the next leaf.
	if (n->is_leaf)
		for (i = n->num_keys; i < bTreeOrder - 1; i++)
			n->pointers[i] = NULL;
	else
		for (i = n->num_keys + 1; i < bTreeOrder; i++)
			n->pointers[i] = NULL;

	return n;
}

// This function adjusts the root after a record has been deleted from the B+ Tree
Node * adjustRoot(Node * root) {

	Node * new_root;

	// If the root is not empty then it means that key and pointer has been deleted already.
	// Do nothing and return the root.
	if (root->num_keys > 0)
		return root;

	if (!root->is_leaf) {
		//If the root is not empty and if it has a child, promote
		// the first (only) child as the new root.
		new_root = root->pointers[0];
		new_root->parent = NULL;
	} else {
		// If the root is not empty and if it is a leaf (has no children), then the whole tree is empty.
		new_root = NULL;
	}

	// De-allocate memory space... Free up space...
	free(root->keys);
	free(root->pointers);
	free(root);

	return new_root;
}

// Combines a node that has become too small after deletion with a neighboring node that
// can accept the additional entries without exceeding the maximum.
Node * mergeNodes(BTreeManager * treeManager, Node * n, Node * neighbor, int neighbor_index, int k_prime) {

	int i, j, neighbor_insertion_index, n_end;
	Node * tmp;
	int bTreeOrder = treeManager->order;

	// Swap neighbor with node if node is on the extreme left and neighbor is to its right.
	if (neighbor_index == -1) {
		tmp = n;
		n = neighbor;
		neighbor = tmp;
	}

	// Starting point in the neighbor for copying keys and pointers from n.
	// n and neighbor have swapped places in the special case of n being a leftmost child.
	neighbor_insertion_index = neighbor->num_keys;

	// If its a non-leaf node, append k_prime and the following pointer.
	// Also, append all pointers and keys from the neighbor.
	if (!n->is_leaf) {
		neighbor->keys[neighbor_insertion_index] = k_prime;
		neighbor->num_keys++;

		n_end = n->num_keys;

		for (i = neighbor_insertion_index + 1, j = 0; j < n_end; i++, j++) {
			neighbor->keys[i] = n->keys[j];
			neighbor->pointers[i] = n->pointers[j];
			neighbor->num_keys++;
			n->num_keys--;
		}

		neighbor->pointers[i] = n->pointers[j];

		// Pointing all children to the same parent.
		for (i = 0; i < neighbor->num_keys + 1; i++) {
			tmp = (Node *) neighbor->pointers[i];
			tmp->parent = neighbor;
		}
	} else {
		// In a leaf, append the keys and pointers of n to the neighbor.
		// Set the neighbor's last pointer to point to what had been n's right neighbor.
		for (i = neighbor_insertion_index, j = 0; j < n->num_keys; i++, j++) {
			neighbor->keys[i] = n->keys[j];
			neighbor->pointers[i] = n->pointers[j];
			neighbor->num_keys++;
		}
		neighbor->pointers[bTreeOrder - 1] = n->pointers[bTreeOrder - 1];
	}

	treeManager->root = deleteEntry(treeManager, n->parent, k_prime, n);

	// De-allocate memory space... Free up space...
	free(n->keys);
	free(n->pointers);
	free(n);
	return treeManager->root;
}

// This function deletes an entry from the B+ tree. It removes the record having the specified key and pointer
// from the leaf, and then makes all appropriate changes to preserve the B+ tree properties.
Node * deleteEntry(BTreeManager * treeManager, Node * n, Value * key, void * pointer) {
	//printf("\n INSIDE DELETE ENTRY \n");
	int min_keys;
	Node * neighbor;
	int neighbor_index;
	int k_prime_index, k_prime;
	int capacity;
	int bTreeOrder = treeManager->order;

	// Remove key and pointer from node.
	n = removeEntryFromNode(treeManager, n, key, pointer);

	// If n is root then perform adjustements
	if (n == treeManager->root)
		return adjustRoot(treeManager->root);

	// Determine minimum allowable size of node, to be preserved after deletion.
	if (n->is_leaf) {
		if ((bTreeOrder - 1) % 2 == 0)
			min_keys = (bTreeOrder - 1) / 2;
		else
			min_keys = (bTreeOrder - 1) / 2 + 1;
	} else {
		if ((bTreeOrder) % 2 == 0)
			min_keys = (bTreeOrder) / 2;
		else
			min_keys = (bTreeOrder) / 2 + 1;
		min_keys--;
	}

	// Node stays at or above minimum.
	if (n->num_keys >= min_keys)
		return treeManager->root;

	// If the node falls below minimum, either merging or redistribution is needed.
	// Find the appropriate neighbor node with which to merge. Also find the key (k_prime)
	// in the parent between the pointer to node n and the pointer to the neighbor.
	neighbor_index = getNeighborIndex(n);
	k_prime_index = neighbor_index == -1 ? 0 : neighbor_index;
	k_prime = n->parent->keys[k_prime_index];
	neighbor =
			(neighbor_index == -1) ? n->parent->pointers[1] : n->parent->pointers[neighbor_index];

	capacity = n->is_leaf ? bTreeOrder : bTreeOrder - 1;

	if (neighbor->num_keys + n->num_keys < capacity)
		// Merging
		return mergeNodes(treeManager, n, neighbor, neighbor_index, k_prime);
	else
		// Re-distributing
		return redistributeNodes(treeManager->root, n, neighbor, neighbor_index, k_prime_index, k_prime);
}

// This function deletes the the entry/record having the specified key.
Node * delete(BTreeManager * treeManager, Value * key) {
	//printf("\n INSIDE DELETE()...");

	Node * record = findRecord(treeManager->root, key);
	NodeData * key_leaf = findLeaf(treeManager->root, key);

	if (record != NULL && key_leaf != NULL) {
		treeManager->root = deleteEntry(treeManager, key_leaf, key, record);
		free(record);
	}
	//printf("\n EXITING DELETE()...");
	return treeManager->root;
}

// This function redistributes the entries between two nodes when one has become too small after deletion
// but its neighbor is too big to append the small node's entries without exceeding the maximum
Node * redistributeNodes(Node * root, Node * n, Node * neighbor, int neighbor_index, int k_prime_index, int k_prime) {
	int i;
	Node * tmp;

	if (neighbor_index != -1) {
		// If n has neighbor to the left, pull the neighbor's last key-pointer pair over from the neighbor's right end to n's left end.
		if (!n->is_leaf)
			n->pointers[n->num_keys + 1] = n->pointers[n->num_keys];
		for (i = n->num_keys; i > 0; i--) {
			n->keys[i] = n->keys[i - 1];
			n->pointers[i] = n->pointers[i - 1];
		}
		if (!n->is_leaf) {
			n->pointers[0] = neighbor->pointers[neighbor->num_keys];
			tmp = (Node *) n->pointers[0];
			tmp->parent = n;
			neighbor->pointers[neighbor->num_keys] = NULL;
			n->keys[0] = k_prime;
			n->parent->keys[k_prime_index] = neighbor->keys[neighbor->num_keys - 1];
		} else {
			n->pointers[0] = neighbor->pointers[neighbor->num_keys - 1];
			neighbor->pointers[neighbor->num_keys - 1] = NULL;
			n->keys[0] = neighbor->keys[neighbor->num_keys - 1];
			n->parent->keys[k_prime_index] = n->keys[0];
		}
	} else {
		// If n is the leftmost child, take a key-pointer pair from the neighbor to the right.
		// Move the neighbor's leftmost key-pointer pair to n's rightmost position.
		if (n->is_leaf) {
			n->keys[n->num_keys] = neighbor->keys[0];
			n->pointers[n->num_keys] = neighbor->pointers[0];
			n->parent->keys[k_prime_index] = neighbor->keys[1];
		} else {
			n->keys[n->num_keys] = k_prime;
			n->pointers[n->num_keys + 1] = neighbor->pointers[0];
			tmp = (Node *) n->pointers[n->num_keys + 1];
			tmp->parent = n;
			n->parent->keys[k_prime_index] = neighbor->keys[0];
		}
		for (i = 0; i < neighbor->num_keys - 1; i++) {
			neighbor->keys[i] = neighbor->keys[i + 1];
			neighbor->pointers[i] = neighbor->pointers[i + 1];
		}
		if (!n->is_leaf)
			neighbor->pointers[i] = neighbor->pointers[i + 1];
	}

	// n now has one more key and one more pointer; the neighbor has one fewer of each.
	n->num_keys++;
	neighbor->num_keys--;

	return root;
}

/*********** PRINTING *************/

// This function helps in printing the B+ Tree
void enqueue(BTreeManager * treeManager, Node * new_node) {
	Node * c;
	if (treeManager->queue == NULL) {
		treeManager->queue = new_node;
		treeManager->queue->next = NULL;
	} else {
		c = treeManager->queue;
		while (c->next != NULL) {
			c = c->next;
		}
		c->next = new_node;
		new_node->next = NULL;
	}
}

// This function helps in printing the B+ Tree
Node * dequeue(BTreeManager * treeManager) {
	Node * n = treeManager->queue;
	treeManager->queue = treeManager->queue->next;
	n->next = NULL;
	return n;
}

// This function gives the length in edges of the path from any node to the root.
int path_to_root(Node * root, Node * child) {
	int length = 0;
	Node * c = child;
	while (c != root) {
		c = c->parent;
		length++;
	}
	return length;
}

/*********** SUPPORT MULTIPLE DATATYPES *************/

// This function compares two keys and returns TRUE if first key is less than second key.
bool isLess(Value * key1, Value * key2) {
	switch (key1->dt) {
	case DT_INT:
		if (key1->v.intV < key2->v.intV) {
			return TRUE;
		} else {
			return FALSE;
		}
		break;
	case DT_FLOAT:
		if (key1->v.floatV < key2->v.floatV) {
			return TRUE;
		} else {
			return FALSE;
		}
		break;
	case DT_STRING:
		if (strcmp(key1->v.stringV, key2->v.stringV) == -1) {
			return TRUE;
		} else {
			return FALSE;
		}
		break;
	case DT_BOOL:
		// Boolean datatype can only be Equal or Not Equal To
		return FALSE;
		break;
	}
}

// This function compares two keys and returns TRUE if first key is greater than second key.
bool isGreater(Value * key1, Value * key2) {
	switch (key1->dt) {
	case DT_INT:
		if (key1->v.intV > key2->v.intV) {
			return TRUE;
		} else {
			return FALSE;
		}
		break;
	case DT_FLOAT:
		if (key1->v.floatV > key2->v.floatV) {
			return TRUE;
		} else {
			return FALSE;
		}
		break;
	case DT_STRING:
		if (strcmp(key1->v.stringV, key2->v.stringV) == 1) {
			return TRUE;
		} else {
			return FALSE;
		}
		break;
	case DT_BOOL:
		// Boolean datatype can only be Equal or Not Equal To
		return FALSE;
		break;
	}
}

// This function compares two keys and returns TRUE if first key is equal to the second key else returns FALSE.
bool isEqual(Value * key1, Value * key2) {
	switch (key1->dt) {
	case DT_INT:
		if (key1->v.intV == key2->v.intV) {
			return TRUE;
		} else {
			return FALSE;
		}
		break;
	case DT_FLOAT:
		if (key1->v.floatV == key2->v.floatV) {
			return TRUE;
		} else {
			return FALSE;
		}
		break;
	case DT_STRING:
		if (strcmp(key1->v.stringV, key2->v.stringV) == 0) {
			return TRUE;
		} else {
			return FALSE;
		}
		break;
	case DT_BOOL:
		if (key1->v.boolV == key2->v.boolV) {
			return TRUE;
		} else {
			return FALSE;
		}
		break;
	}
}
