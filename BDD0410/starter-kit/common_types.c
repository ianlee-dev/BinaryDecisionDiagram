/*
 * Description:
 *
 * This module is a set of functions that are generally used.
 * These functions handle:
 * -	integer/pointer array allocation
 * -	array memory allocation and reallocation
 * -	array item insertion/deletion
 *
 * Author:
 * Tomasz Czajkowski
 * University of Toronto
 * 2002.
 *
 */

/*******************************************************************************************/
/****************************          INCLUDE FILES            ****************************/
/*******************************************************************************************/


#include <stdio.h>
#include <assert.h>
#include <malloc.h>
#include <memory.h>
#include "common_types.h"


/*******************************************************************************************/
/****************************         DEFINE STATEMENTS         ****************************/
/*******************************************************************************************/


#define MINIMUM_ARRAY_SIZE		16
/* Defines the minimum number of elements the array is to be created for. This macro and
 * the next one are created to minimize the number of malloc / free calls when handling
 * variable size arrays. This bound must be a power of 2.
 */


#define	UPPER_GROWTH_BOUND		1024
/* Once the array reaches this number of elements the growth of the array will be linear.
 * Once the array reaches this size it will be increased by the number specified above every
 * time it needs to have more mmeory reallocated. This is to prevent the array growth to the
 * size where almost half of the array is unused, but still takes memory space.
 * This bound must be a power of 2.
 */

/*******************************************************************************************/
/****************************         LOCAL STRUCTURES          ****************************/
/*******************************************************************************************/


/*******************************************************************************************/
/****************************         GLOBAL VARIABLES          ****************************/
/*******************************************************************************************/


/*******************************************************************************************/
/****************************    LOCAL FUNCTION DECLARATIONS    ****************************/
/*******************************************************************************************/


/*******************************************************************************************/
/****************************          IMPLEMENTATION            ***************************/
/*******************************************************************************************/


int *allocate_array(int element_count)
/* This funciton allocates the memory for the specified array. The size is specified in
 * number of elements, not number of bytes. When allocating memory this function will
 * multiply the element_count by sizeof(int).
 */
{
	int *array;
	int array_size;

	array_size = calculate_array_size_using_bounds(element_count);
	array = (int *) malloc(array_size * sizeof(int));

	assert(array != NULL);

	return array;
}


int *reallocate_array(int *array, int new_element_count)
/* This funciton reallocates the memory for the specified array. The size is specified in
 * number of elements, not number of bytes. When reallocating memory this function will
 * multiply the element_count by sizeof(int).
 */
{
	int *new_array;

	new_array = (int *) realloc(array, new_element_count * sizeof(int));

	assert(new_array != NULL);

	return new_array;
}


int append_array_element(int element, int **array, int element_count)
/* This function adds an element to the end of the array. If need be the array
 * will be resized. If the array was NULL to begin with the memory will be
 * allocated to it, because realloc behaves like malloc when a NULL pointer
 * is passed to it as parameter. This function returns the new size of the array.
 */
{
	int new_element_count;
	int *element_access;

	assert(element_count >= 0);
	assert(array != NULL);

	/* Check array element count */
	new_element_count = (int) calculate_array_size_using_bounds(element_count+1);

	if ((*array == NULL) || (new_element_count > calculate_array_size_using_bounds(element_count)))
	{
		*array = reallocate_array(*array, new_element_count);
	}

	/* Add element at the end */
	element_access = *array;
	element_access[element_count] = element;

	return element_count + 1;
}


int append_array_with_array(int *new_elements, int new_element_count, int **array, int array_size)
/* This function adds an array of elements to the end of the array. If need be the array
 * will be resized. If the array was NULL to begin with the memory will be
 * allocated to it, because realloc behaves like malloc when a NULL pointer
 * is passed to it as parameter. This function returns the new size of the array.
 */
{
	int new_array_size = array_size;
	int *element_access;

	assert(array_size >= 0);
	assert(array != NULL);
	assert(new_elements != NULL);
	assert(new_element_count >= 0);
	
	if (new_element_count > 0)
	{
		/* Check array element count */
		new_array_size = (int) calculate_array_size_using_bounds(array_size+new_element_count);

		if ((*array == NULL) || (new_array_size > calculate_array_size_using_bounds(array_size)))
		{
			*array = reallocate_array(*array, new_array_size);
		}

		/* Add elements at the end */
		element_access = *array;
		memcpy(&element_access[array_size], new_elements, sizeof(int)*new_element_count);
	}
	return array_size + new_element_count;
}


long int append_array_element_long(long int element, long int **array, long int element_count)
/* This function adds an element to the end of the array. If need be the array
 * will be resized. If the array was NULL to begin with the memory will be
 * allocated to it, because realloc behaves like malloc when a NULL pointer
 * is passed to it as parameter. This function returns the new size of the array.
 *
 * NOTE: This is the long int version of the function.
 */
{
	long int new_element_count;
	long int *element_access;

	assert(element_count >= 0);
	assert(array != NULL);

	/* Check array element count */
	new_element_count = calculate_array_size_using_bounds(element_count+1);

	if ((*array == NULL) || (new_element_count > calculate_array_size_using_bounds(element_count)))
	{
		*array = (long int *) realloc(*array, new_element_count*sizeof(long int));
		assert(*array != NULL);
	}

	/* Add element at the end */
	element_access = *array;
	element_access[element_count] = element;

	return element_count + 1;
}


int remove_element_by_index(int element_index, int *array, int element_count)
/* This function removes the element from the array, given this elements index.
 * At the same time it moves part of the array so that the gap created by the deleted
 * element is covered, if such a gap was created.
 */
{
	int index;

	assert(array != NULL);
	assert(element_count > 0);

	if (element_count - 1 != element_index)
	{
		/* Copy over unchanged elements. */
		for(index = element_index; index < element_count - 1; index++)
		{
			array[index] = array[index+1];
		}
	}
	/* If this was the last element then no work needed to be done. If it was not
	 * the last element then the new array element count is still decreased by 1. */
	return element_count - 1;
}


int get_element_index(int element, int *array, int element_count)
/* This function returns the index of the element specified.
 */
{
	int index = -1;

	if ((array != NULL) && (element_count > 0))
	{
		/* Search for element */
		for(index = 0; index < element_count; index++)
		{
			if (array[index] == element)
			{
				break;
			}
		}

		if (index >= element_count)
		{
			index = -1;
		}
	}
	return index;
}


t_boolean is_element_in_array(int element, int *array, int element_count)
/* This function searches for the element in the array. If found the function returns
 * T_TRUE and T_FALSE otherwise.
 */
{
	t_boolean result = T_FALSE;
	int index;

	if ((array != NULL) && (element_count > 0))
	{
		for(index = 0; index < element_count; index++)
		{
			if (array[index] == element)
			{
				result = T_TRUE;
				break;
			}
		}
	}
	return result;
}


int remove_element_by_content(int element_content, int *array, int element_count)
/* This function removes the element from the array, given this elements content.
 * It first finds the index of the element in the array then uses the remove_element_by_index
 * function to actually remove the element.
 */
{
	int index;
	int new_element_count = element_count;

	if ((array != NULL) && (element_count > 0))
	{
		index = get_element_index(element_content, array, element_count);

		if (index >= 0)
		{
			/* Element found */
			new_element_count = remove_element_by_index(index, array, element_count);
		}
	}
	else
	{
		new_element_count = 0;
	}
	return new_element_count;
}


long int calculate_array_size_using_bounds(long int element_count)
/* This function calculates the size of the array given the element count,
 * and upper and lower bounds.
 */
{
	long int power_count = 0;
	long int array_size = element_count;

	assert(array_size >= 0);

	/* First find the size of the array with respect to the bounds. */
	if (array_size <= MINIMUM_ARRAY_SIZE)
	{
		array_size = MINIMUM_ARRAY_SIZE;
	}
	else
	{
		if (array_size > UPPER_GROWTH_BOUND)
		{
			array_size = UPPER_GROWTH_BOUND*((array_size/UPPER_GROWTH_BOUND) + 1);
		}
		else
		{
			/* The element count is between the upper and lower bounds */
			while(array_size > 0)
			{
				array_size = array_size >> 1;
				power_count++;
			}
			power_count++;

			array_size = 1 << power_count;
		}
	}
	return array_size;
}
