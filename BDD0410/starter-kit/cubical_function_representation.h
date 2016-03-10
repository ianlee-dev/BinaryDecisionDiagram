/* 
 * This is the header file for cubical_function_representation module.
 *
 * Author:
 * Tomasz Czajkowski
 * University of Toronto
 * 2002.
 */

#if !defined (__CUBICAL_FUNCTION_REPRESENTATION__)
#define __CUBICAL_FUNCTION_REPRESENTATION__

/*****************************************************/
/*** INCLUDE FILES ***********************************/
/*****************************************************/


#include <stdio.h>
#include <conio.h>
#include <malloc.h>
#include "common_types.h"


/*****************************************************/
/*** DATA STRUCTURES *********************************/
/*****************************************************/

/* Define statements */

#define LITERAL_DC		0x3
#define LITERAL_1		0x2
#define LITERAL_0		0x1
#define LITERAL_MARKER	'A'

#define SPACE_CHARACTER ' '
#define	NOT_OPERATOR	'~'
#define	AND_OPERATOR	'*'
#define	OR_OPERATOR		'+'
#define	XOR_OPERATOR	'@'
#define	CONST_1			'1'
#define CONST_0			'0'
#define CONST_DC		'-'

/***************************************************/
/* MAX 64 Variables. - No need to try more anyway. */
/***************************************************/

typedef struct s_blif_cube {
	int data_size;
	/* Number of long ints used */

	long int signal_status[4];
	/* For each cube the signal entry can be either 0, 1, or X.
	 */

	t_boolean is_DC;
	/* is cube a DC? */
} t_blif_cube;
/* This structure defines the cube used in logic function representation.
 */

typedef enum e_signal_type { SIGNAL_LOCAL = 0, SIGNAL_EXTERNAL_IN, SIGNAL_EXTERNAL_OUT, SIGNAL_LOCAL_OTHER } t_signal_type;

typedef struct s_blif_signal
{
	t_signal_type type;

	union {
		char *name;
		int index;
	} data;
} t_blif_signal; 
/* Structure holding signal information */


typedef	struct s_blif_cubical_function {
	int input_count;
	/* Number of inputs to the function */

	t_blif_signal **inputs;
	/* List of input signals */

	t_blif_signal *output;
	/* Logic function output */

	int cube_count;
	/* Number of cubes */

	t_blif_cube **set_of_cubes;
	/* Set of cubes */

	int value;
	/* If there are no cubes then this stores the constant value the function evaluates to. Otherwise, should be < 0 */

} t_blif_cubical_function;
/* This structure represents defines logic function representation. All operations
 * on the function itself will be performed using this structure.
 */


typedef struct s_blif_logic_circuit {
	char *name;
	/* Circuit name */

	t_blif_signal **primary_inputs;	/* Array of pointers to t_signal */
	int primary_input_count;

	t_blif_signal **primary_outputs;	/* Array of pointers to t_signal */ 
	int primary_output_count;

	t_blif_signal **internal_signals;	/* Array of pointers to t_signal */ 
	int internal_signal_count;

	t_blif_cubical_function **list_of_functions; /* Array of pointers to t_cubical_function */
	int function_count;
} t_blif_logic_circuit;
/* This structure is passed to the calling program as a result of reading in a BLIF file. */


/*****************************************************/
/*** FUNCTION DECLARATIONS ***************************/
/*****************************************************/

void					free_cubical_function(t_blif_cubical_function *f);
int						read_cube_variable(long int *cube, int var_index);
void					write_cube_variable(long int *cube, int var_index, int value);

#endif