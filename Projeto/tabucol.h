/**********************************************************
 * Created: Seg 08 Ago 2011 17:22:32 BRT
 *
 * Author: Carla N. Lintzmayer, carla0negri@gmail.com
 *
 **********************************************************
 *
 * Local search Tabucol [1987, Hertz]
 * Works with infeasible k-colorings.
 * Can be used with either a react or a dynamic tabu tenure.
 *
 ***********************************************************/
#ifndef __TABUCOL_H
#define __TABUCOL_H

#include "color.h"

#define TABUCOL_DYNAMIC 1
#define TABUCOL_REACTIVE 2

struct tabucol_t {
  int seed;
  int tl_style;
};

typedef struct tabucol_t tabucol_t;

tabucol_t *tabucol_info;

void tabucol_printbanner(void);
void tabucol_initialization(void);
void tabucol(gcp_solution_t *solution, int max_cycles, int type_of_tl);

#endif /* __TABUCOL_H */

