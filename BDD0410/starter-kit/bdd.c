#include <malloc.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "bdd.h"

#pragma warning(disable: 4996) 

#define BDD_MAX_DIAGRAM_HEIGHT 2048

void bdd_on_diagram_changed(int level);  // TODO? move this and bdd_set_diagram_changed_callback to bdd.*

typedef struct bdd_node_predec_link
{
  bdd_node *predec_node;
	struct bdd_node_predec_link *next;
} bdd_node_predec_link;

// Information about node that has to be connected in the cloned diagram, 
// but couldn't during the main pass
typedef struct clone_delayed_link
{
	char *path;
	bdd_node *target_parent;
	int is_high;
	struct clone_delayed_link *next;
} clone_delayed_link;

typedef struct 
{
	bdd *target_diagram;
	bdd_root_node *target_diagram_root;
	int copy_table_ids;
	char path_buffer[BDD_MAX_DIAGRAM_HEIGHT + 1];
	clone_delayed_link *delayed_links_head;
} clone_state;

char g_path_buffer[BDD_MAX_DIAGRAM_HEIGHT + 1];
bdd_error_callback_ptr g_error_callback_ptr = NULL;


bdd *bdd_create_diagram(int var_count)
{
	bdd *diagram = (bdd *)malloc(sizeof(bdd));
	int i;

	diagram->max_table_id = 1;
	diagram->var_count = var_count;
	diagram->var_reordering = (int *)malloc(sizeof(int) * var_count);
	for (i = 0; i < var_count; i++)
		diagram->var_reordering[i] = i;
	diagram->root_node = bdd_create_node(diagram, 0);
	return diagram;
}

void bdd_free_diagram(bdd *diagram)
{
	bdd_free_subdiagram(diagram->root_node);
	free(diagram->var_reordering);
	free(diagram);
}

void bdd_init_node(bdd_node *node, int var_index, int table_id)
{
	node->parent = NULL;
	node->var_index = var_index;
	node->low = NULL;
	node->high = NULL;
	node->predecs_head = NULL;
  node->label[0] = 0;
	node->table_id = table_id;
}

//void bdd_init_diagram(bdd_root_node *root_node)
//{
//	bdd_init_node(root_node, 0);
//}

bdd_node *bdd_create_node(bdd *diagram, int var_index)
{
	bdd_node *node = (bdd_node *)malloc(sizeof(bdd_node));

	if (node == NULL)
	{
		bdd_report_error("Not enough memory for node");
		return NULL;
	}
	diagram->max_table_id++;
	bdd_init_node(node, var_index, diagram->max_table_id);
	return node;
}

bdd_node *bdd_create_child_node(bdd *diagram, bdd_node *parent, int high, int var_index)
{
	bdd_node *node = (bdd_node *)malloc(sizeof(bdd_node));

	if (node == NULL)
	{
		bdd_report_error("Not enough memory for node");
		return NULL;
	}
	diagram->max_table_id++;
	bdd_init_node(node, var_index, diagram->max_table_id);
	bdd_add_child_node(parent, high, node);
	return node;
}

void bdd_free_node(bdd_node *node)
{
	if (node->parent != NULL)
		bdd_report_error("Trying to free linked node");
	else  if (node->predecs_head != NULL)
		bdd_report_error("Trying to free node with predecessors");
	else  if (!bdd_is_node_terminal_or_null(node->low) || 
		        !bdd_is_node_terminal_or_null(node->high))  
		bdd_report_error("Trying to free node with children");
	else
	  free(node);
}


int is_predec_in_list(bdd_node_predec_link *predecs_head, bdd_node *node)
{
	bdd_node_predec_link *link;

	for (link = predecs_head; link != NULL; link = link->next)
		if (link->predec_node == node)
			return 1;
	return 0;
}

void check_predecs_validity(bdd_node *node)
{
#ifdef BDD_VERBOSE_CORRECTNESS_CHECK
	bdd_node_predec_link *link;

	for (link = node->predecs_head; link != NULL; link = link->next)
	{
		bdd_node *predec_node = link->predec_node;
		if (predec_node == NULL || 
			  (link->next && predec_node == link->next->predec_node) ||
			  predec_node->parent == (bdd_node *)0xfeeefeee)
			bdd_report_error("Corrupted link");
		if (predec_node->low != node && predec_node->high != node)
			bdd_report_error("Invalid link to predecessor");
	}
#endif
}

// Removes predec_node from node's predec_head list. Parent must not be == predec_node
void remove_node_predec_from_list(bdd_node *node, bdd_node *predec_node)
{
	bdd_node_predec_link *link, *prev_link;

	prev_link = NULL;
	for (link = node->predecs_head; link != NULL; link = link->next)
		if (link->predec_node == predec_node)
		{
			if (prev_link == NULL)
				node->predecs_head = link->next;
			else
				prev_link->next = link->next;
			free(link);
			check_predecs_validity(node);
			return;
		}
		else
			prev_link = link;
	bdd_report_error("Invalid node predecessor in remove_node_predec_from_list");
}

void convert_first_predec_to_parent(bdd_node *node)
{
	bdd_node_predec_link *link = node->predecs_head;

	node->parent = link->predec_node;
	node->predecs_head = link->next;
  free(link);

	check_predecs_validity(node);
}

// Remove link to a precedessor from the node's structure
void remove_node_predecessor(bdd_node *node, bdd_node *predec_node)
{
  if (node->parent == predec_node)
	{
		if (node->predecs_head == NULL)
			node->parent = NULL;
		else
			convert_first_predec_to_parent(node);
	}
	else
		remove_node_predec_from_list(node, predec_node);

	check_predecs_validity(node);
}

void add_node_predecessor(bdd_node *node, bdd_node *predec_node)
{
	if (is_predec_in_list(node->predecs_head, predec_node))
	{
		bdd_report_error("Duplicated predecessor");   // Logic error detected
		return;
	}

  if (node->parent == NULL)
		node->parent = predec_node;
	else
	{
		bdd_node_predec_link *new_link = (bdd_node_predec_link *)malloc(sizeof(bdd_node_predec_link));

		new_link->predec_node = predec_node;
		new_link->next = node->predecs_head;
		node->predecs_head = new_link;
	}
}

// Removes link to the parent in the child node's structure
// and frees child and its subdiagram if there is no more links to it
void detach_node_child(bdd_node *parent_node, bdd_node *node)
{
  if (node->parent == parent_node)
	{
		if (node->predecs_head == NULL)
		{
			node->parent = NULL;
			bdd_free_subdiagram(node);
		}
		else
			convert_first_predec_to_parent(node);
	}
	else
		remove_node_predec_from_list(node, parent_node);
}

void bdd_free_subdiagram(bdd_node *node)
{
	if (bdd_is_node_terminal_or_null(node))
		bdd_report_error("Trying to free empty or terminal node structure");
	if (node->parent != NULL || node->predecs_head != NULL)
		bdd_report_error("Trying to free subdiagram with predecessors");

	if (!bdd_is_node_terminal_or_null(node->low))
		detach_node_child(node, node->low); 
	if (node->high != node->low && !bdd_is_node_terminal_or_null(node->high))
		detach_node_child(node, node->high); 
	node->low = (bdd_node *)0xBadBad;    // Just in case of error when we'll see the deleted structure
	free(node);
}


void bdd_add_terminal_child(bdd_node *node, int to_high, int value)
{
	if ((to_high && node->high != NULL) ||
		  (!to_high && node->low != NULL))
		bdd_report_error("Overwriting existing child");   // Overwriting of existing children must be made manually
	if (value < 0 || value > 1)
		bdd_report_error("Invalid terminal node value");

	if (to_high)
		node->high = (bdd_node *)(BDD_TERMINAL_NODE_0 + value);
	else
		node->low = (bdd_node *)(BDD_TERMINAL_NODE_0 + value);
	bdd_on_diagram_changed(0);
}

void bdd_add_child_node(bdd_node *node, int to_high, bdd_node *child_node)
{
	if ((to_high && node->high != NULL) ||
		  (!to_high && node->low != NULL))
		bdd_report_error("Overwriting existing child");

	if (to_high)
		node->high = child_node;
	else
		node->low = child_node;

	if (!bdd_is_node_terminal_or_null(child_node) && node->low != node->high)
		add_node_predecessor(child_node, node);
	bdd_on_diagram_changed(1);
}

void bdd_replace_child_node(bdd_node *node, int to_high, bdd_node *child_node)
{
	if (to_high)
	{
		if (node->high == child_node)
			return;     // Main purpose of this is to not let child's parent changing
		              // and keep cloning of diagrams with multiple precedessors functioning

		if (!bdd_is_node_terminal_or_null(node->high) && node->low != node->high)
			remove_node_predecessor(node->high, node);
		node->high = child_node;
	}
	else
	{
		if (node->low == child_node)
			return;

		if (!bdd_is_node_terminal_or_null(node->low) && node->low != node->high)
			remove_node_predecessor(node->low, node);
		node->low = child_node;
	}

	if (!bdd_is_node_terminal_or_null(child_node) && node->low != node->high)
		add_node_predecessor(child_node, node);
	bdd_on_diagram_changed(2);
}

void bdd_replace_child_links(bdd_node *changing_node,
	                           bdd_node *from_node, bdd_node *to_node)
{
	int low_child_replaced = 0;

	if (changing_node->low == from_node)
	{
    if (changing_node->high != from_node)
		{
			if (!bdd_is_node_terminal_or_null(from_node))
				remove_node_predecessor(from_node, changing_node);
			if (!bdd_is_node_terminal_or_null(to_node) && changing_node->high != to_node)
				add_node_predecessor(to_node, changing_node);
		}
		changing_node->low = to_node;
		low_child_replaced = 1;
	}

	if (changing_node->high == from_node)
	{
		if (!bdd_is_node_terminal_or_null(from_node))
			remove_node_predecessor(from_node, changing_node);
		if (!bdd_is_node_terminal_or_null(to_node) &&
			  (low_child_replaced || changing_node->low != to_node))
			add_node_predecessor(to_node, changing_node);
		changing_node->high = to_node;
	}	

	if (!bdd_is_node_terminal_or_null(from_node))
		check_predecs_validity(from_node);
	if (!bdd_is_node_terminal_or_null(to_node))
		check_predecs_validity(to_node);

	bdd_on_diagram_changed(2);
	// Here the diagram is not in fully consistent state, so if it is cloned,
	// the "Couldn't find equal node during cloning" error can be generated.
	// Because changing_node's parent is already set to NULL, but child nodes
	// refer to changing_node
}

void bdd_replace_links_at_predecessors(bdd_node *from_node, bdd_node *to_node)
{
	while (from_node->predecs_head != NULL)
		bdd_replace_child_links(from_node->predecs_head->predec_node, from_node, to_node);
	if (from_node->parent != NULL)
		bdd_replace_child_links(from_node->parent, from_node, to_node);

	check_predecs_validity(from_node);
	if (!bdd_is_node_terminal_or_null(to_node))
		check_predecs_validity(to_node);
}

void bdd_replace_node_var_index(bdd_node *node, int new_var_index)
{
	if (node->parent && new_var_index <= node->parent->var_index)
		bdd_report_error("Invalid new node variable");
	node->var_index = new_var_index;
	bdd_on_diagram_changed(3);
}


int bdd_is_node_terminal(bdd_node *node)
{ 
	return (int)node == BDD_TERMINAL_NODE_0 ||
		     (int)node == BDD_TERMINAL_NODE_1;
}

int bdd_is_node_terminal_or_null(bdd_node *node)
{ 
	return node == NULL ||
		     (int)node == BDD_TERMINAL_NODE_0 ||
		     (int)node == BDD_TERMINAL_NODE_1;
}

int bdd_is_terminal_node_1(bdd_node *node)
{
	return (int)node == BDD_TERMINAL_NODE_1;
}


bdd_root_node *bdd_get_node_diagram(bdd_node *node)
{
	while (node->parent)
		node = node->parent;
	return node;
}

bdd_node *bdd_get_node_by_path(bdd_node *root, char *path)
{
	bdd_node *cur_node = root;
	bdd_node *next_node;
	int i;

	for (i = root->var_index; path[i] != 0; )
	{
		if (path[i] == '0')
			next_node = cur_node->low;
		else 
			next_node = cur_node->high;

		if (bdd_is_node_terminal_or_null(next_node))
		{
			return NULL;
		}
		if (next_node->var_index <= cur_node->var_index)
		{
			bdd_report_error("Incorrect variable index");
			return NULL;
		}
		i += next_node->var_index - cur_node->var_index;
		cur_node = next_node;
	}
	return cur_node;
}

void bdd_get_node_full_path(char *path_result, bdd_node *node)
{
	int cur_var_index = node->var_index;
	bdd_node *cur_node = node;
	int i;

	if (cur_var_index >= BDD_MAX_DIAGRAM_HEIGHT)
	{
		bdd_report_error("Diagram height is too big");
		path_result[0] = 0;
		return;
	}

	memset(path_result, '0', cur_var_index);
	path_result[cur_var_index] = 0;
	for (cur_node = node; cur_node->parent != NULL; cur_node = cur_node->parent)
	{
		bdd_node *parent = cur_node->parent;

		if (parent->high == cur_node)
		{
			for (i = parent->var_index; i < cur_node->var_index; i++)
				path_result[i] = '1';
		}
		else  if (parent->low != cur_node)
			bdd_report_error("Node is not among its parent's children");
	}

	for (i = 0; i < cur_node->var_index; i++)
		path_result[i] = '1';
}

bdd_node *bdd_get_equal_node(bdd_root_node *search_diagram, bdd_node *node)
{
	bdd_node *equal_node;

	bdd_get_node_full_path(g_path_buffer, node);
	equal_node = bdd_get_node_by_path(search_diagram, g_path_buffer);
	return equal_node;
}

bdd_node *clone_get_equal_node(clone_state *state, bdd_node *child_node, 
	                             bdd_node *target_parent, int is_high)
{
	bdd_node *new_equal_node;

	bdd_get_node_full_path(state->path_buffer, child_node);
	new_equal_node = bdd_get_node_by_path(state->target_diagram_root, state->path_buffer);
	if (new_equal_node == NULL)
	{
		// We can't find and thus connect the node now since the corresponding node isn't cloned yet.
		// We'll connect it after the cloning is mainly finished

		clone_delayed_link *delayed_link = (clone_delayed_link *)malloc(sizeof(clone_delayed_link));

		delayed_link->path = strdup(state->path_buffer);
		delayed_link->target_parent = target_parent;
		delayed_link->is_high = is_high;		
		delayed_link->next = state->delayed_links_head;
		state->delayed_links_head = delayed_link;
	}
	return new_equal_node;
}

void clone_connect_rest_nodes(clone_state *state)
{
	clone_delayed_link *delayed_link, *next;
	bdd_node *new_equal_node;

	for (delayed_link = state->delayed_links_head; delayed_link != NULL; delayed_link = next)
	{
		new_equal_node = bdd_get_node_by_path(state->target_diagram_root, delayed_link->path);
		if (new_equal_node == NULL)
			bdd_report_error("Couldn't find equal node during cloning");
		bdd_add_child_node(delayed_link->target_parent, delayed_link->is_high, 
						           new_equal_node);

		next = delayed_link->next;
		free(delayed_link->path);
		free(delayed_link);
	}
}

bdd *bdd_clone_diagram(bdd *diagram)
{
	bdd *new_diagram = bdd_create_diagram(diagram->var_count);

	memcpy(new_diagram->var_reordering, diagram->var_reordering, 
				 sizeof(diagram->var_reordering[0]) * diagram->var_count);

	bdd_free_node(new_diagram->root_node);
	new_diagram->max_table_id--;
	new_diagram->root_node = NULL;
	new_diagram->root_node = bdd_clone_subdiagram(new_diagram, diagram->root_node, 1);
	return new_diagram;
}

bdd_node *bdd_clone_node(bdd *diagram, bdd_node *node)
{
	bdd_node *new_node = bdd_create_node(diagram, node->var_index);

	bdd_add_child_node(new_node, 0, node->low);
	bdd_add_child_node(new_node, 1, node->high);
	_snprintf(new_node->label, BDD_MAX_NODE_LABEL_LEN, 
		        "%s2", node->label);
	new_node->label[BDD_MAX_NODE_LABEL_LEN] = 0;
	return new_node;
}

void bdd_clone_child_subdiagram(clone_state *state, bdd_node *new_node, 
	                              int is_high, bdd_node *node)
{
	bdd_node *child_node;
	bdd_node *new_child_node;
	
	if (is_high)
		child_node = node->high;
	else
		child_node = node->low;

	if (bdd_is_node_terminal_or_null(child_node))
		new_child_node = child_node;
	else
	{
		if (child_node->parent == node)
		{
			if (is_high && node->low == node->high)
				new_child_node = new_node->low;
			else
			{
				new_child_node = bdd_create_child_node(state->target_diagram, 
							new_node, is_high, child_node->var_index);
				strcpy(new_child_node->label, child_node->label);
				if (state->copy_table_ids)
					new_child_node->table_id = child_node->table_id;

       	bdd_clone_child_subdiagram(state, new_child_node, 0, child_node);
	      bdd_clone_child_subdiagram(state, new_child_node, 1, child_node);
				new_child_node = NULL;    // Already attached
			}
		}
		else
		{
			// We must not simply clone subdiagram since in the source diagram
			// there are multiple predecessors of child_node

			new_child_node = clone_get_equal_node(state, child_node, 
				                                    new_node, is_high);
		}
	}

	if (new_child_node)
		bdd_add_child_node(new_node, is_high, new_child_node);
}

bdd_node *bdd_clone_subdiagram(bdd *target_diagram, bdd_node *node, int copy_table_ids)
{
	bdd_node *new_node = bdd_create_node(target_diagram, node->var_index);
	clone_state state;

	if (target_diagram->root_node == NULL)
		target_diagram->root_node = new_node;

	state.target_diagram = target_diagram;
	state.target_diagram_root = target_diagram->root_node;
	state.copy_table_ids = copy_table_ids;
	state.delayed_links_head = NULL;

	strcpy(new_node->label, node->label);
	bdd_clone_child_subdiagram(&state, new_node, 0, node);
	bdd_clone_child_subdiagram(&state, new_node, 1, node);
	clone_connect_rest_nodes(&state);

#ifdef _DEBUG     // Additional logic correctness check
	//if (bdd_get_node_diagram(new_node) == bdd_get_node_diagram(node) || 
	//	  (!bdd_is_node_terminal_or_null(new_node->low) && 
	//	   bdd_get_node_diagram(new_node->low) != target_diagram_root) ||
	//		(!bdd_is_node_terminal_or_null(new_node->high) && 
	//	   bdd_get_node_diagram(new_node->high) != target_diagram_root))
	//	 bdd_report_error("Subdiagram cloning error");
#endif
	return new_node;
}

int bdd_is_predecessor_single(bdd_node *node)
{
	return node->predecs_head == NULL;
}


void bdd_report_error(const char *err)
{
	printf("Error: %s\n", err);
	if (g_error_callback_ptr)
		(*g_error_callback_ptr)(err);
}

void bdd_set_error_callback(bdd_error_callback_ptr callback_ptr)
{
	g_error_callback_ptr = callback_ptr;
}