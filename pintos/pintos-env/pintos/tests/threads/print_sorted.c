/*
	Pintos assignment 1: lists - OS 2020
	Stefano Taillefert
*/
#include <stdio.h>
#include "../../lib/kernel/list.h"
#include "listpop.h"

void populate(struct list * l, int * a, int n);
bool compare_items(const struct list_elem * a, const struct list_elem * b, void * aux UNUSED);
void print_sorted(struct list * l);
void test_print_sorted(void);

struct item {
	struct list_elem elem;
	int priority;
};

/** 
 * Fills the list l with the elements of array a, which contains n integers.
 */
void populate(struct list * l, int * a, int n) {
	int i;
	for (i = 0; i < n; ++i) {
		struct item newitem;
		newitem.priority = a[i];
		list_push_back(l, &newitem.elem);
		printf("Populating, priority: %d\n", newitem.priority);
	}
}

/**
 * Helper function to sort the list.
 */
bool compare_items(const struct list_elem * a, const struct list_elem * b, void * aux UNUSED) {
	struct item * ia = list_entry(a, struct item, elem);
	struct item * ib = list_entry(b, struct item, elem);
	return (ia->priority < ib->priority);
}

/**
 * Sorts the elements of the list l and prints them in ascending order of priority.
 */
void print_sorted(struct list * l) {
	list_sort(l, compare_items, NULL);

	struct list_elem * pos;
	for (pos = list_begin(l); pos != list_end(l); pos = list_next(pos))	{
		struct item * it = list_entry(pos, struct item, elem);
		printf("Item priority %d\n", it->priority);
	}
}

/**
 * Test: creates, populates and prints the sorted list.
 */
void test_print_sorted(void) {
	struct list item_list;
	list_init(&item_list);

	populate(&item_list, ITEMARRAY, ITEMCOUNT);
	print_sorted(&item_list);
}