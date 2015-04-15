/*************************************************************************
 * Created: Ter 01 Fev 2011 22:50:55 BRST
 *
 * Author: Carla N. Lintzmayer, carla0negri@gmail.com
 *
 *************************************************************************/




#include <stdlib.h>
#include "util.h"
#include <sys/resource.h>





void* malloc_(size_t size) {/*{{{*/
    void *p;
	p = malloc(size);

    if (!p) {
        perror("malloc");
        abort();
    }

    return p;
}/*}}}*/

double current_usertime_secs(void) {/*{{{*/
    double usertime, systemtime;
    struct rusage usage;

    if (getrusage(RUSAGE_SELF, &usage) < 0) {
        perror("get rusage");
        return -1;
    }

    usertime = usage.ru_utime.tv_sec + (usage.ru_utime.tv_usec * 1e-6);
    systemtime = usage.ru_stime.tv_sec + (usage.ru_stime.tv_usec * 1e-6);

    return (usertime + systemtime);
}/*}}}*/

long int create_seed(void) {/*{{{*/
    long int gseed;
    struct timeval tval;

    if (gettimeofday(&tval, NULL) < 0) {
        perror("get time of day");
        return 1;
    }

    gseed = tval.tv_sec * tval.tv_usec;

    return gseed;
}/*}}}*/


