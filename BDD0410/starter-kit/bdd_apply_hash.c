#include <stdlib.h>
//#include <string.h>
//#include <assert.h>
//#include <malloc.h>
#include "bdd_apply_hash.h"

apply_hash_table *apply_create_hash_table(int size)
{
	apply_hash_table *new_table;
	int i;

	if (size < 1) 
		return NULL; /* invalid size for table */

	/* Attempt to allocate memory for the table structure */
	if ((new_table = (apply_hash_table *)malloc(sizeof(apply_hash_table))) == NULL) {
		return NULL;
	}

	/* Attempt to allocate memory for the table itself */
	new_table->ptrs = (apply_hash_item_ptr *)malloc(sizeof(apply_hash_item_ptr) * size);

	/* Initialize the elements of the table */
	for(i=0; i<size; i++) 
		new_table->ptrs[i] = NULL;

	/* Set the table's size */
	new_table->size = size;

	return new_table;
}

void apply_free_hash_table(apply_hash_table *table)
{
	int i;
	apply_hash_item *cur_item, *next_item;

	if (table == NULL) 
		return;

	for(i = table->size - 1; i >= 0; i--)
	{
		cur_item = table->ptrs[i];
		while (cur_item != NULL) 
		{
			next_item = cur_item->next;
			free(cur_item);
			cur_item = next_item;
		}
	}

	free(table->ptrs);
	free(table);
}

// apply_hash apply_get_hash(hash_table *table, apply_hash_key *key)
apply_hash apply_get_hash(apply_hash_table *table, bdd_node *node1, bdd_node *node2)
{
	// Using nodes addresses/values: 000aaaa0
	//                               bbbb0000  
	return ((((apply_hash)node1 >> 4) & 0xFFFFF) ^
		      (((apply_hash)node2 & (~0xF)) << 12)) % (apply_hash)table->size;
}

bdd_node *apply_hash_search(apply_hash_table *table, bdd_node *node1, bdd_node *node2)
{
	apply_hash cell_index = apply_get_hash(table, node1, node2);
	apply_hash_item *cur_item = table->ptrs[cell_index];

	while (cur_item != NULL)
	{
		if (cur_item->key.nodes[0] == node1 && cur_item->key.nodes[1] == node2)
			return cur_item->value_node;
		else
			cur_item = cur_item->next;
	}
	return NULL;
}

void apply_hash_insert(apply_hash_table *table, bdd_node *node1, bdd_node *node2, 
	                     bdd_node *result_node)
{
  apply_hash_item *new_item = (apply_hash_item *)malloc(sizeof(apply_hash_item));
	apply_hash cell_index = apply_get_hash(table, node1, node2);

	if (new_item == NULL)
	{
		bdd_report_error("Not enough memory for hash item");
		return;
	}

	new_item->key.nodes[0] = node1;
	new_item->key.nodes[1] = node2;
	new_item->value_node = result_node;
	new_item->next = table->ptrs[cell_index];
	table->ptrs[cell_index] = new_item;
}
    