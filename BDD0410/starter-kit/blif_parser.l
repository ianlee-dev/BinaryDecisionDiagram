/* BLIF Parser
 *
 * This is the token file for the BLIF parser. 
 *
 * Author: Tomasz Czajkowski
 * Date: October 11, 2004
 * NOTES/REVISIONS:
 */

%{


/*********************************************************************/
/**** PUT INCLUDE FILES HERE *****************************************/
/*********************************************************************/

#define CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <stdlib.h>
#include <string.h>
#include "cubical_function_representation.h"
#include "blif_parser.tab.h"

%}

/*********************************************************************/
/**** PUT OPTIONS HERE ***********************************************/
/*********************************************************************/

%option noyywrap
%option yylineno

/*********************************************************************/
/**** PUT SYMBOL DEFINITIONS HERE ************************************/
/*********************************************************************/
STATE	[0-1]|[-]

/*********************************************************************/
/**** PUT ANALYZER SYMBOL STATES *************************************/
/*********************************************************************/
/* Exclusive state - only when parsing cubes */

%s cubes
%s strings

%%
^[ \t]*\#[^\n]*\n									/* skip one-line comments */
[ \t]+												/* skip white spaces */
\n													BEGIN(INITIAL); /* skip empty lines */
\.model												BEGIN(strings); return TOKEN_MODEL;
\.inputs											BEGIN(strings); return TOKEN_INPUT;
\.outputs											BEGIN(strings); return TOKEN_OUTPUT;
\.names												BEGIN(strings); return TOKEN_NAMES;
\.end												return TOKEN_END;
\\[ \t]*\n											return TOKEN_LINE_BREAK;
<cubes>0[ \t]*\n									blif_yylval.value = LITERAL_0; BEGIN(INITIAL); return TOKEN_CUBE_VALUE;
<cubes>1[ \t]*\n									blif_yylval.value = LITERAL_1; BEGIN(INITIAL); return TOKEN_CUBE_VALUE;
<cubes>[-][ \t]*\n									blif_yylval.value = LITERAL_DC; BEGIN(INITIAL); return TOKEN_CUBE_VALUE;
<strings>[a-zA-Z_0-9()<>\[\]][a-zA-Z_0-9()\[\]\.]*	blif_yylval.string = (char *)malloc(yyleng+1); strcpy(blif_yylval.string, yytext); return TOKEN_STRING;
0[ \t]*\n											blif_yylval.value = LITERAL_0; BEGIN(INITIAL); return TOKEN_CUBE_VALUE;
1[ \t]*\n											blif_yylval.value = LITERAL_1; BEGIN(INITIAL); return TOKEN_CUBE_VALUE;
[-][ \t]*\n											blif_yylval.value = LITERAL_DC; BEGIN(INITIAL); return TOKEN_CUBE_VALUE;
{STATE}+											BEGIN(cubes); blif_yylval.string = (char *)malloc(yyleng+1); strcpy(blif_yylval.string, yytext); return TOKEN_CUBE_STRING;
.													return (int)(*yytext);

%%
