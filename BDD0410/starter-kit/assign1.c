////////////////////////////////////////////////////////////////////////
// Solution to assignment #1 for ECE1733.
// This program implements the Quine-McCluskey method for 2-level
// minimization. 
////////////////////////////////////////////////////////////////////////

/**********************************************************************/
/*** HEADER FILES *****************************************************/
/**********************************************************************/

#include <stdlib.h>
#include <conio.h>
#include <stdio.h>
#include <assert.h>
#include <malloc.h>
#include <string.h>
#include <windows.h>
#include "common_types.h"
#include "bdd.h"
#include "bdd_apply.h"
#include "bdd_sifting.h"
#include "bdd_tools.h"
#include "blif_common.h"
#include "cubical_function_representation.h"

/**********************************************************************/
/*** DATA STRUCTURES DECLARATIONS *************************************/
/**********************************************************************/

/**********************************************************************/
/*** DEFINE STATEMENTS ************************************************/
/**********************************************************************/

/**********************************************************************/
/*** GLOBAL VARIABLES *************************************************/
/**********************************************************************/

/**********************************************************************/
/*** FUNCTION DECLARATIONS ********************************************/
/**********************************************************************/

int cube_cost(t_blif_cube *cube, int num_inputs);
int function_cost(t_blif_cubical_function *f);
int cover_cost(t_blif_cube **cover, int num_cubes, int num_inputs);

void simplify_function(t_blif_cubical_function *f);


/**********************************************************************/
/*** BODY *************************************************************/
/**********************************************************************/

bdd *bdd_console_create_diagram_from_blif(t_blif_cubical_function *function)
{
	bdd *diagram = bdd_create_diagram(function->input_count);
	int cube_index;

	bdd_set_cur_diagram(diagram);
	bdd_on_diagram_changed(1);
	for (cube_index = 0; cube_index < function->cube_count; cube_index++)
		bdd_add_cube_path(diagram, diagram->root_node, 
						          function->set_of_cubes[cube_index], 0, cube_index + 1);
	return diagram;   // This is actually a tree
}

/**********************************************************************/
/*** COST FUNCTIONS ***************************************************/
/**********************************************************************/


int cube_cost(t_blif_cube *cube, int num_inputs)
/* Wires and inverters are free, everything else is #inputs+1*/
{
	int index;
	int cost = 0;

	for (index = 0; index < num_inputs; index++)
	{
		if (read_cube_variable(cube->signal_status, index) != LITERAL_DC)
		{
			cost++;
		}
	}
	if (cost > 1)
	{
		cost++;
	}
	return cost;
}


int function_cost(t_blif_cubical_function *f)
{
	int cost = 0;
	int index;
	
	if (f->cube_count > 0)
	{
		for(index = 0; index < f->cube_count; index++)
		{
			cost += cube_cost(f->set_of_cubes[index], f->input_count);
		}
		if (f->cube_count > 1)
		{
			cost += (f->cube_count+1);
		}
	}

	return cost;
}


int cover_cost(t_blif_cube **cover, int num_cubes, int num_inputs)
{
	int result = 0;
	int index;

	for (index = 0; index < num_cubes; index++)
	{
		result += cube_cost(cover[index], num_inputs);
	}
	if (num_cubes > 1)
	{
		result += num_cubes+1;
	}
	return result;
}



/**********************************************************************/
/*** MINIMIZATION CODE ************************************************/
/**********************************************************************/

void simplify_function(t_blif_cubical_function *f)
/* This function simplifies the function f. The minimized set of cubes is
 * returned though a field in the input structure called set_of_cubes.
 * The number of cubes is stored in the field cube_count.
 */
{
	/* PUT YOUR CODE HERE */
}


void print_function_info(t_blif_cubical_function *function, int function_index)
{
	int cube_index;

	printf("Function %i: #inputs = %i; #cubes = %i; cost = %i\n", 
				 function_index+1, function->input_count, function->cube_count, 
				 function_cost(function)); 

	for (cube_index = 0; cube_index < function->cube_count; cube_index++)
	{
		printf("Cube %d: ", cube_index);
		print_cube(function->input_count, function->set_of_cubes[cube_index]);
	}
}

/**********************************************************************/
/*** MAIN FUNCTION ****************************************************/
/**********************************************************************/

void reduce_and_sift_all_functions(t_blif_logic_circuit *circuit)
{
	int index;

	/* Minimize each function, one at a time. */
	printf("Minimizing logic functions\n");
	for (index = 0; index < circuit->function_count; index++)
	{
		t_blif_cubical_function *function = circuit->list_of_functions[index];

		simplify_function(function);
	}

	bdd_set_diagram_changed_callback(&bdd_print_cur_diagram_as_table, 0);
	/* Print out synthesis report. */
	printf("Report:\r\n");
	for (index = 0; index < circuit->function_count; index++)
	{
		t_blif_cubical_function *function = circuit->list_of_functions[index];
		int var_count = function->input_count;
		bdd *diagram;

		print_function_info(function, index);
		diagram = bdd_console_create_diagram_from_blif(function);
		bdd_print_diagram_statistics(diagram);
		printf("\n");

		bdd_reduce(diagram);
		printf("After reduce:\n");
		bdd_print_diagram_statistics(diagram);
		printf("\n");

		do
		{
		} while (bdd_do_sifting(diagram) > 0);

		bdd_free_diagram(diagram);
	}

	/* Finish. */
	printf("Done.\r\n");
}

// Var_count contains number of variables in the bigger diagram (diagrams[0])
bdd *do_apply_on_two_diagrams(bdd *diagrams[], int use_hash)
{
	int var_count = max(diagrams[0]->var_count, diagrams[1]->var_count);
	LARGE_INTEGER time0, time, freq;
	char *addit_info;
	bdd *diagram = bdd_create_diagram(var_count);
	bdd *diagram2;

	bdd_set_cur_diagram(diagram);
	if (use_hash)
		addit_info = " with hash";
	else 
		addit_info = "";
	QueryPerformanceFrequency(&freq);

	QueryPerformanceCounter(&time0);
	bdd_add_apply_result(diagram, diagrams[0], apply_operation_or,
		                   diagrams[1], use_hash, var_count);
	QueryPerformanceCounter(&time);
	printf("\nApply%s result (%.6g seconds):\n", 
		     addit_info, (double)(time.QuadPart - time0.QuadPart) / freq.QuadPart);
	bdd_print_diagram_statistics(diagram);
	printf("\n");
			
	//QueryPerformanceCounter(&time0);
	//bdd_reduce(diagram);
	//QueryPerformanceCounter(&time);
	//printf("After reduce (%.6g seconds):\n", 
	//	     (double)(time.QuadPart - time0.QuadPart) / freq.QuadPart);
	//bdd_print_diagram_statistics(diagram);

	diagram2 = bdd_create_diagram(var_count);
	QueryPerformanceCounter(&time0);
	bdd_add_apply_result(diagram2, diagram, apply_operation_and,
						           diagrams[0], 1, var_count);
	QueryPerformanceCounter(&time);
	printf("Apply 2%s result (%.6g seconds):\n", 
			   addit_info, (double)(time.QuadPart - time0.QuadPart) / freq.QuadPart);
	bdd_print_diagram_statistics(diagram2);
	printf("\n");
			
	QueryPerformanceCounter(&time0);
	bdd_reduce(diagram2);
	QueryPerformanceCounter(&time);
	printf("After reduce (%.6g seconds):\n", 
			   (double)(time.QuadPart - time0.QuadPart) / freq.QuadPart);
	bdd_print_diagram_statistics(diagram2);

	QueryPerformanceCounter(&time0);
	bdd_free_diagram(diagram);
	QueryPerformanceCounter(&time);
	printf("Free time: %.6g seconds\n", 
			    (double)(time.QuadPart - time0.QuadPart) / freq.QuadPart);

	return diagram2;
}

void do_apply_on_two_functions(t_blif_logic_circuit *circuit)
{
	bdd *saved_diagrams[] = { NULL, NULL };
	int index, i;
	LARGE_INTEGER time0, time, freq;

	QueryPerformanceFrequency(&freq);
	bdd_set_diagram_changed_callback(&bdd_print_cur_diagram_as_table, 0);
	for (index = 0; index < circuit->function_count; index++)
	{
		t_blif_cubical_function *function = circuit->list_of_functions[index];
		int var_count = function->input_count;
		bdd *diagram;

		print_function_info(function, index);
		diagram = bdd_console_create_diagram_from_blif(function);
		bdd_print_diagram_statistics(diagram);

		bdd_reduce(diagram);
		printf("After reduce:\n");
		bdd_print_diagram_statistics(diagram);
		printf("\n");

		if (index < 2)
			saved_diagrams[index] = diagram;
		else
			bdd_free_diagram(diagram);
	}

	if (circuit->function_count >= 2)
	{
		// Both diagrams must contain variables in the same order.
		// Saved_diagrams[1] may contain less variables than saved_diagrams[0],
		// but not inversely
		int var_count = circuit->list_of_functions[0]->input_count;
		bdd *diagram_hash = do_apply_on_two_diagrams(saved_diagrams, 1);
		bdd *diagram = do_apply_on_two_diagrams(saved_diagrams, 0);

		if (!bdd_are_subdiagrams_equal(diagram_hash->root_node, diagram->root_node))
			bdd_report_error("Different results of Apply");
		QueryPerformanceCounter(&time0);
		bdd_free_diagram(diagram_hash);
		bdd_free_diagram(diagram);
		QueryPerformanceCounter(&time);
		printf("Free time: %.6g seconds\n", 
			     (double)(time.QuadPart - time0.QuadPart) / freq.QuadPart);
	}

	printf("Done.\r\n");
	for (i = 0; i < sizeof(saved_diagrams) / sizeof(saved_diagrams[0]); i++)
		if (saved_diagrams[i] != NULL)
			bdd_free_diagram(saved_diagrams[i]);
}

int main(int argc, char* argv[])
{
	t_blif_logic_circuit *circuit = NULL;

	if (argc != 2)
	{
		printf("Usage: %s <source BLIF file>\r\n", argv[0]);
		return 0;
	}
//	printf("Quine-McCluskey 2-level logic minimization program.\r\n");

	/* Read BLIF circuit. */
	printf("Reading file %s...\n",argv[1]);
	circuit = ReadBLIFCircuit(argv[1]);

	if (circuit != NULL)
	{
		reduce_and_sift_all_functions(circuit);    // Mainly code that was in old main()
		do_apply_on_two_functions(circuit);
		DeleteBLIFCircuit(blif_circuit);
	}
	else
	{
		printf("Error reading BLIF file. Terminating.\n");
	}
	return 0;
}

