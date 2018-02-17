#include "monitor/watchpoint.h"
#include "monitor/expr.h"

#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head, *free_;

void init_wp_pool() {
	int i;
	for(i = 0; i < NR_WP; i ++) {
		wp_pool[i].NO = i;
		wp_pool[i].next = &wp_pool[i + 1];
	}
	wp_pool[NR_WP - 1].next = NULL;

	head = NULL;
	free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

int get_NR_WP() {
	return NR_WP;
}

WP *get_wp_ptr(int no) {
	int i;
	for(i = 0; i < NR_WP; i++) {
		if(wp_pool[i].NO == no)
			return &wp_pool[i];
	}
	return NULL;
}

WP *new_wp() {
	WP *new = free_;
	if(head == NULL)
		head = free_;
	if(free_->next == NULL)
		panic("No free watchpoint\n");
	free_ = free_->next;
	return new;
}

void free_wp(WP *wp) {
	WP *tmp = free_;

	while(tmp->next != NULL) {
		if(tmp == wp) {
			printf("This wp '%d' no need to been free\n", wp->NO);
			return ;
		}
		tmp = tmp->next;
	}
	tmp->next = wp;
	if(head != wp) {
		tmp = head;
		while(tmp->next != wp)
			tmp = tmp->next;
		tmp->next = wp->next;
	} else {	
		if(head->next == free_)
			head = NULL;
		else 
			head = head->next;
	}
 	wp->next = NULL;	
}
