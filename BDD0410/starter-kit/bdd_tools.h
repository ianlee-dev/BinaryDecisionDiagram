#if !defined(BDD_TOOLS_HEADER)
#define BDD_TOOLS_HEADER

#include "bdd.h"
#include "cubical_function_representation.h"

typedef struct bdd_node_info
{
	bdd_node *node;
	struct bdd_node_info *next, *prev;
} bdd_node_info;

typedef struct bdd_level_info
{
	bdd_node_info *info_head, *info_last;
	int leftX;
} bdd_level_info;

typedef struct 
{
	int var_count;
	unsigned int *data;
	int *var_reordering;
} bdd_truth_table;

typedef void (*bdd_callback_ptr)();


// Even more complicated methods for BDD processing
void bdd_reduce(bdd *diagram);

void bdd_add_cube_path(bdd *diagram, bdd_node *node, 
	                     t_blif_cube *cube, int start_input_index, int int_label);
void print_cube(int input_count, t_blif_cube *cube);
int bdd_are_subdiagrams_equal(bdd_node *node1, bdd_node *node2);

bdd_level_info *create_level_infos(int var_count);
							// Allocates and initialises array of level_info
void free_level_info(bdd_level_info *level);
void free_level_infos(bdd_level_info *levels, int var_count);
void bdd_divide_nodes_by_level(bdd_level_info *levels, int level_count, bdd_node *node);
void bdd_label_nodes_by_levels(bdd_level_info *levels, int level_count);
int bdd_print_diagram_statistics(bdd *diagram);
              // Returns diagram's node count
char *bdd_print_diagram_as_table_to_string(bdd *diagram);
              // Prints diagram as table into a string.
              // The string is allocated inside and must be freed by the client

bdd_truth_table *bdd_create_truth_table(bdd *diagram, int consider_reordering);
              // Allocate and build truth table for all possible input combinations.
              // Consider_reordering == 0 means that default diagram variables order
              // is used (build of the table will be faster in this case)
void bdd_free_truth_table(bdd_truth_table *truth_table);
int bdd_are_truth_tables_equal(bdd_truth_table *truth_table1, bdd_truth_table *truth_table2);
int bdd_check_diagrams_truth_tables_equal(bdd *diagram1, bdd *diagram2);

void bdd_set_diagram_changed_callback(bdd_callback_ptr callback_ptr, int min_level);
              // Set function which will be called when something interesting is happening
              // (nodes are removed during reducing)
void bdd_on_diagram_changed(int level);
              // Notify that the current diagram changed (i.e. call the callback if it is set).
              // Level - from 0 to 9. Higher level means more important change

void bdd_set_cur_diagram(bdd *diagram);
              // Sets diagram that will be printed on each next call of bdd_print_cur_diagram_as_table
void bdd_print_cur_diagram_as_table();
              // Function that may be used as callback

#endif