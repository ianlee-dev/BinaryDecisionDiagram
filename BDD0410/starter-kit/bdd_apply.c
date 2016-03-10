//#include <stdlib.h>
#include <string.h>
//#include <assert.h>
//#include <malloc.h>
#include "bdd_apply.h"
#include "bdd_apply_hash.h"
#include "bdd_tools.h"

#pragma warning(disable: 4996) 

//bdd_root_node *bdd_create_apply_result(bdd *left_diagram, apply_operation_type operation,
//	                                     bdd *right_diagram, int use_hash, int var_count)
//{
//	bdd  *new_diagram_root = bdd_create_node(0);
//
//	bdd_add_apply_result(new_diagram_root, left_diagram_root, operation,
//		                   right_diagram_root, use_hash, var_count);
//	return new_diagram_root;
//}

void bdd_add_apply_result(bdd *target_diagram, 
	                        bdd *left_diagram, apply_operation_type operation,
	                        bdd *right_diagram, int use_hash, int var_count)
{
	apply_hash_table *hash_table = NULL;

	if (use_hash)
	{
		int size = 97;

		if (var_count > 22)
			size = 25165843;
		else  if (var_count > 15)
			size = 98317; 
		else if (var_count > 8)
			size = 769;

		hash_table = apply_create_hash_table(size);
		if (hash_table == NULL)
			return;
	}

	bdd_do_apply(target_diagram, target_diagram->root_node, -1, 
		           left_diagram->root_node, operation, right_diagram->root_node, hash_table);
	if (hash_table != NULL)
		apply_free_hash_table(hash_table);
}

void bdd_apply_unite_labels(bdd_node *target_node, bdd_node *left_node, bdd_node *right_node)
{
	if (bdd_is_node_terminal(left_node))
		sprintf(target_node->label, "%d", bdd_is_terminal_node_1(left_node));
	else
	{
		strcpy(target_node->label, left_node->label);
		target_node->label[BDD_MAX_NODE_LABEL_LEN / 2] = 0;
	}

  if (bdd_is_node_terminal(right_node))
		_snprintf(target_node->label, BDD_MAX_NODE_LABEL_LEN,
					    "%s+%d", target_node->label, bdd_is_terminal_node_1(right_node));
	else
		_snprintf(target_node->label, BDD_MAX_NODE_LABEL_LEN,
					    "%s+%s", target_node->label, right_node->label);
  target_node->label[BDD_MAX_NODE_LABEL_LEN] = 0;
}

void bdd_do_apply(bdd *target_diagram, bdd_node *target_parent, int target_high,
	                bdd_node *left_node, apply_operation_type operation,
	                bdd_node *right_node, apply_hash_table *hash_table)
{
	int min_var_index; 
	bdd_node *new_node, *left_child_nodes[2], *right_child_nodes[2];
	int child_high;

	if (bdd_is_node_terminal(left_node) && bdd_is_node_terminal(right_node))
	{
		int value = apply_operation_result(bdd_is_terminal_node_1(left_node), operation,
				    											     bdd_is_terminal_node_1(right_node));

		bdd_add_terminal_child(target_parent, target_high, value);
		return;
	}

	if (left_node == NULL || right_node == NULL)
		return;

	if (hash_table)
	{
		bdd_node *existing_node = apply_hash_search(hash_table, left_node, right_node);

		if (existing_node != NULL)
		{
			bdd_add_child_node(target_parent, target_high, existing_node);
			return;
		}
	}

	if (!bdd_is_node_terminal(left_node))
	{
		min_var_index = left_node->var_index;
		if (!bdd_is_node_terminal(right_node) && min_var_index > right_node->var_index)
			min_var_index = right_node->var_index;
	}
	else  
		min_var_index = right_node->var_index;

  if (!bdd_is_node_terminal(left_node) && left_node->var_index == min_var_index)
	{
		left_child_nodes[0] = left_node->low;
		left_child_nodes[1] = left_node->high;
	}
	else
	{
		left_child_nodes[0] = left_node;
		left_child_nodes[1] = left_node;
	}

  if (!bdd_is_node_terminal(right_node) && right_node->var_index == min_var_index)
	{
		right_child_nodes[0] = right_node->low;
		right_child_nodes[1] = right_node->high;
	}
	else
	{
		right_child_nodes[0] = right_node;
		right_child_nodes[1] = right_node;
	}

	if (target_high < 0)
		new_node = target_parent;   // We create diagram root node sooner
	else
		new_node = bdd_create_child_node(target_diagram,
		      target_parent, target_high, min_var_index);
	bdd_apply_unite_labels(new_node, left_node, right_node);

	for (child_high = 0; child_high < 2; child_high++)
	{
		bdd_do_apply(target_diagram, new_node, child_high,
				         left_child_nodes[child_high], operation,
	               right_child_nodes[child_high], hash_table);
		bdd_on_diagram_changed(6);
	}

	if (hash_table)
		apply_hash_insert(hash_table, left_node, right_node, new_node);
}

int apply_operation_result(int left, apply_operation_type operation, int right)
{
	switch (operation)
	{
		case apply_operation_and:  return left && right;
	  case apply_operation_or:   return left || right;
		case apply_operation_xor:  return left != right;
		default:              		 bdd_report_error("Unknown operation");
	}
	return -1;
}