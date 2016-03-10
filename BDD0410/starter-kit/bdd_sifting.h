#if !defined(BDD_SIFTING_HEADER)
#define BDD_SIFTING_HEADER

#include "bdd.h"

void bdd_swap_variables(bdd *diagram, int first_var_index);
              // Swaps first_var_index and first_var_index + 1 variables
              // TODO? speed up - to preserve and keep updated the diagram divided by levels
int bdd_do_sifting(bdd *diagram);
              // Returns by how much nodes it was able to reduce the diagram

#endif