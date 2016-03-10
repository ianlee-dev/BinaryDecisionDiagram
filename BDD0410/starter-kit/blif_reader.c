// BLIF Reader
//
// This is the shell for the BLIF reader program, which reads in a BLIF described logic circuit.
// 
// Author: Tomasz Czajkowski
// Date: October 13, 2006
// NOTES/REVISIONS:
/////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////
//   INCLUDE   //////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////

#include <conio.h>
#include <string.h>
#include <assert.h>

#include "blif_common.h"
#include "common_types.h"
#include "cubical_function_representation.h"

/////////////////////////////////////////////////////////////////////////////////////////////////
//   LOCAL VARIABLES   //////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////

t_blif_logic_circuit *blif_circuit = NULL;
int blif_error_code = 0;

/////////////////////////////////////////////////////////////////////////////////////////////////
//   LOCAL DECLARATIONS   ///////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////

void FreeSignalList(int signal_count, t_blif_signal **list, t_boolean free_name);

/////////////////////////////////////////////////////////////////////////////////////////////////
//   CODE   /////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////

void write_cube(t_blif_cube *cube, int index, int val)
{
	write_cube_variable(cube->signal_status, index, val);
}


void addSignalToList(int list_id, char *signal_name)
/* Add the specified signal to one of the signal lists. Assume that the memory for the signal
 * name is already allocated and passed as a pointer to this function. */
{
	t_blif_signal *signal = (t_blif_signal *) malloc(sizeof(t_blif_signal));

	assert(signal != NULL);

	signal->data.name = signal_name;
	signal->type = (list_id == INTERNAL_SIGNAL_LIST) ? SIGNAL_LOCAL : ((list_id == PRIMARY_INPUT_LIST) ? SIGNAL_EXTERNAL_IN : SIGNAL_EXTERNAL_OUT);

	if (list_id == INTERNAL_SIGNAL_LIST)
	{
		blif_circuit->internal_signal_count =
			append_array_element(	((int)(signal)),
									(int **) &(blif_circuit->internal_signals),
									blif_circuit->internal_signal_count);
	}

	if (list_id == PRIMARY_INPUT_LIST)
	{
		blif_circuit->primary_input_count =
			append_array_element(	((int)(signal)),
									(int **) &(blif_circuit->primary_inputs),
									blif_circuit->primary_input_count);
	}

	if (list_id == PRIMARY_OUTPUT_LIST)
	{
		blif_circuit->primary_output_count =
			append_array_element(	((int)(signal)),
									(int **) &(blif_circuit->primary_outputs),
									blif_circuit->primary_output_count);
	}
}


int findSignalInList(int list_id, char *signal_name)
/* Find a signal with the specified name. */
{
	t_blif_signal **list = NULL;
	int list_size = 0;
	int result = -1;

	assert(signal_name != NULL);
	

	if (list_id == INTERNAL_SIGNAL_LIST)
	{
		list = blif_circuit->internal_signals;
		list_size = blif_circuit->internal_signal_count;
	}
	else
	if (list_id == PRIMARY_INPUT_LIST)
	{
		list = blif_circuit->primary_inputs;
		list_size = blif_circuit->primary_input_count;
	}
	else
	if (list_id == PRIMARY_OUTPUT_LIST)
	{
		list = blif_circuit->primary_outputs;
		list_size = blif_circuit->primary_output_count;
	}

	/* Now search. */
	if ((list != NULL) && (list_size > 0))
	{
		int index;

		for (index = 0; index < list_size; index++)
		{
			t_blif_signal *signal = list[index];

			if (strcmp(signal->data.name, signal_name) == 0)
			{
				result = index;
				break;
			}
		}
	}
	return result;
}


t_blif_logic_circuit *ReadBLIFCircuit(char *filename)
/* Read a BLIF defined circuit from the specified file. */
{
	blif_yyin = fopen(filename,"r");
	blif_circuit = (t_blif_logic_circuit *) malloc(sizeof(t_blif_logic_circuit));
	blif_error_code = 0;

	assert(blif_yyin != NULL);
	assert(blif_circuit != NULL);

	/* Initialize blif_circuit structure. */
	blif_circuit->name = NULL;
	blif_circuit->internal_signals = NULL;
	blif_circuit->list_of_functions = NULL;
	blif_circuit->primary_inputs = NULL;
	blif_circuit->primary_outputs = NULL;
	blif_circuit->primary_output_count = 0;
	blif_circuit->primary_input_count = 0;
	blif_circuit->function_count = 0;
	blif_circuit->internal_signal_count = 0;

	blif_yyparse();
	blif_yyrestart(blif_yyin);
	fclose(blif_yyin);

	/* If error occured then delete the circuit and return NULL */
	if (blif_error_code != 0)
	{
		DeleteBLIFCircuit(blif_circuit);
		blif_circuit = NULL;
	}
	return blif_circuit;
}


void FreeSignalList(int signal_count, t_blif_signal **list, t_boolean free_name)
/* Free signal list. */
{
	if (list != NULL)
	{
		int index;

		for (index = 0; index < signal_count; index++)
		{
			t_blif_signal *input = list[index];
			
			if (free_name == T_TRUE)
			{
				free(input->data.name);
			}
			free(input);
		}
		free(list);
	}
}

void DeleteBLIFCircuit(t_blif_logic_circuit *circuit)
/* Free the memory allocated during parsing of the BLIF file. */
{
	if (circuit != NULL)
	{
		FreeSignalList(circuit->primary_input_count, circuit->primary_inputs, T_TRUE);
		FreeSignalList(circuit->primary_output_count, circuit->primary_outputs, T_TRUE);
		FreeSignalList(circuit->internal_signal_count, circuit->internal_signals, T_TRUE);

		free(circuit->name);
		free(circuit);
	}
}


