/***********************************************************
 * Created: Ter 09 Ago 2011 21:05:59 BRT
 *
 * Author: Carla N. Lintzmayer, carla0negri@gmail.com
 *
 ***********************************************************
 *
 * HCA [1999, , Galinier and Hao]
 *
 * Hybrid Coloring Algorithm.
 * Utilizing GPX crossover.
 *
 ***********************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <pthread.h>
#include <semaphore.h>
#include <bits/stdio.h>
#include "color.h"
#include "util.h"
#include "tabucol.h"
#include "hca.h"

/* ### */
extern gcp_solution_t *bufferCrossover[BUFFER_SIZE];
extern int indexBufferCrossover;

static gcp_solution_t *crossoverReturn;
pthread_t crossOverThread;
pthread_t tabucolThreads[BUFFER_SIZE];
pthread_t updatePopulationThread;

sem_t semFiller;
sem_t semEmptier;
sem_t semUpdater;
pthread_barrier_t barrier;

/* ### */

static gcp_solution_t **population;
static gcp_solution_t *best_solution;
static gcp_solution_t *offspring;

void hca_printbanner(void) {
    fprintf(problem->fileout, "HCA\n");
    fprintf(problem->fileout, "-------------------------------------------------\n");
    fprintf(problem->fileout, "Using Parameters:\n");
    fprintf(problem->fileout, "  Population size..: %d\n", hca_info->sizeof_population);
    fprintf(problem->fileout, "  Max LS iter......: %d\n", hca_info->cyc_local_search);
    fprintf(problem->fileout, "  Max colors.......: %i\n", problem->max_colors);
    fprintf(problem->fileout, "  Seed.............: %i\n", hca_info->seed);
}

void hca_initialization(void) {
    if (!(get_flag(problem->flags, FLAG_COLOR)))
        problem->max_colors = problem->nof_vertices;

    if (!(get_flag(problem->flags, FLAG_SEED)))
        hca_info->seed = create_seed();

    srand48(hca_info->seed);
}

void hca_show_solution(void) {
    fprintf(problem->fileout, "Nof. crossovers realized: %d\n", hca_info->nof_cross);
    fprintf(problem->fileout, "Diversity in the final population: %d\n", hca_info->diversity);
}

static void test_solution(gcp_solution_t* sol) {
    int i, j, v;
    for (i = 0; i < problem->max_colors; i++) {
        for (j = 1; j <= sol->class_color[i][0]; j++) {
            v = sol->class_color[i][j];
            if (i != sol->color_of[v]) {
                printf(" ERROR!! %d está na classe %d, mas cor de %d = %d \n", v + 1, i, v + 1, sol->color_of[v]);
            }
        }
    }
}

static gcp_solution_t* create_indiv(void) {

    gcp_solution_t *solution = init_solution();
    int i, j, c, v, color, nc, v_max_degree;
    int possible_color[problem->nof_vertices];
    //    int confl_vertices[problem->nof_vertices];
    int neighbors_by_color[problem->nof_vertices][problem->max_colors + 1];

    /* Initializing auxiliary arrays and choosing a vertex with a maximal degree
     * to be the first one */
    v_max_degree = 0;
    for (i = 0; i < problem->nof_vertices; i++) {
        possible_color[i] = 0;
        solution->color_of[i] = -1;
        //		confl_vertices[i] = 0;
        for (j = 0; j < problem->max_colors; j++) {
            neighbors_by_color[i][j] = 0;
            solution->class_color[j][i] = -1;
        }
        neighbors_by_color[i][problem->max_colors] = 0;
        if (problem->degree[i] > problem->degree[v_max_degree]) {
            v_max_degree = i;
        }
    }
    for (j = 0; j < problem->max_colors; j++) {
        solution->class_color[j][problem->nof_vertices] = -1;
        solution->class_color[j][0] = 0;
    }

    v_max_degree = 0;
    /* Color the chosen vertex with the first color (0) */
    color = 0;
    solution->color_of[v_max_degree] = color;
    solution->class_color[color][0]++;
    solution->class_color[color][solution->class_color[color][0]] = v_max_degree;
    v = v_max_degree; /* the current vertex, last one that was colored */
    nc = 1; /* number of colored vertices */

    while (nc < problem->nof_vertices) {

        /* Update degree of saturation and possible colors; choose vertex with
         * maximal saturation degree */
        for (i = 0; i < problem->nof_vertices; i++) {
            if (problem->adj_matrix[v][i]) {
                /* update degree of saturation: */
                if (neighbors_by_color[i][color] == 0) {
                    neighbors_by_color[i][problem->max_colors]++;
                }
                /* now <i> has a neighbor colored with <color> */
                neighbors_by_color[i][color]++;

                if (solution->color_of[i] == -1) {
                    /* if <i> is not colored yet and <i> is neighbor of <v>,
                     * update possible color for <i>: among all the possible 
                     * colors for a neighbor of <i>, chose the least one */
                    int changed = FALSE;
                    for (c = problem->max_colors; c >= 0; c--) {
                        if (neighbors_by_color[i][c] == 0) {
                            possible_color[i] = c;
                            changed = TRUE;
                        }
                    }
                    if (!changed) possible_color[i] = problem->max_colors;
                }
            }
        }

        v_max_degree = -1;
        for (i = 0; i < problem->nof_vertices; i++) {
            /* choose vertex with a maximal saturation degree: */
            if (solution->color_of[i] == -1) {
                if (v_max_degree == -1) v_max_degree = i;
                else if (neighbors_by_color[i][problem->max_colors] >
                        neighbors_by_color[v_max_degree][problem->max_colors]) {
                    v_max_degree = i;
                }
            }
        }

        v = v_max_degree;
        color = possible_color[v];

        /* if no viable color is found for <v>, chose a random one.
         * this means that a conflict is being generated. */
        if (color == problem->max_colors) {
            color = (int) RANDOM(problem->max_colors);
        }

        solution->color_of[v] = color;
        solution->class_color[color][0]++;
        solution->class_color[color][solution->class_color[color][0]] = v;
        nc++;

    }

    solution->spent_time = current_usertime_secs();
    solution->time_to_best = solution->spent_time;

    test_solution(solution);

    return solution;

}

static void create_population(void) {

    int i;
    population = malloc_(sizeof (gcp_solution_t*) * hca_info->sizeof_population);
    best_solution->nof_confl_edges = INT_MAX;

    for (i = 0; i < hca_info->sizeof_population; i++) {
        population[i] = create_indiv();
        population[i]->cycles_to_best = 0;
        population[i]->nof_colors = problem->max_colors;
        tabucol(population[i], hca_info->cyc_local_search, tabucol_info->tl_style);
        population[i]->time_to_best = current_usertime_secs();

        if (population[i]->nof_confl_edges < best_solution->nof_confl_edges) {
            cpy_solution(population[i], best_solution);
        }
    }

}

static void choose_parents(int *p1, int *p2) {
    (*p1) = (int) RANDOM(hca_info->sizeof_population);
    (*p2) = (int) RANDOM(hca_info->sizeof_population);
    while ((*p2) == (*p1)) {
        (*p2) = (int) RANDOM(hca_info->sizeof_population);
    }
}

static void crossover(int p1, int p2, int posBuffer) {
    /*####*/
    int color, parent, max, i, j, c, v, otherparent, p;
    int class_colors[2][problem->max_colors][problem->nof_vertices + 1];

    bufferCrossover[posBuffer]->nof_colors = problem->max_colors;
    for (i = 0; i < problem->nof_vertices; i++) {
        bufferCrossover[posBuffer]->color_of[i] = -1;
        for (c = 0; c < problem->max_colors; c++) {
            class_colors[0][c][i] = population[p1]->class_color[c][i];
            class_colors[1][c][i] = population[p2]->class_color[c][i];
            bufferCrossover[posBuffer]->class_color[c][i] = -1;
        }
    }
    for (c = 0; c < problem->max_colors; c++) {
        bufferCrossover[posBuffer]->class_color[c][0] = 0;
        bufferCrossover[posBuffer]->class_color[c][problem->nof_vertices] = -1;
        class_colors[0][c][problem->nof_vertices] =
                population[p1]->class_color[c][problem->nof_vertices];
        class_colors[1][c][problem->nof_vertices] =
                population[p2]->class_color[c][problem->nof_vertices];
    }

    for (color = 0; color < problem->max_colors; color++) {
        parent = (color % 2) ? 0 : 1; // 0 equals p1 and 1 equals p2
        otherparent = (parent == 0) ? 1 : 0;

        /* choose <max> such as C_max of <parent> is maximal */
        max = 0;
        for (i = 1; i < problem->max_colors; i++) {
            if (class_colors[parent][i][0] > class_colors[parent][max][0]) {
                max = i;
            }
        }

        /* C_color gets C_max from <parent> */
        for (i = 1; i <= class_colors[parent][max][0]; i++) {

            v = class_colors[parent][max][i];
            class_colors[parent][max][i] = -1;

            bufferCrossover[posBuffer]->class_color[color][0]++;
            bufferCrossover[posBuffer]->class_color[color][bufferCrossover[posBuffer]->class_color[color][0]] = v;
            bufferCrossover[posBuffer]->color_of[v] = color;

            /* Remove all vertices of <otherparent> */
            p = (otherparent == 0) ? p1 : p2;
            c = population[p]->color_of[v];
            for (j = 1; j <= class_colors[otherparent][c][0]; j++) {
                if (class_colors[otherparent][c][j] == v) {
                    class_colors[otherparent][c][j] =
                            class_colors[otherparent][c][class_colors[otherparent][c][0]];
                    class_colors[otherparent][c][class_colors[otherparent][c][0]] = -1;
                    class_colors[otherparent][c][0]--;
                    break;
                }
            }
        }

        /* Remove all vertices in C_color of <parent> */
        class_colors[parent][max][0] = 0;

    }

    /* Assign randomly the vertices of V - (C_1 U ... U C_k) */
    for (i = 0; i < problem->nof_vertices; i++) {
        if (bufferCrossover[posBuffer]->color_of[i] == -1) {
            c = (int) RANDOM(problem->max_colors);
            bufferCrossover[posBuffer]->color_of[i] = c;
            bufferCrossover[posBuffer]->class_color[c][0]++;
            bufferCrossover[posBuffer]->class_color[c][bufferCrossover[posBuffer]->class_color[c][0]] = i;
        }
    }

    test_solution(population[p1]);
    test_solution(population[p2]);
    test_solution(bufferCrossover[posBuffer]);
    /*####*/
}

static int substitute_worst(int p1, int p2, gcp_solution_t* offspring) {/*{{{*/
    if (population[p1]->nof_confl_edges > population[p2]->nof_confl_edges) {
        cpy_solution(offspring, population[p1]);
        return p1;
    }

    cpy_solution(offspring, population[p2]);
    return p2;
}

static int distance(gcp_solution_t *ind1, gcp_solution_t *ind2) {/*{{{*/
    int i, dist = 0;

    for (i = 0; i < problem->nof_vertices; i++) {
        if (ind1->color_of[i] != ind2->color_of[i]) {
            dist++;
        }
    }

    return dist;
}

static int calculate_diversity(void) {
    int i, j;
    double dist, diversity = 0;

    for (i = 0; i < hca_info->sizeof_population; i++) {
        dist = 0;
        for (j = 0; j < hca_info->sizeof_population; j++) {
            if (i != j) {
                dist += distance(population[i], population[j]);
            }
        }
        dist = (double) dist / (hca_info->sizeof_population - 1);
        diversity += dist;
    }
    diversity = (double) diversity / hca_info->sizeof_population;

    return diversity;
}

static int hca_terminate_conditions(gcp_solution_t *solution, int diversity) {

    if (diversity < HCA_DEFAULT_DIVERSITY) {
        solution->stop_criterion = STOP_ALG;
        return TRUE;
    } else if (best_solution->nof_confl_edges == 0) {
        solution->stop_criterion = STOP_BEST;
        return TRUE;
    }
    return FALSE;

}

/*###*/
int cross;

void *filler(void *threadId) {
    long tid = (long) threadId;
    int parent1, parent2;
    sem_wait(&semFiller);
    cross = 0;
    while (1) {
        if (indexBufferCrossover == BUFFER_SIZE) {
            int i;
            for (i = 0; i < BUFFER_SIZE; i++) {
                sem_post(&semEmptier);
            }
            sem_wait(&semFiller);
        } else {
            choose_parents(&parent1, &parent2);
            crossover(parent1, parent2, indexBufferCrossover);
            indexBufferCrossover++;
            cross++;
        }
    }
}

void *emptier(void *threadId) {
    long int id = (long int) threadId;
    int contador = 0;
    sem_wait(&semEmptier);
    while (1) {
        test_solution(bufferCrossover[id]);
        tabucol(bufferCrossover[id], hca_info->cyc_local_search, tabucol_info->tl_style);

        pthread_barrier_wait(&barrier);
        if (id == 0) {
            sem_post(&semUpdater);
        }
        sem_wait(&semEmptier);
    }
}

int findBestInBuffer() {
    int posBest = 0;
    int i;
    for (i = 1; i < BUFFER_SIZE; i++) {
        if (bufferCrossover[i]->nof_confl_vertices < bufferCrossover[posBest]->nof_confl_vertices) {
            posBest = i;
        }
    }
    return posBest;
}

int findWorstParent() {
    int posWorst = 0;
    int i;
    for (i = 1; i < hca_info->sizeof_population; i++) {
        if (population[i]->nof_confl_vertices > population[posWorst]->nof_confl_vertices) {
            posWorst = i;
        }
    }
    return posWorst;
}

/**
 * Verifica se o pior da populacao é melhor que o melhor do buffer, se for ele mantem o cara no buffer para realizar tabucol de novo
 * @return posicao do filho atualizado na populacao ou -1 caso o filho permaneceu no buffer
 */
int updatePopulation() {
    int posWorst = findWorstParent();
    int posBestBuffer = findBestInBuffer();
    if (population[posWorst]->nof_confl_vertices > bufferCrossover[posBestBuffer]->nof_confl_vertices) {
        cpy_solution(bufferCrossover[posBestBuffer], population[posWorst]);
        return posWorst;
    }
    return -1;
}

void *updater(void *threadId) {
    long int id = (long int) threadId;
    int cycle = 0;
    int converg = 0;
    int cont = 0;
    sem_wait(&semUpdater);
    while (1) {
        printf("#%ld: atualizando populacao - %d\n", id, cont++);

        sem_post(&semFiller);
        sem_wait(&semUpdater);

        int sp = updatePopulation();

        //verifica se atualizou na populacao ou manteve no buffer
        if (sp >= 0) {
            if (best_solution->nof_confl_edges > population[sp]->nof_confl_edges) {
                cpy_solution(population[sp], best_solution);
                best_solution->time_to_best = current_usertime_secs();
                best_solution->cycles_to_best = cycle;
                converg = 0;
            }
            indexBufferCrossover = 0;
        } else {
            printf("TA FODA SUBSTITUIR A POPULATION\n");
            if (best_solution->nof_confl_edges > bufferCrossover[0]->nof_confl_edges) {
                cpy_solution(bufferCrossover[0], best_solution);
                best_solution->time_to_best = current_usertime_secs();
                best_solution->cycles_to_best = cycle;
                converg = 0;
            }
            indexBufferCrossover = 1;
        }
        
        hca_info->diversity = calculate_diversity();
        printf("Diversidade: %d\n",hca_info->diversity);

        if (get_flag(problem->flags, FLAG_VERBOSE)) {
            fprintf(problem->fileout, "HCA: cycle %d; best so far: %d; diversity: %d; parent substituted: %d\n",
                    cycle, best_solution->nof_confl_edges, hca_info->diversity, sp + 1);
        }

        if (!(!hca_terminate_conditions(best_solution, hca_info->diversity) &&
                !terminate_conditions(best_solution, cycle, converg))) {
            best_solution->spent_time = current_usertime_secs();
            best_solution->total_cycles = cycle;
            hca_info->nof_cross = cross;

            printf("FIIIMMM\n\n");
            show_solution(best_solution);
            
            getchar();
            exit(EXIT_SUCCESS);
            //return best_solution;
        }
        cycle++;
        converg++;
    }
}

/*###*/

gcp_solution_t* hca(void) {

    int cycle = 0;
    int converg = 0;
    int parent1, parent2, sp;
    int cross = 0;

    hca_info->diversity = 2 * HCA_DEFAULT_DIVERSITY;

    best_solution = init_solution();
    best_solution->nof_confl_edges = INT_MAX;
    offspring = init_solution();
    /*###*/
    indexBufferCrossover = 0;
    /*###*/
    create_population();

    /*###*/
    pthread_t fillerThread;
    pthread_t emptierThreads[BUFFER_SIZE];
    pthread_t updaterThread;

    sem_init(&semFiller, 0, 1);
    sem_init(&semEmptier, 0, 0);
    sem_init(&semUpdater, 0, 0);
    pthread_barrier_init(&barrier, NULL, BUFFER_SIZE);

    long int i = 0;
    pthread_create(&fillerThread, NULL, filler, (void*) i);
    for (i = 0; i < BUFFER_SIZE; i++) {
        pthread_create(&emptierThreads[i], NULL, emptier, (void*) i);
    }
    i = 0;
    pthread_create(&updaterThread, NULL, updater, (void*) i);


    pthread_join(fillerThread, NULL);
    for (i = 0; i < BUFFER_SIZE; i++) {
        pthread_join(emptierThreads[i], NULL);
    }
    pthread_join(updaterThread, NULL);

    /*###*/


    while (!hca_terminate_conditions(best_solution, hca_info->diversity) &&
            !terminate_conditions(best_solution, cycle, converg)) {

        /*choose_parents(&parent1, &parent2);
        crossover(parent1, parent2);
        cross++;*/


        tabucol(offspring, hca_info->cyc_local_search, tabucol_info->tl_style);
        sp = substitute_worst(parent1, parent2, offspring);

        if (best_solution->nof_confl_edges > offspring->nof_confl_edges) {
            cpy_solution(offspring, best_solution);
            best_solution->time_to_best = current_usertime_secs();
            best_solution->cycles_to_best = cycle;
            converg = 0;
        }

        hca_info->diversity = calculate_diversity();

        if (get_flag(problem->flags, FLAG_VERBOSE)) {
            fprintf(problem->fileout, "HCA: cycle %d; best so far: %d; diversity: %d; parent substituted: %d\n",
                    cycle, best_solution->nof_confl_edges, hca_info->diversity, sp + 1);
        }

        cycle++;
        converg++;

    }

    best_solution->spent_time = current_usertime_secs();
    best_solution->total_cycles = cycle;
    hca_info->nof_cross = cross;

    return best_solution;

}


