/*
 * This is a header file containing some commonly used types and definitions.
 *
 * Author:
 * Tomasz Czajkowski
 * University of Toronto
 * 2002.
 */


#if !defined (__COMMON_TYPES__)
#define __COMMON_TYPES__

#include <math.h>

/*******************************************************************************************/
/****************************               TYPES               ****************************/
/*******************************************************************************************/


#if !defined(t_boolean)
typedef enum e_boolean { T_FALSE = 0, T_TRUE } t_boolean;
#endif


/*******************************************************************************************/
/****************************         FUNCTION HEADERS          ****************************/
/*******************************************************************************************/

int *allocate_array(int element_count);
int append_array_element(int element, int **array, int element_count);
int append_array_with_array(int *new_elements, int new_element_count, int **array, int array_size);
long int append_array_element_long(long int element, long int **array, long int element_count);
t_boolean is_element_in_array(int element, int *array, int element_count);
int remove_element_by_index(int element_index, int *array, int element_count);
int get_element_index(int element, int *array, int element_count);
int remove_element_by_content(int element_content, int *array, int element_count);
long int calculate_array_size_using_bounds(long int element_count);

#endif
