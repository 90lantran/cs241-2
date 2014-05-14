/** @file part1.c */

/*
 * Machine Problem #0
 * CS 241 Fall2013
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "part2-functions.h"

/**
 * (Edit this function to print out the ten "Illinois" lines in part2-functions.c in order.)
 */
int main()
{
    first_step(81);

    int *a = malloc(sizeof(int));
    *a = 132;
    second_step(a);
    free(a);

    int **b = malloc(sizeof(int));
    b[0] = malloc(sizeof(int));
    *b[0] = 8942;
    double_step(b);
    free(b[0]);
    free(b);

    int *c = 0;
    strange_step(c);
    free(c);

    char *d = malloc(3);
    d[3] = 0;
    void *d2 = d;
    empty_step(d2);
    free(d);

    char *e = malloc(3);
    e[3] = 'u';
    void *f = e;
    two_step(f, e);
    free(e);

    char *g = malloc(1);
    char *h = g+2;
    char *i = h+2;
    three_step(g, h, i);
    free(g);

    char *j = malloc(3);
    char *k = malloc(3);
    char *l = malloc(3);
    j[1] = 'a';
    k[2] = j[1]+8;
    l[3] = k[2]+8;
    step_step_step(j, k, l);
    free(j);
    free(k);
    free(l);

    char *m = malloc(1);
    *m = 'a';
    it_may_be_odd(m, (int)'a');
    free(m);

    char *n = malloc(4);
    n[0] = 1;
    n[1] = 0;
    n[2] = 0;
    n[3] = 2;
    void *n2 = n;
    void *o2 = n;
    the_end(n2, o2);
    free(n);
	return 0;
}
