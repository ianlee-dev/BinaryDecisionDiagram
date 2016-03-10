#if !defined(BDD_APPLY_HEADER)
#define BDD_APPLY_HEADER

#include "bdd.h"
#include "bdd_apply_hash.h"

typedef enum 
{
	apply_operation_and = 1,
	apply_operation_or,
	apply_operation_xor
} apply_operation_type;

bdd_root_node *bdd_create_apply_result(bdd_root_node *left_diagram_root, apply_operation_type operation,
	                                     bdd_root_node *right_diagram_root, int use_hash, int var_count);
              // Creates new diagram - combination of two other diagrams. Non-reduced.
              // Var_count is only necessary to choose good hash size. 0 or -1 mean small default size
void bdd_add_apply_result(bdd *target_diagram, 
	                        bdd *left_diagram, apply_operation_type operation,
	                        bdd *right_diagram, int use_hash, int var_count);
              // The same, but without allocation of the diagram

// Implementation (internal) declarations
void bdd_do_apply(bdd *target_diagram, bdd_node *target_parent, int target_high,
	                bdd_node *left_node, apply_operation_type operation,
	                bdd_node *right_nodes, apply_hash_table *hash_table);
int apply_operation_result(int left, apply_operation_type operation, int right);
              // Applies boolean operation to two arguments. Left, right and result - 0 or 1


#endif