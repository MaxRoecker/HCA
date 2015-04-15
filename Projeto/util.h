/*************************************************************************
 * Created: Ter 01 Fev 2011 22:59:46 BRST
 *
 * Author: Carla N. Lintzmayer, carla0negri@gmail.com
 *
 *************************************************************************/




#ifndef __UTIL_H
#define __UTIL_H


#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#define RANDOM(limit) (lrand48() % (limit))
#define RANDOM_UNIT() (lrand48())

#define bool int
#define TRUE 1
#define FALSE 0


void* malloc_(size_t size);
double current_usertime_secs(void);
long int create_seed(void);




#endif /* __UTIL_H */
