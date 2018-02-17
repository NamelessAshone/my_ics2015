#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"

typedef struct watchpoint {
	int NO;
	struct watchpoint *next;

	/* TODO: Add more members if necessary */
	uint32_t val;
	char expr[32];

} WP;

int get_NR_WP();

WP *get_wp_ptr(int no); 

WP *new_wp();

void free_wp(WP *wp);

#endif
