/**********************************************************************/
/*** HEADER FILES *****************************************************/
/**********************************************************************/

//#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <malloc.h>
#include "bdd_tools.h"
#include "blif_common.h"
#include "cubical_function_representation.h"

bdd_callback_ptr g_diagram_changed_callback_ptr = NULL;
int g_min_importance_level_for_callback = -1;
bdd *g_cur_printing_diagram = NULL;
char *g_prev_printed_diagram_str = NULL;

/**********************************************************************/
/*** CREATION OF TREE FROM BLIF ***************************************/
/**********************************************************************/

#pragma warning(disable: 4996) 


void bdd_add_cube_path(bdd *diagram, bdd_node *node, 
	                     t_blif_cube *cube, int start_input_index, int int_label)
{
	const int input_count = diagram->var_count;
	bdd_node *cur_node = node;
	bdd_node *new_node;
	int input_index, value;

	for (input_index = start_input_index; input_index < input_count - 1; input_index++)
	{
		if (cur_node->var_index != input_index)
			bdd_report_error("Variable index mismatch in bdd_add_cube_path");   // Some logic error has happened

		switch (read_cube_variable(cube->signal_status, input_index))
		{
			case LITERAL_0:
				if (bdd_is_node_terminal(cur_node->low))
					bdd_report_error("Terminal node already present");   // Very strange. Most likely logic error
				if (cur_node->low == NULL)
				{
					new_node = bdd_create_child_node(diagram, cur_node, 0, input_index + 1);
					sprintf(new_node->label, "%d", int_label);
				}
				else  if (cur_node->low == cur_node->high)
				{
					new_node = bdd_clone_subdiagram(diagram, cur_node->low, 0);
					bdd_replace_child_node(cur_node, 1, new_node);
					sprintf(cur_node->high->label, "%d", int_label);
				}

				cur_node = cur_node->low;
				break;

			case LITERAL_1:
				if (bdd_is_node_terminal(cur_node->high))
					bdd_report_error("Terminal node already present");   // Very strange. Most likely logic error
				if (cur_node->high == NULL)
				{
					new_node = bdd_create_child_node(diagram, cur_node, 1, input_index + 1);
					sprintf(new_node->label, "%d", int_label);
				}
				else  if (cur_node->low == cur_node->high)
				{
					new_node = bdd_clone_subdiagram(diagram, cur_node->low, 0);
					bdd_replace_child_node(cur_node, 1, new_node);
					sprintf(cur_node->high->label, "%d", int_label);
				}

				cur_node = cur_node->high;
				break;

			case LITERAL_DC:  // 0 and 1
				if (bdd_is_node_terminal(cur_node->low) || bdd_is_node_terminal(cur_node->high))
					bdd_report_error("Terminal node already present");   // Very strange. Most likely logic error
				if (cur_node->low == cur_node->high)
				{
					if (cur_node->low == NULL)
					{
						new_node = bdd_create_child_node(diagram, cur_node, 0, input_index + 1);
						sprintf(new_node->label, "%d", int_label);
						bdd_add_child_node(cur_node, 1, new_node);
					}

					cur_node = cur_node->low;
				}
				else
				{
					if (cur_node->low == NULL)
					{
						new_node = bdd_create_child_node(diagram, cur_node, 0, input_index + 1);
						sprintf(new_node->label, "%d", int_label);
					}
					
					// Here we continue processing recursively, independently for both branches.
					// In all other cases we can easily go without recursion - 
					// with only for (input_index = ...)
					bdd_add_cube_path(diagram, cur_node->low, 
						                cube, input_index + 1, int_label);
					
					if (cur_node->high == NULL)
					{
						new_node = bdd_create_child_node(diagram, cur_node, 1, input_index + 1);
						sprintf(new_node->label, "%d", int_label);
					}
					
					bdd_add_cube_path(diagram, cur_node->high, 
						                cube, input_index + 1, int_label);
					return;  
				}
				break;

			default:
				bdd_report_error("Unexpected cube variable error in bdd_add_cube_path");
	  }
	}
	bdd_on_diagram_changed(4);

	input_index = input_count - 1; 
	value = !cube->is_DC;
	switch (read_cube_variable(cube->signal_status, input_index))
	{
		case LITERAL_0:
			bdd_add_terminal_child(cur_node, 0, value);
			break;
		case LITERAL_1:
			bdd_add_terminal_child(cur_node, 1, value);
			break;
		case LITERAL_DC:
			bdd_add_terminal_child(cur_node, 0, value);
			bdd_add_terminal_child(cur_node, 1, value);
			break;
		default:
			bdd_report_error("Unexpected cube variable error in bdd_add_cube_path");
	}
	bdd_on_diagram_changed(6);
}

/**********************************************************************/
/*** MINIMIZATION CODE ************************************************/
/**********************************************************************/

void bdd_remove_redundant_checks(bdd_node *node)
{
	if (node == NULL)
	{
	//	bdd_report_error("NULL node");   // Useful, but too much errors on our examples
		return;
	}

	if (!bdd_is_node_terminal(node->low))
	{
		if ((node->low && node->low->parent == node) || 
			  node->low == NULL)   // Just to report the error
			bdd_remove_redundant_checks(node->low);
  	// The ->parent == node check guarantees that we will not made excess passes by children 
	}
	if (!bdd_is_node_terminal(node->high))
	{
		if ((node->high && node->high->parent == node) ||
			  node->high == NULL)
			bdd_remove_redundant_checks(node->high);
	}

	if (node->low == node->high && node->parent != NULL)
	{
		bdd_replace_links_at_predecessors(node, node->low);
		bdd_replace_child_links(node, node->low, NULL);
		bdd_free_node(node);

		bdd_on_diagram_changed(5);
	}
	// TODO: To use bdd * here. Now the following 
	// isn't reduced correctly since child node should replace the root.
	// -0 -
  // -1 1
}

bdd_level_info *create_level_infos(int var_count)
{
	bdd_level_info *levels;
	int level_index;

	levels = (bdd_level_info *)malloc(sizeof(bdd_level_info) * var_count);
	for (level_index = 0; level_index < var_count; level_index++)
	{
		levels[level_index].info_head = NULL;
		levels[level_index].info_last = NULL;
	}
	return levels;
}

void free_level_info(bdd_level_info *level)
{
	bdd_node_info *info, *next;

	for (info = level->info_head; info != NULL; info = next)
	{
		next = info->next;
		free(info);
	}
}

void free_level_infos(bdd_level_info *levels, int var_count)
{
	int level_index;

	for (level_index = var_count - 1; level_index >= 0; level_index--)
			free_level_info(&(levels[level_index]));
		free(levels);
}

void bdd_divide_nodes_by_level(bdd_level_info *levels, int level_count, bdd_node *node)
{
	bdd_node_info *new_info;
	bdd_level_info *level;
	
	if (node->var_index >= level_count)
	{
		bdd_report_error("Variable index overflow");
		return;
	}
	
	level = &(levels[node->var_index]);
	new_info = (bdd_node_info *)malloc(sizeof(bdd_node_info));
	new_info->prev = level->info_last;
	new_info->next = NULL;   
	new_info->node = node;
	if (level->info_last != NULL)
		level->info_last->next = new_info;
	level->info_last = new_info;
	if (level->info_head == NULL)
		level->info_head = new_info; 
	// Without info_last member it would be a bit faster, 
	// but it would revert the order. We would like to preserve the order - 
	// especially for the future graphical displaying

	if (!bdd_is_node_terminal_or_null(node->low) && node->low->parent == node)
		bdd_divide_nodes_by_level(levels, level_count, node->low);
	if (!bdd_is_node_terminal_or_null(node->high) && node->high->parent == node &&
		  node->low != node->high)
		bdd_divide_nodes_by_level(levels, level_count, node->high);
}

void bdd_label_nodes_by_levels(bdd_level_info *levels, int level_count)
{
	int level_index;
	bdd_node_info *cur_info;
	char lab = 'a';
	int num = 0;

	for (level_index = 0; level_index < level_count; level_index++)
	{
		for (cur_info = levels[level_index].info_head; cur_info != NULL; cur_info = cur_info->next)
		{
			char *label = cur_info->node->label;

			if (strlen(label) < BDD_MAX_NODE_LABEL_LEN)
				sprintf(label + strlen(label), "%c", lab);
		//	sprintf(label, "%c", lab);
		//	_snprintf(label, BDD_MAX_NODE_LABEL_LEN, "%d%c", num, lab);
			lab++;
			if (lab > 'z')
				lab = 'a';
			num++;
		}
	}
}

int bdd_are_subdiagrams_equal(bdd_node *node1, bdd_node *node2)
{
	if (node1->var_index != node2->var_index)
		return 0;

	if (bdd_is_node_terminal_or_null(node1->low))
	{
		if (node1->low != node2->low)
			return 0;
	}
	else	if (bdd_is_node_terminal_or_null(node2->low))
		return 0;

	if (bdd_is_node_terminal_or_null(node1->high))
	{
		if (node1->high != node2->high)
			return 0;
	}
	else	if (bdd_is_node_terminal_or_null(node2->high))
		return 0;

	if (!bdd_is_node_terminal_or_null(node1->low) &&
		  node1->low != node2->low)
	{
		if (!bdd_are_subdiagrams_equal(node1->low, node2->low))
			return 0;
	}
	if (!bdd_is_node_terminal_or_null(node1->high) &&
		  node1->high != node2->high)
	{
		if (!bdd_are_subdiagrams_equal(node1->high, node2->high))
			return 0;
	}
	return 1;
}

// Compares only one level of nodes subdiagrams
int bdd_are_nodes_equal(bdd_node *node1, bdd_node *node2)
{
	if (node1->var_index != node2->var_index)
		return 0;

	return (node1->low == node2->low) && (node1->high == node2->high);
}

void bdd_unite_equal_subdiagrams(bdd_level_info *levels, bdd_level_info *level)
{
	bdd_node_info *cur_info1, *cur_info2, *next_info;

	if (level->info_head == NULL)
		return;     // Strange (some variable isn't used in the function), but ok

	for (cur_info1 = level->info_head; cur_info1 != NULL && cur_info1->next != NULL; cur_info1 = cur_info1->next)
	{
		for (cur_info2 = cur_info1->next; cur_info2 != NULL; )
		{
			if (cur_info1->node == cur_info2->node)
				bdd_report_error("Node is duplicated at level");   // Some logic error in bdd_divide_nodes_by_level has happened
			if (cur_info1->node->var_index != cur_info2->node->var_index)
				bdd_report_error("Invalid variable index in bdd_unite_equal_subdiagrams");   // Some logic error has happened
#ifdef BDD_VERBOSE_CORRECTNESS_CHECK
			if (bdd_are_nodes_equal(cur_info1->node, cur_info2->node) != 
				  bdd_are_subdiagrams_equal(cur_info1->node, cur_info2->node))
        bdd_report_error("Quick equality check is incorrect");
#endif

			if (bdd_are_nodes_equal(cur_info1->node, cur_info2->node))
			{
				//{
				//	char st[40];
				//	bdd_node_info *cur_info1;
				//
				//	OutputDebugString("Cur:\n");
				//	for (cur_info1 = level->info_head; cur_info1 != NULL; cur_info1 = cur_info1->next)
				//	{
				//		sprintf(st, "%08X - %08X %s\n", 
				//			      cur_info1, cur_info1->node, cur_info1->node->label);
				//		OutputDebugString(st);
				//	}
				//}

				next_info = cur_info2->next;
				bdd_replace_links_at_predecessors(cur_info2->node, cur_info1->node);

				_snprintf(cur_info1->node->label, BDD_MAX_NODE_LABEL_LEN,
					        "%s%s", cur_info1->node->label, cur_info2->node->label);
				cur_info1->node->label[BDD_MAX_NODE_LABEL_LEN] = 0;

				bdd_free_subdiagram(cur_info2->node);
				cur_info2->prev->next = cur_info2->next;
				if (cur_info2->next)
					cur_info2->next->prev = cur_info2->prev;
				if (level->info_last == cur_info2)
					level->info_last = cur_info2->prev;
				free(cur_info2);

				cur_info2 = next_info;
				bdd_on_diagram_changed(7);
			}
			else
				cur_info2 = cur_info2->next;
		}
	}
}

void bdd_reduce(bdd *diagram)
{
#ifdef BDD_VERBOSE_CORRECTNESS_CHECK
	bdd_truth_table *start_truth_table = bdd_create_truth_table(diagram, 1);
	bdd_truth_table *cur_truth_table;
#endif
	bdd_level_info *levels;
	int level_index;

	bdd_remove_redundant_checks(diagram->root_node);

  levels = create_level_infos(diagram->var_count);
	bdd_divide_nodes_by_level(levels, diagram->var_count, diagram->root_node);
	for (level_index = diagram->var_count - 1; level_index >= 0; level_index--)
		bdd_unite_equal_subdiagrams(levels, &(levels[level_index]));

	free_level_infos(levels, diagram->var_count);

	bdd_remove_redundant_checks(diagram->root_node);

#ifdef BDD_VERBOSE_CORRECTNESS_CHECK
	cur_truth_table = bdd_create_truth_table(diagram, 1);
	if (!bdd_are_truth_tables_equal(start_truth_table, cur_truth_table))
		bdd_report_error("Diagrams truth tables are different");
	bdd_free_truth_table(start_truth_table);
	bdd_free_truth_table(cur_truth_table);
#endif
}


/**********************************************************************/
/*** UTILITIES ********************************************************/
/**********************************************************************/

void print_cube(int input_count, t_blif_cube *cube)
{
	int input_index;

	for (input_index = 0; input_index < input_count; input_index++)
		switch (read_cube_variable(cube->signal_status, input_index))
		{
			case LITERAL_DC:
				printf("-");
				break;
			case LITERAL_1:
				printf("1");
				break;
			case LITERAL_0:
				printf("0");
				break;
			case LITERAL_MARKER:
				printf("A");
				break;
			default:
				bdd_report_error("Unexpected cube variable error");
	  }
	printf("\n");
}

int bdd_print_diagram_statistics(bdd *diagram)
{
	// This function is very simple (based on existing code), but very inefficient 

	bdd_level_info *levels = create_level_infos(diagram->var_count);
	int total_node_count = 0;
	int level_index, node_count, var_num;
	bdd_node_info *cur_info;

	bdd_divide_nodes_by_level(levels, diagram->var_count, diagram->root_node);

	printf("Level      ");
	for (level_index = 0; level_index < diagram->var_count; level_index++)
		printf(" %3d", level_index + 1);
	printf("\nVariable   ");
	for (level_index = 0; level_index < diagram->var_count; level_index++)
		printf(" %3d", diagram->var_reordering[level_index] + 1);
	printf("\nNode count ");
	for (level_index = 0; level_index < diagram->var_count; level_index++)
	{
		node_count = 0;
		for (cur_info = levels[level_index].info_head; cur_info != NULL; cur_info = cur_info->next)
			node_count++;

		var_num = diagram->var_reordering[level_index];
		printf(" %3d", node_count);
		total_node_count += node_count;
	}
	printf("\n");

	free_level_infos(levels, diagram->var_count);
	return total_node_count;
}

void print_node_table_index(char **print_buf_cur_ptr_ptr, bdd_node *node)
{
	int id;

	if (node == NULL)
	{
		(*print_buf_cur_ptr_ptr) += sprintf(*print_buf_cur_ptr_ptr, 
					"  -");
		return;
	}
	
	if (bdd_is_node_terminal(node))
	{
		if (bdd_is_terminal_node_1(node))
			id = 1;
		else
			id = 0;
	}
	else
		id = node->table_id;
	assert(id >= 0);
	(*print_buf_cur_ptr_ptr) += sprintf(*print_buf_cur_ptr_ptr, 
					"%3d", id);
}

char *bdd_print_diagram_as_table_to_string(bdd *diagram)
{
	int cur_buf_size = 1024;
	char *print_buf = (char *)malloc(cur_buf_size);
	char *print_buf_cur_ptr, *print_buf_end;
	bdd_level_info *levels = create_level_infos(diagram->var_count);
	int level_index;
	bdd_node_info *cur_info;

	bdd_divide_nodes_by_level(levels, diagram->var_count, diagram->root_node);

	print_buf_cur_ptr = print_buf;
	print_buf_end = print_buf + cur_buf_size;
	print_buf_cur_ptr += _snprintf(print_buf_cur_ptr, print_buf_end - print_buf_cur_ptr, 
				"  u | level | var | low | high\n"
	      "  0 |  %3d  | %3d |     |\n"
	      "  1 |  %3d  | %3d |     |\n", 
				diagram->var_count + 1, diagram->var_count + 1,
				diagram->var_count + 1, diagram->var_count + 1);

	for (level_index = diagram->var_count - 1; level_index >= 0; level_index--)
	{
		for (cur_info = levels[level_index].info_head; cur_info != NULL; cur_info = cur_info->next)
		{
			bdd_node *node = cur_info->node;

			if (print_buf_end - print_buf_cur_ptr < 100)
			{
				print_buf[cur_buf_size - 1] = 0;
				cur_buf_size *= 2;
				print_buf_cur_ptr = (char *)malloc(cur_buf_size);
				strcpy(print_buf_cur_ptr, print_buf);
				free(print_buf);
				print_buf = print_buf_cur_ptr;
				print_buf_cur_ptr += strlen(print_buf);
				print_buf_end = print_buf + cur_buf_size;
			}
			print_buf_cur_ptr += _snprintf(print_buf_cur_ptr, print_buf_end - print_buf_cur_ptr, 
	          "%3d |  %3d  | %3d | ", 
						node->table_id, node->var_index + 1, 
						diagram->var_reordering[node->var_index] + 1);
			print_node_table_index(&print_buf_cur_ptr, node->low);
			print_buf_cur_ptr += _snprintf(print_buf_cur_ptr, print_buf_end - print_buf_cur_ptr, 
						" | ");
			print_node_table_index(&print_buf_cur_ptr, node->high);
			print_buf_cur_ptr += _snprintf(print_buf_cur_ptr, print_buf_end - print_buf_cur_ptr, 
						"\n");
		}
	}

	free_level_infos(levels, diagram->var_count);
	return print_buf;
}

// Fast version - without variables reordering support
void bdd_fill_truth_table_with_1s_no_reordering(bdd_truth_table *truth_table, 
	                                              int shift, int var_index)
{
	int bit_count = 1 << (truth_table->var_count - var_index);
	unsigned int mask;
	int cell_index = shift / (sizeof(unsigned int) * 8);

	assert(shift >= 0 && bit_count >= 1);
	assert(shift + bit_count <= (1 << truth_table->var_count));
	assert(shift % bit_count == 0);
	if (bit_count < sizeof(unsigned int) * 8)
	{
		mask = ((unsigned int)1 << bit_count) - 1;
		truth_table->data[cell_index] |= mask << (shift % (sizeof(unsigned int) * 8));
	}
	else
	  memset(&(truth_table->data[cell_index]), 0xFF, bit_count / 8);

	//int bit_index;

	//for (bit_index = shift + (1 << (truth_table->var_count - node->var_index - 1)) - 1; 
	//		 bit_index >= shift; 
	//		 bit_index--)
	//	set_tru
}

//int get_high_child(int cur_var_index, int node_var_index)

void bdd_fill_truth_table_no_reordering(bdd_truth_table *truth_table,
	                                      bdd_node *node, int shift, int var_index)
{
	assert(var_index <= truth_table->var_count);
	if (bdd_is_node_terminal_or_null(node))
	{
		if (bdd_is_terminal_node_1(node))
			bdd_fill_truth_table_with_1s_no_reordering(truth_table, shift, var_index);
	}
	else
	{
		int high_shift = shift + (1 << (truth_table->var_count - var_index - 1));

		assert(node->var_index >= var_index);
		if (node->var_index == var_index)
		{
			bdd_fill_truth_table_no_reordering(truth_table, node->low, shift, var_index + 1);
  		bdd_fill_truth_table_no_reordering(truth_table, node->high, high_shift, var_index + 1);
		}
		else
		{
			bdd_fill_truth_table_no_reordering(truth_table, node, shift, var_index + 1);
  		bdd_fill_truth_table_no_reordering(truth_table, node, high_shift, var_index + 1);
		}
	}
}

void bdd_fill_truth_table_with_1s(bdd_truth_table *truth_table, 
	                                int shift, int var_index)
{
	assert(var_index <= truth_table->var_count);
	if (var_index == truth_table->var_count)
	{
		int cell_index = shift / (sizeof(unsigned int) * 8);

		assert(shift >= 0 && shift < (1 << truth_table->var_count));
		truth_table->data[cell_index] |= 1 << (shift % (sizeof(unsigned int) * 8));
	}
	else
	{
		int source_var_index = truth_table->var_reordering[var_index];
		int high_shift = shift + (1 << (truth_table->var_count - source_var_index - 1));

		bdd_fill_truth_table_with_1s(truth_table, shift, var_index + 1);
		bdd_fill_truth_table_with_1s(truth_table, high_shift, var_index + 1);
	}
}

void bdd_fill_truth_table(bdd_truth_table *truth_table,
	                        bdd_node *node, int shift, int var_index)
{
	assert(var_index <= truth_table->var_count);
	if (bdd_is_node_terminal_or_null(node))
	{
		if (bdd_is_terminal_node_1(node))
			bdd_fill_truth_table_with_1s(truth_table, shift, var_index);
	}
	else
	{
		int source_var_index = truth_table->var_reordering[var_index];
		int high_shift = shift + (1 << (truth_table->var_count - source_var_index - 1));

		assert(node->var_index >= var_index);
		if (node->var_index == var_index)
		{
			bdd_fill_truth_table(truth_table, node->low, shift, var_index + 1);
  		bdd_fill_truth_table(truth_table, node->high, high_shift, var_index + 1);
		}
		else
		{
			bdd_fill_truth_table(truth_table, node, shift, var_index + 1);
  		bdd_fill_truth_table(truth_table, node, high_shift, var_index + 1);
		}
	}
}

bdd_truth_table *bdd_create_truth_table(bdd *diagram, int consider_reordering)
{
	bdd_truth_table *truth_table = (bdd_truth_table *)malloc(sizeof(bdd_truth_table));
	int data_len = 1 << (diagram->var_count - 3);

	truth_table->var_count = diagram->var_count;
	if (data_len < sizeof(unsigned int))
		data_len = sizeof(unsigned int);
	truth_table->data = (unsigned int *)malloc(data_len);
	if (truth_table->data == NULL)
	{
		bdd_report_error("Not enough memory for truth table");
		return NULL;
	}
	memset(truth_table->data, 0, data_len);
	if (consider_reordering)
	{
		truth_table->var_reordering = NULL;
		truth_table->var_reordering = (int *)malloc(sizeof(int) * diagram->var_count);
		memcpy(truth_table->var_reordering, diagram->var_reordering, 
			     sizeof(int) * diagram->var_count);
		bdd_fill_truth_table(truth_table, diagram->root_node, 0, 0);
	}
	else
	{
		truth_table->var_reordering = NULL;
		bdd_fill_truth_table_no_reordering(truth_table, diagram->root_node, 0, 0);
	}

	return truth_table;
}

void bdd_free_truth_table(bdd_truth_table *truth_table)
{
	free(truth_table->data);
	free(truth_table);
}

int bdd_are_truth_tables_equal(bdd_truth_table *truth_table1, bdd_truth_table *truth_table2)
{
	int data_len = 1 << (truth_table1->var_count - 3);

	if (truth_table1->var_count != truth_table2->var_count)
	  return 0;
	if (data_len < sizeof(unsigned int))
		data_len = sizeof(unsigned int);
	return memcmp(truth_table1->data, truth_table2->data, data_len) == 0;
}

int bdd_check_diagrams_truth_tables_equal(bdd *diagram1, bdd *diagram2)
{
	bdd_truth_table *truth_tables[2] = { bdd_create_truth_table(diagram1, 1),
		                                   bdd_create_truth_table(diagram2, 1) };
	int are_equal = bdd_are_truth_tables_equal(truth_tables[0], truth_tables[1]);
	
	if (!are_equal)
		bdd_report_error("Diagrams truth tables are different");
	bdd_free_truth_table(truth_tables[0]);
	bdd_free_truth_table(truth_tables[1]);
	return are_equal;
}

void bdd_set_diagram_changed_callback(bdd_callback_ptr callback_ptr, int min_importance_level)
{
	g_diagram_changed_callback_ptr = callback_ptr;
	g_min_importance_level_for_callback = min_importance_level;
}

void bdd_on_diagram_changed(int importance_level)
{
	if (g_diagram_changed_callback_ptr != NULL && 
		  importance_level >= g_min_importance_level_for_callback)
		(*g_diagram_changed_callback_ptr)();
}


void bdd_set_cur_diagram(bdd *diagram)
{
	g_cur_printing_diagram = diagram;
}

void bdd_print_cur_diagram_as_table()
{
	if (g_cur_printing_diagram != NULL)
	{
		char *str =	bdd_print_diagram_as_table_to_string(g_cur_printing_diagram);

		if (g_prev_printed_diagram_str != NULL && strcmp(g_prev_printed_diagram_str, str) == 0)
		{
			//	printf("repeat");
			free(str);
		}
		else
		{
			printf("%s\n", str);

			if (g_prev_printed_diagram_str != NULL)
				free(g_prev_printed_diagram_str);
			g_prev_printed_diagram_str = str;
		}
	}
}

