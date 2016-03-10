/* BLIF Parser Grammar File
 *
 * This file describes the grammar for the BLIF file format.
 *
 * Author: Tomasz Czajkowski
 * Date: October 11, 2006
 * NOTES/REVISIONS:
 */

%{

//#define BLIF_VERBOSE
#define CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include "common_types.h"
#include "blif_common.h"

%}

/********************************************************/
/**** DEFINE STRUCTURES *********************************/
/********************************************************/

%union
{
	int value;
	char *string;
}

/********************************************************/
/**** DEFINE TOKENS *************************************/
/********************************************************/


%token TOKEN_MODEL
%token TOKEN_NAMES
%token TOKEN_INPUT
%token TOKEN_OUTPUT
%token TOKEN_END
%token TOKEN_CUBE_STRING
%token TOKEN_CUBE_VALUE
%token TOKEN_STRING
%token TOKEN_LINE_BREAK

%type <value> signal signal_list cube_statement cube_statements function_declaration single_function function_list TOKEN_CUBE_VALUE
%type <string> header TOKEN_STRING TOKEN_CUBE_STRING


/********************************************************/
/**** DEFINE START POINT ********************************/
/********************************************************/


%start blif_circuit
%%

blif_circuit:		models {}
			;

models:		model {}
			|	models model {}
			;

model:		header input_declaration output_declaration function_list footer
			{
				/* Now create a module structure and store all data you just read into it. */
				t_temp_list *list = (t_temp_list *) $4;
				blif_circuit->name = (char *) $1;
				blif_circuit->function_count = list->count;
				blif_circuit->list_of_functions = (t_blif_cubical_function **) list->contents;
				free(list);
			}
			;

header:		TOKEN_MODEL TOKEN_STRING
			{
				$$ = $2;
			}
			;

footer:		/* Empty */
			|	TOKEN_END
			;

input_declaration:	/* Empty */ {}
					|	TOKEN_INPUT signal_list
						{
							/* Add signals to the set of primary inputs. */
							t_temp_list *list = (t_temp_list *) $2;
							
							if (list != NULL)
							{
								int index;
								
								for(index = 0; index < list->count; index++)
								{
									t_blif_signal *signal = (t_blif_signal *) list->contents[index];
									
									signal->type = SIGNAL_EXTERNAL_IN;
								}
								blif_circuit->primary_inputs = (t_blif_signal **) list->contents;	/* Array of pointers to t_signal */
								blif_circuit->primary_input_count = list->count;

								free(list);
							}						
						}
					;
output_declaration:	/* Empty */ {}
					|	TOKEN_OUTPUT signal_list
						{
							/* Add signals to the set of primary outputs. */
							t_temp_list *list = (t_temp_list *) $2;
							
							if (list != NULL)
							{
								int index;
								
								for(index = 0; index < list->count; index++)
								{
									t_blif_signal *signal = (t_blif_signal *) list->contents[index];
									
									signal->type = SIGNAL_EXTERNAL_OUT;
								}							
								blif_circuit->primary_outputs = (t_blif_signal **) list->contents;	/* Array of pointers to t_signal */
								blif_circuit->primary_output_count = list->count;

								free(list);
							}
						}
					;


function_list:		function_list single_function
					{
						t_temp_list *list = (t_temp_list *) $1;
						t_blif_cubical_function *func = (t_blif_cubical_function *) $2;

						list->count =
							append_array_element((int) func,
												 (int **) &(list->contents),
												 list->count);
						$$ = (int) list;
					}
				|	single_function
					{
						t_blif_cubical_function *func = (t_blif_cubical_function *) $1;
						t_temp_list *list = (t_temp_list *) malloc(sizeof(t_temp_list));
						
						list->contents = NULL;
						list->count = 0;
						list->count =
							append_array_element((int) func,
												 (int **) &(list->contents),
												 list->count);
						$$ = (int) list;
					}					
				|	/* Empty */
					{
						$$ = (int) NULL;
					}
				;
				
single_function:	function_declaration cube_statements
						{
							t_blif_cubical_function *function = (t_blif_cubical_function *) $1;
							t_temp_cube_list *list = (t_temp_cube_list *) $2;
						#if defined (BLIF_VERBOSE)
							int index;
							t_blif_signal *signal;
						#endif
							
							assert(function != NULL);
							
							if (list == NULL)
							{
								function->value = 0;
							}
							else
							{
								function->set_of_cubes = list->cube_list;
								function->cube_count = list->cube_count;
								free(list);
							}
						#if defined (BLIF_VERBOSE)							
							printf("Added Function:\n");
							signal = function->output;
							if (signal->type == SIGNAL_LOCAL)
							{
								printf("Output: %s\n", blif_circuit->internal_signals[signal->data.index]->data.name);
							}
							else
							if (signal->type == SIGNAL_EXTERNAL_OUT)
							{
								printf("Output: %s\n", blif_circuit->primary_outputs[signal->data.index]->data.name);
							}
							printf("Inputs: ");
							for(index = 0; index < function->input_count; index++)
							{
								signal = function->inputs[index];
								
								if (signal->type == SIGNAL_LOCAL)
								{
									printf("%s ", blif_circuit->internal_signals[signal->data.index]->data.name);
								}
								else
								if (signal->type == SIGNAL_EXTERNAL_IN)
								{
									printf("%s ", blif_circuit->primary_inputs[signal->data.index]->data.name);
								}
								else
								if (signal->type == SIGNAL_EXTERNAL_OUT)
								{
									printf("%s ", blif_circuit->primary_outputs[signal->data.index]->data.name);
								}								
							}
							printf("\n");
							printf("Cubes: %i\n", function->cube_count);
						#endif
							$$ = (int) function;
						}
					;
					
function_declaration: TOKEN_NAMES signal_list
						{
							/* By now the list of function inputs is an ordered list -> as read from a file. 
							 * The last signal in the signal_list is the function output.
							 *
							 * Create a cubical function declaration for this function. Replace signal names with indicies to
							 * corresponding signals in the circuit primary I/O and internal signal lists.
							 */
							int index;
							t_temp_list *list = (t_temp_list *) $2;
							t_blif_cubical_function *function = (t_blif_cubical_function *) malloc(sizeof(t_blif_cubical_function));

							assert(function != NULL);
							
							function->input_count = 0;
							function->inputs = NULL;
							function->output = NULL;
							function->cube_count = 0;
							function->set_of_cubes = NULL;
							function->value = -1;
							
							assert(list != NULL);
						
							for(index = 0; index < list->count; index++)
							{
								t_blif_signal *logic_signal = (t_blif_signal *) list->contents[index];
								int location, list_id;
								
								location = findSignalInList(INTERNAL_SIGNAL_LIST, logic_signal->data.name);
								list_id = INTERNAL_SIGNAL_LIST;
								if (location < 0)
								{
									list_id = PRIMARY_INPUT_LIST;
									location = findSignalInList(PRIMARY_INPUT_LIST, logic_signal->data.name);
									if (location < 0)
									{
										list_id = PRIMARY_OUTPUT_LIST;
										location = findSignalInList(PRIMARY_OUTPUT_LIST, logic_signal->data.name);
									}							
								}
								
								if (location < 0)
								{
									/* A new intermediate signal has just been declared, so store it. */
									list_id = INTERNAL_SIGNAL_LIST;
									addSignalToList(list_id, logic_signal->data.name);
									location = blif_circuit->internal_signal_count - 1;
									logic_signal->data.index = location;
								}
								else
								{
									/* If it already exists then free memory taken by the signal name string. */
									free(logic_signal->data.name);
								}
								
								/* Save data in proper memory location. */
								logic_signal->type = (list_id == INTERNAL_SIGNAL_LIST) ? SIGNAL_LOCAL : ((list_id == PRIMARY_INPUT_LIST) ? SIGNAL_EXTERNAL_IN : SIGNAL_EXTERNAL_OUT);
								logic_signal->data.index = location;
								
								if (index == list->count - 1)
								{
									function->output = logic_signal;
								}
								else
								{
									function->input_count = append_array_element((int) logic_signal,
																				 (int **) &(function->inputs),
																				 function->input_count);
								}
							}
							
							free(list->contents);
							free(list);
							
							$$ = (int) function;
						}
					;

cube_statements:	/* Empty */
						{
							$$ = (int) NULL;
						}					
					| cube_statement
						{
							$$ = $1;
						}
					| cube_statements cube_statement
						{
							t_temp_cube_list *list = (t_temp_cube_list *) $1;
							t_temp_cube_list *new_cube = (t_temp_cube_list *) $2;
							
							assert(list != NULL);
							assert(new_cube != NULL);
							
							if (new_cube->cube_count == 1)
							{
								list->cube_count = append_array_element((int) new_cube->cube_list[0],
																	(int **) &(list->cube_list),
																	list->cube_count);
							}
							free(new_cube->cube_list);
							free(new_cube);
							
							$$ = (int) list;
						}
					;
			
cube_statement:		TOKEN_CUBE_STRING TOKEN_CUBE_VALUE
					{
						t_temp_cube_list *cube_list = (t_temp_cube_list *) malloc(sizeof(t_temp_cube_list));
						char *cube_string = (char *) $1;
						int cube_value = (int) $2;

						cube_list->cube_list = NULL;
						cube_list->cube_count = 0;
						if ((cube_value == LITERAL_1) || (cube_value == LITERAL_DC))
						{
							t_blif_cube *cube = (t_blif_cube *) malloc(sizeof(t_blif_cube));
							int index;
							
							memset(cube, 0, sizeof(t_blif_cube));
							cube->data_size = strlen(cube_string) / 16;
							cube->is_DC = ((cube_value == LITERAL_1) ? T_FALSE : T_TRUE);
							
							if (cube->data_size == 0)
							{
								cube->data_size = 1;
							}
							
							for(index = 0; index < (int) strlen(cube_string); index++)
							{
								int val = LITERAL_0;
								
								if (cube_string[index] == CONST_1)
								{
									val = LITERAL_1;
								}
								else if (cube_string[index] == CONST_DC)
								{
									val = LITERAL_DC;
								}
								write_cube(cube, index, val);
							}
							cube_list->cube_count =
								append_array_element((int) cube,
													 (int **) &(cube_list->cube_list),
													 cube_list->cube_count);
						}
						
						$$ = (int) cube_list;
					}
				|	TOKEN_CUBE_VALUE
					{
						/* A constant function */
						t_temp_cube_list *cube_list = (t_temp_cube_list *) malloc(sizeof(t_temp_cube_list));
						
						cube_list->cube_list = NULL;
						cube_list->cube_count = 0;
						if (($1 == LITERAL_1) || ($1 == LITERAL_DC))
						{
							t_blif_cube *cube = (t_blif_cube *) malloc(sizeof(t_blif_cube));

							memset(cube, 0, sizeof(t_blif_cube));
							cube->data_size = 0;
							cube->is_DC = (($1 == LITERAL_1) ? T_FALSE : T_TRUE);

							cube_list->cube_count =
								append_array_element((int) cube,
													 (int **) &(cube_list->cube_list),
													 cube_list->cube_count);
						}
						
						$$ = (int) cube_list;
					}										
				;
				
signal_list:	signal
				{
					t_blif_signal *logic_signal = (t_blif_signal *) $1;
					t_temp_list *list = (t_temp_list *) malloc(sizeof(t_temp_list));
					
					assert(logic_signal != NULL);
					assert(list != NULL);
					
					list->count = 0;
					list->contents = NULL;
					
					list->count = append_array_element((int) logic_signal,
														&(list->contents), list->count);
					$$ = (int) list;
				}
			|	signal_list signal 
				{
					t_blif_signal *logic_signal = (t_blif_signal *) $2;
					t_temp_list *list = (t_temp_list *) $1;
					
					assert(logic_signal != NULL);
					assert(list != NULL);
					
					list->count = append_array_element((int) logic_signal,
														&(list->contents), list->count);
					$$ = (int) list;				
				}
			|	signal_list TOKEN_LINE_BREAK signal 
				{
					t_blif_signal *logic_signal = (t_blif_signal *) $3;
					t_temp_list *list = (t_temp_list *) $1;
					
					assert(logic_signal != NULL);
					assert(list != NULL);
					
					list->count = append_array_element((int) logic_signal,
														&(list->contents), list->count);
					$$ = (int) list;				
				}								
			;

signal:		TOKEN_STRING
			{
				t_blif_signal *logic_signal = (t_blif_signal *) malloc(sizeof(t_blif_signal));
				
				assert(logic_signal != NULL);
				
				logic_signal->data.name = $1;
				$$ = (int) logic_signal;
			}
			;
%%


/********************************************************/
/**** DEFINE ERROR HANDLER FUNCTION *********************/
/********************************************************/


int blif_yyerror(char *s)
{
	blif_error_code = 1;
	printf("ERROR %i: %s at symbol '%s'\r\n", blif_yylineno, s, blif_yytext);
	return 0;
}
