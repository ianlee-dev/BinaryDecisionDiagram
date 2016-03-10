/*
 * Description:
 *
 * This file contains functions that manipulate logic function representation.
 * The idea is to have a unified representation for the function in its logic form,
 * not necessarily how it is implemented. For this purpose each function will be represented
 * in cubical form. This representation will be derived from the physical implementation,
 * so that new mapping possibility can be derived.
 *
 * Author:
 * Tomasz Czajkowski
 * University of Toronto
 * 2002.
 *
 *
 * REVISION HISTORY:
 * June 10, 2003: The following functions have been made available though the header file:
 *					create_empty_cubical_function_with_variable_list(...);
 *					translate_equation_to_cubical_function_representation(...);
 *					create_variable_index_list_for_function(...);
 *					process_single_LUT_function(...);
 *				  As it turns out this code is reusable in carry chain shortening,
 *				  but with different initial conditions. (TC)
 * October 2, 2006: Changed the logic function representation to use 2-bits per symbol versus 8. (TC)
 */

/*******************************************************************************************/
/****************************          INCLUDE FILES            ****************************/
/*******************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <malloc.h>
#include <string.h>

#include "common_types.h"
#include "cubical_function_representation.h"

/*******************************************************************************************/
/****************************         DEFINE STATEMENTS         ****************************/
/*******************************************************************************************/


/*******************************************************************************************/
/****************************         GLOBAL VARIABLES          ****************************/
/*******************************************************************************************/


/*******************************************************************************************/
/****************************    LOCAL FUNCTION DECLARATIONS    ****************************/
/*******************************************************************************************/


/*******************************************************************************************/
/****************************           IMPLEMENTATION          ****************************/
/*******************************************************************************************/


int read_cube_variable(long int *cube_signal_status, int var_index)
{
	return (cube_signal_status[var_index / (4*sizeof(long int))] & 
				  (LITERAL_DC << (2*(var_index & (4*sizeof(long int) - 1))))) >> (2*var_index);
}


void write_cube_variable(long int *cube, int var_index, int value)
{
	int mask = (-1) ^ (LITERAL_DC << (2*(var_index & (4*sizeof(long int) - 1))));

	assert((value == LITERAL_0) | (value == LITERAL_1) | (value == LITERAL_DC));

	cube[var_index / (4*sizeof(long int))] = (cube[var_index / (4*sizeof(long int))] & (mask)) |
											 (value << (2*(var_index & (4*sizeof(long int) - 1))));
}


void free_cubical_function(t_blif_cubical_function *f)
/* This function releases the memory allocated for a cubical function.
 */
{
	int index;

	if (f != NULL)
	{
		if (f->inputs > 0)
		{
			for(index = 0; index < f->input_count; index++)
			{
				free(f->inputs[index]);
			}
			free(f->inputs);
		}
		if (f->output != NULL)
		{
			free(f->output);
		}
		for(index = 0; index < f->cube_count; index++)
		{
			free(f->set_of_cubes[index]);
		}
		if (f->set_of_cubes != NULL)
		{
			free(f->set_of_cubes);
		}
		free(f);
	}
}

