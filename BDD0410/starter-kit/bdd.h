#if !defined(BDD_HEADER)
#define BDD_HEADER

#define BDD_TERMINAL_NODE_0 0x10
#define BDD_TERMINAL_NODE_1 0x11
							// Usually pointers < 0x10000 are invalid, so we may use such values
#define BDD_MAX_NODE_LABEL_LEN 7

//#define BDD_VERBOSE_CORRECTNESS_CHECK
// Uncomment for additional program self-checking

struct bdd_node_predec_link;

typedef struct bdd_node
{
	// It's ok to read all structure members. But changing of them, excluding label,
	// must be made with methods in bdd.h only

	struct bdd_node *parent;     // First node's predecessor 
	int var_index;               // Index of the corresponding variable 
	                             // in the var_reordering array, from 0 to n - 1 
	struct bdd_node *low, *high; // 0 and 1 nodes below (successors)
	                             // May also contain BDD_TERMINAL_NODE_* or NULL 
	                             // (but NULL means that the diagram is not built)
	struct bdd_node_predec_link *predecs_head;   
	                             // Head of linked list of this node's predessors, excluding parent
	char label[BDD_MAX_NODE_LABEL_LEN + 1];
	int table_id;
} bdd_node;

typedef bdd_node bdd_root_node;     
// Sometimes diagrams are simply identified by theirs roots.
// Usually this is very convenient since we can process diagrams and subdiagrams 
// almost equally. So in most code we use this simplified typedef.
// One rare problem with this approach - we can't remove the root node.
// And another, less rare, but solved by means of bdd_get_node_diagram: 
// sometimes we need to search for nodes in the entire root_node.

typedef struct
{
	bdd_node *root_node;
	int var_count;
	int *var_reordering;    // I-th element contains initial index of the source variable
	                        // which is on the i-th place now
	int max_table_id;
} bdd;

typedef void (*bdd_error_callback_ptr)(const char *err);

// Dynamic allocation-deallocation of structures
bdd *bdd_create_diagram(int var_count);
void bdd_free_diagram(bdd *diagram);

void bdd_free_subdiagram(bdd_node *node);
bdd_node *bdd_create_node(bdd *diagram, int var_index);
bdd_node *bdd_create_child_node(bdd *diagram, bdd_node *parent, int high, int var_index);
void bdd_free_node(bdd_node *node);  
              // Frees node without predecessors and children

//// Initialization of already allocated structures
//// (but of course it's not complete because you also need to fill nodes' low and high).
//// These functions are not actual - *create* methods are more convenient
//void bdd_init_diagram(bdd_root_node *root_node);
//void bdd_init_node(bdd_node *node, bdd_node *parent, int var_index);

// Building operations
void bdd_add_terminal_child(bdd_node *node, int to_high, int value);
							// To_high == 0 means low child, == 1 means high child
							// Value - 0 or 1
void bdd_add_child_node(bdd_node *node, int to_high, bdd_node *child_node);

// Simple modify operations
void bdd_replace_child_node(bdd_node *node, int to_high, bdd_node *child_node);
void bdd_replace_child_links(bdd_node *changing_node,
	                           bdd_node *from_node, bdd_node *to_node);
              // Replaces low/high changing_node's links which point to from_node
void bdd_replace_links_at_predecessors(bdd_node *from_node, bdd_node *to_node);
              // Replaces all links to from_node with links to to_node
void bdd_replace_node_var_index(bdd_node *node, int new_var_index);

// More complicated operations
bdd_root_node *bdd_get_node_diagram(bdd_node *node);
bdd_node *bdd_get_node_by_path(bdd_node *root, char *path);
              // Finds subchild. E.g. for path = "01" - parent->low->high
void bdd_get_node_full_path(char *path_result, bdd_node *node);
              // Proceed to node's diagram root and stores node's path from there
bdd_node *bdd_get_equal_node(bdd_node *start_search_node, bdd_node *node);
              // Finds the given node with equal subgraph as 
              // under node in start_search_node's diagram.
              // Warning: the node is found through paths, so it may work incorrectly
              // if the subdiagrams above search_start_node and node are different.
              // But for the bdd_clone_subdiagram this method is ok

bdd *bdd_clone_diagram(bdd *diagram);
bdd_node *bdd_clone_subdiagram(bdd *target_diagram, bdd_node *node, int copy_table_ids);
              // Clones diagram or subdiagram. If the result will "live" by its own,
              // target_diagram_root must be NULL, otherwise target_diagram_root must be the root
              // of the diagram the cloned subdiagram will be attached to.
              // Copy_table_ids == 0 means that table Ids will be generated,
              // copy_table_ids == 1 means that they will be copied from source nodes
bdd_node *bdd_clone_node(bdd *diagram, bdd_node *node);
              // Creates node with the same children. Doesn't attach it to any parent

int bdd_is_node_terminal(bdd_node *node);
              // Returns 1 if node is terminal 0 or 1
int bdd_is_node_terminal_or_null(bdd_node *node);
int bdd_is_terminal_node_1(bdd_node *node);
int bdd_is_predecessor_single(bdd_node *node);

void bdd_report_error(const char *err);
void bdd_set_error_callback(bdd_error_callback_ptr callback_ptr);

#endif 