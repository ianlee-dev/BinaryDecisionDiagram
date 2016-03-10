//#include <stdlib.h>
#include <string.h>
#include <assert.h>
//#include <malloc.h>
#include <windows.h>
#include "bdd_sifting.h"
#include "bdd_tools.h"

/**********************************************************************/
/*** SIFTING SERVICE DATA STRUCTURES DECLARATIONS *********************/
/**********************************************************************/

typedef struct 
{
	// Source data
	bdd *diagram;

	// Resulting data
	int *best_var_positions;
	bdd_truth_table *start_truth_table;
	int node_counts[20][20];       // Human-readable table of results
	int swap_count;

	// Temporary data
	int moving_var_num;
	int moving_var_best_position;
	int min_node_count, cur_node_count;
} sifting_state;


/**********************************************************************/
/*** DIAGRAM TRANSFORMATION METHODS ***********************************/
/**********************************************************************/

void bdd_swap_variables_at_node(bdd *diagram, bdd_node *top_node)
{
  bdd_node *bottom_nodes[4], *new_middle_nodes[2];

	if ((top_node->low == top_node->high && bdd_is_node_terminal_or_null(top_node->low)) ||                 
			(!bdd_is_node_terminal_or_null(top_node->low) && 
			 top_node->low->var_index > top_node->var_index + 1 &&
			 !bdd_is_node_terminal_or_null(top_node->high) &&
			 top_node->high->var_index > top_node->var_index + 1))
	{
		bdd_replace_node_var_index(top_node, top_node->var_index + 1);     // Case d
		bdd_on_diagram_changed(4);
		return;
	}

	if (bdd_is_node_terminal_or_null(top_node->low) || 
		  top_node->low->var_index > top_node->var_index + 1)
	{
		bottom_nodes[0] = top_node->low;
		bottom_nodes[1] = top_node->low;
		new_middle_nodes[0] = NULL;
	//	new_middle_nodes[0] = bdd_create_child_node(top_node)
	}
	else
	{
		bottom_nodes[0] = top_node->low->low;
		bottom_nodes[1] = top_node->low->high;

		if (bdd_is_predecessor_single(top_node->low))
			new_middle_nodes[0] = top_node->low;
		else
		{
			new_middle_nodes[0] = bdd_clone_node(diagram, top_node->low);
			bdd_replace_child_node(top_node, 0, new_middle_nodes[0]);
		}
		//if (top_node->low->parent == top_node && top_node->low->predecs_head == NULL)
		//	new_middle_nodes[0] = top_node->low;    // We can modify it
		//else
		//	new_middle_nodes[0] = NULL;
	}

	if (bdd_is_node_terminal_or_null(top_node->high) || 
		  top_node->high->var_index > top_node->var_index + 1)
	{
		bottom_nodes[2] = top_node->high;
		bottom_nodes[3] = top_node->high;
		new_middle_nodes[1] = NULL;
	}
	else
	{
		bottom_nodes[2] = top_node->high->low;
		bottom_nodes[3] = top_node->high->high;
		if (bdd_is_predecessor_single(top_node->high))
			new_middle_nodes[1] = top_node->high;
		else
		{
			new_middle_nodes[1] = bdd_clone_node(diagram, top_node->high);
			bdd_replace_child_node(top_node, 1, new_middle_nodes[1]);
		}
	}

	if (bottom_nodes[0] == bottom_nodes[2])
	{
		if (new_middle_nodes[0] != NULL)
		{
			if (new_middle_nodes[0] == new_middle_nodes[1])
			{
				new_middle_nodes[1] = bdd_clone_node(diagram, top_node->high);
				bdd_replace_child_node(top_node, 1, new_middle_nodes[1]);
			}
			bdd_replace_child_node(new_middle_nodes[0], 1, bottom_nodes[2]);
		}
	}
	else
	{
		if (new_middle_nodes[0] == NULL)
		{
			bdd_replace_child_node(top_node, 0, NULL);
			new_middle_nodes[0] = bdd_create_child_node(diagram,
				    top_node, 0, top_node->var_index + 1);
			bdd_add_child_node(new_middle_nodes[0], 0, bottom_nodes[0]);
		}

		bdd_replace_child_node(new_middle_nodes[0], 1, bottom_nodes[2]);
	}

	if (bottom_nodes[1] == bottom_nodes[3])
	{
		if (new_middle_nodes[1] != NULL)
		{
			if (new_middle_nodes[0] == new_middle_nodes[1])
			{
				new_middle_nodes[1] = bdd_clone_node(diagram, top_node->high);
				bdd_replace_child_node(top_node, 1, new_middle_nodes[1]);
			}
			bdd_replace_child_node(new_middle_nodes[1], 0, bottom_nodes[1]);
		}
	}
	else
	{
		if (new_middle_nodes[1] == NULL)
		{
			bdd_replace_child_node(top_node, 1, NULL);
			new_middle_nodes[1] = bdd_create_child_node(diagram, 
						top_node, 1, top_node->var_index + 1);
			bdd_add_child_node(new_middle_nodes[1], 1, bottom_nodes[3]);
		}

		bdd_replace_child_node(new_middle_nodes[1], 0, bottom_nodes[1]);
	}

	bdd_on_diagram_changed(4);
}

void bdd_swap_variables_without_node_at_top_level(bdd_level_info *middle_level, int first_var_index)
{
	bdd_node_info *cur_info;
	int changed = 0;

	for (cur_info = middle_level->info_head; cur_info != NULL; cur_info = cur_info->next)
		if (cur_info->node->parent == NULL ||
			  cur_info->node->parent->var_index < first_var_index)
		{
			bdd_replace_node_var_index(cur_info->node, first_var_index);   // Case e
			changed++;
		}
	if (changed)
		bdd_on_diagram_changed(5);
}

void bdd_swap_variables(bdd *diagram, int first_var_index)
{
	bdd_level_info *levels = create_level_infos(diagram->var_count);
	bdd_node_info *cur_info;
	int prev_var_num;

	bdd_divide_nodes_by_level(levels, diagram->var_count, diagram->root_node);
	for (cur_info = levels[first_var_index].info_head; cur_info != NULL; cur_info = cur_info->next)
		bdd_swap_variables_at_node(diagram, cur_info->node);

  bdd_swap_variables_without_node_at_top_level(&(levels[first_var_index + 1]), first_var_index);
	free_level_infos(levels, diagram->var_count);

	prev_var_num = diagram->var_reordering[first_var_index];
	diagram->var_reordering[first_var_index] = diagram->var_reordering[first_var_index + 1];
	diagram->var_reordering[first_var_index + 1] = prev_var_num;
}


/**********************************************************************/
/*** SIFTING ALGORITHM ************************************************/
/**********************************************************************/

void init_sifting_state(sifting_state *state, bdd *diagram)
{
	int i;

	state->diagram = diagram;
	state->best_var_positions = (int *)malloc(sizeof(int) * diagram->var_count);
	for (i = 0; i < diagram->var_count; i++)
		state->best_var_positions[i] = -1;
	state->swap_count = 0;

	memset(state->node_counts, -1, sizeof(state->node_counts));
#ifdef BDD_VERBOSE_CORRECTNESS_CHECK
	state->start_truth_table = bdd_create_truth_table(diagram, 1);
#else
	state->start_truth_table = NULL;
#endif
}

void uninit_sifting_state(sifting_state *state)
{
	free(state->best_var_positions);
	if (state->start_truth_table != NULL)
		bdd_free_truth_table(state->start_truth_table);
}

void sifting_save_node_count(sifting_state *state, int moving_var_num, int pos, int node_count)
{
	int size = sizeof(state->node_counts) / sizeof(state->node_counts[0]);
	
	if (moving_var_num < size && pos < size)
		state->node_counts[moving_var_num][pos] = node_count;
}

void sifting_print_node_counts(sifting_state *state)
{
	int size = sizeof(state->node_counts) / sizeof(state->node_counts[0]);
	int var_count = state->diagram->var_count;
	int i, j;

	if (var_count > size)
		var_count = size;
	for (i = 0; i < var_count; i++)
	{		
		for (j = 0; j < var_count; j++)
			printf(" %3d", state->node_counts[i][j]);
		printf("\n");
	}
}

int get_cur_var_position(sifting_state *state, int var_num)
{
	int i;

	for (i = state->diagram->var_count - 1; i >= 0; i--)
		if (state->diagram->var_reordering[i] == var_num)
			return i;
	bdd_report_error("Variable not found");
	return -1;
}

int do_sifting_step(sifting_state *state, int swap_first_index)
{
	bdd *diagram = state->diagram;
#ifdef BDD_VERBOSE_CORRECTNESS_CHECK
	bdd_truth_table *cur_truth_table;
#endif
	int cur_node_count;

	printf("Swapping levels %d and %d\n", swap_first_index + 1, swap_first_index + 2);
	_snprintf(diagram->root_node->label, BDD_MAX_NODE_LABEL_LEN,
						"Sift%d", swap_first_index + 1);
	diagram->root_node->label[BDD_MAX_NODE_LABEL_LEN] = 0;
	bdd_on_diagram_changed(5);
	bdd_swap_variables(diagram, swap_first_index);
	state->swap_count++;

	_snprintf(diagram->root_node->label, BDD_MAX_NODE_LABEL_LEN,
						"Sifted");
	diagram->root_node->label[BDD_MAX_NODE_LABEL_LEN] = 0;
	bdd_on_diagram_changed(9);

	// Very slow, but very simple
	bdd_reduce(diagram);   
	bdd_on_diagram_changed(8);

#ifdef BDD_VERBOSE_CORRECTNESS_CHECK
	cur_truth_table = bdd_create_truth_table(diagram, 1);
	if (!bdd_are_truth_tables_equal(state->start_truth_table, cur_truth_table))
	{
		bdd_report_error("Diagrams truth tables are different");
		bdd_set_diagram_changed_callback(NULL);
	}
	bdd_free_truth_table(cur_truth_table);
#endif

	cur_node_count = bdd_print_diagram_statistics(diagram);
	printf("\n");
	return cur_node_count;
}

void sifting_move_variable(sifting_state *state, int start_pos, int last_pos)
{
	int last_index, delta, cur_index;

	sifting_save_node_count(state, state->moving_var_num, start_pos, state->cur_node_count);
	assert(start_pos != last_pos);
	if (start_pos < last_pos)
	{
		cur_index = start_pos;
		delta = 1;
		last_index = last_pos - 1;
	}
	else
	{
		cur_index = start_pos - 1;
		delta = -1;
		last_index = last_pos;
	}

	while (1)
	{
		int cur_node_count = do_sifting_step(state, cur_index);
		int cur_pos = cur_index;

		if (delta > 0)
			cur_pos++;
		state->cur_node_count = cur_node_count;

		if (state->min_node_count >= cur_node_count)
		{
			// Reassigning even when min == cur in order to reduce moving of variable back
			state->min_node_count = cur_node_count;
			state->moving_var_best_position = cur_pos;
		}
		sifting_save_node_count(state, state->moving_var_num, cur_pos, cur_node_count);

		if (cur_index == last_index)
			break;
		cur_index += delta;
	}
}

int bdd_do_sifting(bdd *diagram)
{
	const int var_count = diagram->var_count;
	int start_node_count;
	int moving_var_num;  // Source index of the variable we are searching the best place for 
	sifting_state state_var;
	sifting_state *state = &state_var;
	LARGE_INTEGER time0, time, freq;

	if (var_count < 2)
		return 0;
	
	init_sifting_state(state, diagram);
	state->cur_node_count = bdd_print_diagram_statistics(diagram);
	state->min_node_count = state->cur_node_count;
	start_node_count = state->cur_node_count;

	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&time0);
	for (moving_var_num = 0; moving_var_num < var_count; moving_var_num++)
	{
		int cur_pos = get_cur_var_position(state, moving_var_num);

		state->moving_var_num = moving_var_num;
		state->moving_var_best_position = cur_pos;

    if (cur_pos == 0)
		{
			sifting_move_variable(state, cur_pos, var_count - 1);
		}  
		else  if (cur_pos == var_count - 1)
		{
			sifting_move_variable(state, cur_pos, 0);
		}
		else  if (cur_pos < var_count / 2)
		{
			sifting_move_variable(state, cur_pos, 0);
			sifting_move_variable(state, 0, var_count - 1);
		}
		else
		{
			sifting_move_variable(state, cur_pos, var_count - 1);
			sifting_move_variable(state, var_count - 1, 0);
		}

		cur_pos = get_cur_var_position(state, moving_var_num);
		if (state->moving_var_best_position != cur_pos)
			sifting_move_variable(state, cur_pos, state->moving_var_best_position);

		state->best_var_positions[moving_var_num] = state->moving_var_best_position;
		if (state->min_node_count <= 1)
			break;
	}
	QueryPerformanceCounter(&time);

	sifting_print_node_counts(state);
	printf("%d swap(s) made, %.6g seconds/swap\n", 
		     state->swap_count, 
				 (double)(time.QuadPart - time0.QuadPart) / freq.QuadPart / state->swap_count);
	uninit_sifting_state(state);
	return start_node_count - state->min_node_count;
}