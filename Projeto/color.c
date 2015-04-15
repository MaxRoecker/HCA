/*************************************************************************
 * Created: Seg 7 Fev 2011 16:36:24 BRST
 *
 * Author: Carla N. Lintzmayer, carla0negri@gmail.com
 *
 *************************************************************************
 * 
 *
 *************************************************************************/
#include <stdio.h>
#include <math.h>
#define _GNU_SOURCE
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include "color.h"
#include "util.h"
#include "hca.h"
#include "tabucol.h"

static char *namefilein;

void show_help(char *nameprog) {
  fprintf(problem->fileout, "Usage: %s [options] <file>\n", nameprog);

  fprintf(problem->fileout, "  [-c, --cycles    ] <value>\tDefine maximum number of cycles as <value>. Default: %d.\n", HCA_DEFAULT_CYC);
  fprintf(problem->fileout, "  [-t, --maxtime   ] <value>\tDefine maximum time of execution as <value>. Default: %d.\n", DEFAULT_TIME);
  fprintf(problem->fileout, "  [-g, --converg   ] <value>\tDefine maximum number of cycles without improvement as <value>. Default: %d.\n", DEFAULT_CONV);

  fprintf(problem->fileout, "  [-p, --population] <value>\tDescription: Define size of population as <value>. Default: %d.\n", HCA_DEFAULT_POP);
  fprintf(problem->fileout, "  [-l, --ls_cyc    ] <value>\tDescription: Define maximum of local search cycles without improvement. Default: %d.\n", HCA_DEFAULT_LS);
  fprintf(problem->fileout, "  [-k, --colors    ] <value>\tDefine maximum number of colors as <value>. Default: number of vertices.\n");    

  fprintf(problem->fileout, "  [-r, --reactive  ] \t\tDefine a reactive scheme for tabu tenure. Default: dynamic scheme.\n");

  fprintf(problem->fileout, "  [-s, --seed      ] <value>\tDefine <value> as the seed of rand function. Default: random.\n");    

  fprintf(problem->fileout, "  [-o, --output    ] <file>\tPlace the output into <file>. Default: stdout.\n");
  fprintf(problem->fileout, "  [-v, --verbose   ] \t\tDisplay informations during execution\n");
  fprintf(problem->fileout, "  [-h, --help      ] \t\tDisplay this information\n\n");

}

void parseargs(int argc, char *argv[]) {

  extern char *optarg;
  char op;
  problem->algorithm = -1;

  /* Usando getopt para tratamento dos argumentos */
  struct option longopts[] = {

    {"maxcycles",	1, NULL, 'c'},
    {"maxtime",		1, NULL, 't'},
    {"converge",        1, NULL, 'g'},
    {"population",	1, NULL, 'p'},
    {"ls_cyc",	        1, NULL, 'l'},
    {"maxcolors",	1, NULL, 'k'},
    {"reative",         0, NULL, 'r'},
    {"seed",	        1, NULL, 's'},
    
    {"output",		1, NULL, 'o'},
    {"verbose",		0, NULL, 'v'},
    {"help",		0, NULL, 'h'},
  };

  while ((op = getopt_long(argc, argv, "c:t:g:p:l:k:rs:o:vh", longopts, NULL)) != -1) {
    switch (op) {
    case 'c':
      problem->max_cycles = atoi(optarg);
      set_flag(problem->flags, FLAG_CYCLE);
      break;
    case 't':
      problem->max_time = atof(optarg);
      set_flag(problem->flags, FLAG_TIME);
      break;   
    case 'g':
      problem->max_cyc_converg = atoi(optarg);
      set_flag(problem->flags, FLAG_CONV);
      break;
    case 'p':
      hca_info->sizeof_population = atoi(optarg);
      break;
    case 'l':
      hca_info->cyc_local_search = atoi(optarg);
      break;
    case 'k':
      problem->max_colors = atoi(optarg);
      set_flag(problem->flags, FLAG_COLOR);
      break;
    case 'r':
      tabucol_info->tl_style = TABUCOL_REACTIVE;
      break;
    case 's':
      hca_info->seed = atoi(optarg);
      tabucol_info->seed = atoi(optarg);
      set_flag(problem->flags, FLAG_SEED);
      break;
    case 'o':
      problem->fileout = fopen(optarg, "w");
      break;
    case 'v':
      set_flag(problem->flags, FLAG_VERBOSE);
      break;
    case 'h':
      show_help(argv[0]);
      exit(0);
    }

  }

  
  /* O único argumento não capturado acima é o nome do arquivo de entrada,
   * se existir */
  if (optind < argc) {
    namefilein = malloc_(sizeof(char) * strlen(argv[optind])+1);
    strcpy(namefilein, argv[optind++]);

    /* verificar se foi passado algum argumento a mais */
    if (optind < argc) {
      printf("error: invalid argument. Use '-h'\n");
      exit(0);
    }
  }
  else {
    printf("error: no input files\n");
    exit(0);
  }

}

void initialization(void) {

  FILE *in;
	
  int i, j, vi, vj;
  char f, t[50];
	
  problem->max_time = get_flag(problem->flags, FLAG_TIME) ? problem->max_time : DEFAULT_TIME;
  problem->max_cycles = get_flag(problem->flags, FLAG_CYCLE) ? problem->max_cycles : HCA_DEFAULT_CYC;
  problem->max_cyc_converg = get_flag(problem->flags, FLAG_CONV) ? problem->max_cyc_converg : DEFAULT_CONV;

  set_flag(problem->flags, FLAG_TIME);
  set_flag(problem->flags, FLAG_CYCLE);
  set_flag(problem->flags, FLAG_CONV);

  set_flag(problem->flags, FLAG_S_ASSIGN);
  set_flag(problem->flags, FLAG_S_PARTITION);
  set_flag(problem->flags, FLAG_ADJ_MATRIX);
  
  hca_info->sizeof_population = hca_info->sizeof_population ? hca_info->sizeof_population : HCA_DEFAULT_POP;
  hca_info->cyc_local_search = hca_info->cyc_local_search ? hca_info->cyc_local_search : HCA_DEFAULT_LS;

  tabucol_info->tl_style = tabucol_info->tl_style ? tabucol_info->tl_style : TABUCOL_DYNAMIC;

  in = fopen(namefilein, "r");
  if (!in) {
    printf("error: no input files\n");
    exit(0);
  }

  /* Ignoring initial informations */
  while ((j = fscanf(in, "%c", &f)) && f != 'p') {
    while (f != '\n') {
      j = fscanf(in, "%c", &f);
    }
  }

  j = fscanf(in, "%s %d %d\n", t, &problem->nof_vertices, &problem->nof_edges);
  problem->degree = malloc_(sizeof(int) * problem->nof_vertices);
  if (get_flag(problem->flags, FLAG_ADJ_MATRIX)) {
    problem->adj_matrix = malloc_(sizeof(int*) * problem->nof_vertices);
  }
  if (get_flag(problem->flags, FLAG_ADJ_LIST)) {
    problem->adj_list = malloc_(sizeof(int*) * problem->nof_vertices);
  }

  for (i = 0; i < problem->nof_vertices; i++) {

    if (get_flag(problem->flags, FLAG_ADJ_MATRIX)) {
      problem->adj_matrix[i] = malloc_(sizeof(int) * problem->nof_vertices);
    }
    if (get_flag(problem->flags, FLAG_ADJ_LIST)) {
      problem->adj_list[i] = malloc_(sizeof(int) * (problem->nof_edges+1));
    }

    for (j = 0; j < problem->nof_vertices; j++) {
		
      if (get_flag(problem->flags, FLAG_ADJ_MATRIX)) {
	problem->adj_matrix[i][j] = 0;
      }
      if (get_flag(problem->flags, FLAG_ADJ_LIST)) {
	problem->adj_list[i][j] = 0;
      }
    }

    if (get_flag(problem->flags, FLAG_ADJ_LIST)) {
      problem->adj_list[i][problem->nof_vertices] = 0;
    }

    problem->degree[i] = 0;
  }

  for (i = 0; i < problem->nof_edges; i++) {
    j = fscanf(in, "%c %d %d\n", &f, &vi, &vj);

    if (get_flag(problem->flags, FLAG_ADJ_MATRIX)) {
      problem->adj_matrix[vi-1][vj-1] = 1;
      problem->adj_matrix[vj-1][vi-1] = 1;
    }

    if (get_flag(problem->flags, FLAG_ADJ_LIST)) {
      problem->adj_list[vi-1][0]++;
      problem->adj_list[vi-1][problem->adj_list[vi-1][0]] = vj-1;
      problem->adj_list[vj-1][0]++;
      problem->adj_list[vj-1][problem->adj_list[vj-1][0]] = vi-1;
    }

    problem->degree[vi-1]++;
    problem->degree[vj-1]++;
  }

  fclose(in);

  hca_initialization();
  tabucol_initialization();


}

void printbanner(void) {

  fprintf(problem->fileout, "-------------------------------------------------\n");		
  hca_printbanner();
  fprintf(problem->fileout, "-------------------------------------------------\n");
  tabucol_printbanner();
  fprintf(problem->fileout, "-------------------------------------------------\n");
  fprintf(problem->fileout, "Max time......................: %lf\n", problem->max_time);
  fprintf(problem->fileout, "Max cycles....................: %d\n", problem->max_cycles);
  fprintf(problem->fileout, "Max cycles without improvement: %d\n", problem->max_cyc_converg);
  if (problem->flags & FLAG_VERBOSE)
    fprintf(problem->fileout, "Running on Verbose mode\n");
  fprintf(problem->fileout, "-------------------------------------------------\n");	

}



void test_map(gcp_solution_t *solution) {
  int i, j, n;
  int confs = 0;
  printf("\n");
  for (i = 0; i < problem->nof_vertices; i++) {
    //printf("color of %d: %d\n", i+1, solution->color_of[i]);
    if (get_flag(problem->flags, FLAG_ADJ_MATRIX)) {
      for (j = i; j < problem->nof_vertices; j++) {
	if (problem->adj_matrix[i][j] && 
	    solution->color_of[i] == solution->color_of[j]) {
	  //	printf("ERROR!! Conflicting edge %d--%d \n", i+1, j+1);
	  confs++;
	}
      }
    }
    else {
      for (j = 1; j <= problem->adj_list[i][0]; j++) {
	n = problem->adj_list[i][j];
	if (solution->color_of[i] == solution->color_of[n]) {
	  //	printf("ERROR!! Conflicting edge %d--%d \n", i+1, n+1);
	  confs++;
	}
      }
    }
  }
  if (confs != solution->nof_confl_edges) {
    printf("ERROR!! Confl edges = %d; Calculated = %d\n", confs, solution->nof_confl_edges);
  }
  printf("\n");
}

void cpy_solution(gcp_solution_t *src, gcp_solution_t *dst) {

  int i, j;
  if (get_flag(problem->flags, FLAG_S_ASSIGN)) {
    for (i = 0; i < problem->nof_vertices; i++) {
      dst->color_of[i] = src->color_of[i];
    }
  }
  if (get_flag(problem->flags, FLAG_S_PARTITION)) {
    for (i = 0; i <= problem->nof_vertices; i++) {
      for (j = 0; j < problem->max_colors; j++)
	dst->class_color[j][i] = src->class_color[j][i];
    }
  }

  dst->spent_time				= src->spent_time;
  dst->time_to_best			= src->time_to_best;
  dst->total_cycles			= src->total_cycles;
  dst->cycles_to_best			= src->cycles_to_best;
  dst->nof_colors				= src->nof_colors;
  dst->nof_confl_edges		= src->nof_confl_edges;
  dst->nof_confl_vertices		= src->nof_confl_vertices;
  dst->nof_uncolored_vertices	= src->nof_uncolored_vertices;
  dst->stop_criterion			= src->stop_criterion;

}

void show_solution(gcp_solution_t *solution) {
  fprintf(problem->fileout, "-------------------------------------------------\n");
  fprintf(problem->fileout, "SOLUTION:\n");
  fprintf(problem->fileout, "-------------------------------------------------\n");
  fprintf(problem->fileout, "No. of colors utilized: %d\n", solution->nof_colors);
  fprintf(problem->fileout, "No. of conflicting edges: %d\n", solution->nof_confl_edges);
  fprintf(problem->fileout, "No. of conflicting vertices: %d\n", solution->nof_confl_vertices);
  fprintf(problem->fileout, "No. of uncolored vertices: %d\n", solution->nof_uncolored_vertices);
  fprintf(problem->fileout, "Real time: %lf\n", problem->real_time);
  fprintf(problem->fileout, "Spent time: %lf\n", solution->spent_time);
  fprintf(problem->fileout, "Time to the best: %lf\n", solution->time_to_best);
  fprintf(problem->fileout, "Total of cycles: %d\n", solution->total_cycles);
  fprintf(problem->fileout, "Cycles to the best: %d\n", solution->cycles_to_best);
  fprintf(problem->fileout, "Stop criterion: %d\n", solution->stop_criterion);

  hca_show_solution();

  fprintf(problem->fileout, "-------------------------------------------------\n");
  test_map(solution);
}

gcp_solution_t* init_solution(void) {
  int i;
  gcp_solution_t *solution;

  solution = malloc_(sizeof(gcp_solution_t));

  if (get_flag(problem->flags, FLAG_S_ASSIGN)) {
    solution->color_of = malloc_(sizeof(int) * problem->nof_vertices);
  }
  if (get_flag(problem->flags, FLAG_S_PARTITION)) {
    solution->class_color = malloc_(sizeof(int*) * problem->max_colors);
    for (i = 0; i < problem->max_colors; i++) {
      solution->class_color[i] = malloc_(sizeof(int) * (problem->nof_vertices+1));
    }
  }

  solution->nof_colors = 0;
  solution->total_cycles = 0;
  solution->cycles_to_best = 0;
  solution->nof_confl_edges = 0;
  solution->nof_confl_vertices = 0;
  solution->nof_uncolored_vertices = 0;
  solution->stop_criterion = -1;

  return solution;
}

gcp_solution_t* find_solution() {

  gcp_solution_t* sol = NULL;

  sol = hca();

  return sol;

}

int terminate_conditions(gcp_solution_t *solution, int cycle, int converg) {

  if (get_flag(problem->flags, FLAG_CONV) && 
      converg >= problem->max_cyc_converg) {
    solution->stop_criterion = STOP_CONV;
    return TRUE;
  }
  else if (get_flag(problem->flags, FLAG_CYCLE) && 
	   cycle >= problem->max_cycles) {
    solution->stop_criterion = STOP_CYCLES;
    return TRUE;
  }
  else if (get_flag(problem->flags, FLAG_TIME) && 
	   current_usertime_secs() >= problem->max_time) {
    solution->stop_criterion = STOP_TIME;
    return TRUE;
  }
  return FALSE;
	
}

int main(int argc, char *argv[]) {

  struct timeval time;
  double time1, time2;

  gcp_solution_t *results;

  gettimeofday(&time, NULL);
  time1 = time.tv_sec + (time.tv_usec/1000000.0);

  problem = malloc_(sizeof(gcp_t));
  init_flag(problem->flags);
  problem->fileout = stdout;
  problem->nof_vertices = 0;             
  problem->nof_edges = 0;                        
  problem->max_cycles = 0;                       
  problem->max_cyc_converg = 0;  
  problem->max_time = 0;              
  problem->real_time = 0;
  problem->max_colors = 0;                       
  problem->flags = 0;                            
  problem->degree = 0;                  
  problem->adj_matrix = 0;             
  problem->adj_list = 0;  

  hca_info = malloc_(sizeof(hca_t));
  hca_info->sizeof_population = 0;
  hca_info->cyc_local_search = 0;
  hca_info->seed = 0;
  hca_info->nof_cross = 0;
  hca_info->diversity = 0;

  tabucol_info = malloc_(sizeof(tabucol_t));
  tabucol_info->seed = 0;
  tabucol_info->tl_style = 0;

  parseargs(argc, argv);
	  	
  initialization();
	
  printbanner();

  results = find_solution();
	
  gettimeofday(&time, NULL);
  time2 = time.tv_sec + (time.tv_usec/1000000.0);

  problem->real_time = time2 - time1;

  show_solution(results);

  fclose(problem->fileout);
	
  return 0;

}

