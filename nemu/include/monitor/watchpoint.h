#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"

typedef struct watchpoint {
	int NO;
	struct watchpoint *next;

	/* TODO: Add more members if necessary */
	uint32_t val;		//last value of the point
	char what[32];		//where the bp/wp is in the source of code
	//int type;			//breakpoint, watchpoint, catchpoint
	//uint32_t addr;		//hardware address
	//int disp;			//disposition : whether the bp/wp is marked to be disabled or keeped or deleted when hit
	bool enb;			//enable or disabled

} WP;

void init_wp_pool();

int get_NR_WP();

WP *get_wp_ptr(int no); 

void print_wp();

WP *new_wp();

void free_wp(WP *wp);

int check_wp_pool(uint32_t *new_val);

#endif
