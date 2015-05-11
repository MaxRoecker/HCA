/**********************************************************
 * Created: Seg 08 Ago 2011 16:20:54 BRT
 *
 * Author: Carla N. Lintzmayer, carla0negri@gmail.com
 *
 **********************************************************/
#ifndef __COLOR_H
#define __COLOR_H

/* Define kind of stop criterion (to be used) */
#define FLAG_TIME		1
#define FLAG_CYCLE		2
#define FLAG_CONV		4
#define FLAG_VERBOSE   	        8
#define FLAG_COLOR		16
#define FLAG_SEED		32

/* Define kind of structure of adjacency */
#define FLAG_ADJ_MATRIX		64
#define FLAG_ADJ_LIST		128
/* Define kind of solution: an assignment of colors to vertices or a partition
 * of vertices into color classes */
#define FLAG_S_ASSIGN		256
#define FLAG_S_PARTITION	512

#define init_flag(flag) flag = 0
#define set_flag(flag, pos) flag = flag | pos
#define get_flag(flag, pos) flag & pos

#define DEFAULT_TIME	3600
#define DEFAULT_CONV	500

/* Define kind of stop criterion (when the run is finished) */
#define STOP_CYCLES	1
#define STOP_TIME	2
#define STOP_BEST	3
#define STOP_CONV	4
#define STOP_ALG	5 // specific for some algorithms

/*###*/
#define BUFFER_SIZE 1
/*###*/

struct gcp_t {
  int nof_vertices;		// number of vertices
  int nof_edges;			// number of edges
  int max_cycles;			// maximum number of cycles
  int max_cyc_converg;	// maximum number of cycles without improvements
  double max_time;		// maximum time
  int max_colors;			// maximum of colors (value for k)
  int flags;				// flags related to the stop conditions
  int *degree;			// keeps the degree of each vertex
  int **adj_matrix;		// adjacency matrix
  int **adj_list;			// adjacency list
  int algorithm;			// indicates the algorithm being used
  double real_time;
  FILE *fileout;
};

struct gcp_solution_t {
  int *color_of;				// (assignment solution) map vertex->color
  int **class_color;			// (partition solution) color classes
  double spent_time;			// total time spent
  double time_to_best;		// time to the best so far
  int total_cycles;			// total number of cycles
  int cycles_to_best;			// cycles to the best so far
  int nof_colors;				// number of colors used
  int nof_confl_edges;		// number of conflicting edges
  int nof_confl_vertices;		// number of conflicting vertices
  int nof_uncolored_vertices;	// number of uncolored vertices
  int stop_criterion;			// type of stop criterion
};


typedef struct gcp_t gcp_t;
typedef struct gcp_solution_t gcp_solution_t;

gcp_t *problem;

gcp_solution_t* find_solution(void);
void show_solution(gcp_solution_t *solution);
gcp_solution_t* init_solution(void);
int terminate_conditions(gcp_solution_t *solution, int cycle, int converg);
void cpy_solution(gcp_solution_t *src, gcp_solution_t *dst);

 
#endif /* __COLOR_H */

