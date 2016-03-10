#if !defined(BLIF_COMMON)
#define BLIF_COMMON

#include <stdio.h>
#include "cubical_function_representation.h"

extern int			blif_yylex();
extern char*		blif_yytext;
extern int			blif_yylineno;
extern FILE			*blif_yyin;
extern int			blif_yyerror(char *s);
extern int			blif_yyparse();
extern void			blif_yyrestart(FILE *input_file);

/* Global Variable set */
extern t_blif_logic_circuit *blif_circuit;
extern int blif_error_code;

/* Define statements */
#define INTERNAL_SIGNAL_LIST	0
#define PRIMARY_INPUT_LIST		1
#define PRIMARY_OUTPUT_LIST		2

/* Functions to be called from the parser */
void addSignalToList(int list_id, char *signal_name);
int findSignalInList(int list_id, char *signal_name);
void write_cube(t_blif_cube *cube, int index, int val);

/* Structures */
typedef struct s_temp_list {
	int count;
	int *contents;
} t_temp_list;

typedef struct s_temp_cube_list {
	t_blif_cube **cube_list;
	int cube_count;
} t_temp_cube_list;

t_blif_logic_circuit	*ReadBLIFCircuit(char *filename);
void					DeleteBLIFCircuit(t_blif_logic_circuit *circuit);


#endif