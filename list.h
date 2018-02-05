/*
 * This file is borrowed from linux kernel. It is quite useful to list, stack
 * and hash list manupalation.
 *
 * struct list_head
 *
 * INIT_LIST_HEAD(list)
 * __list_add(new, prev, next)
 * list_add(new, head)
 * list_add_tail(new, head)
 * list_del(entry)
 * list_replace(old, new)
 * list_replace_init(old, new)
 * list_del_init(entry)
 * list_move(list, head)
 * list_move_tail(list, head)
 * list_is_last(list, head)
 * list_empty(head)
 * list_empty_careful(head)
 * list_is_singular(head)
 * list_cut_position(list, head, entry)
 * list_splice(list, head)
 * list_splice_tail(list, head)
 * list_splice_init(list, head)
 * list_splice_tail_init(list, head)
 *
 * list_entry(ptr, type, member)
 * list_first_entry(ptr, type, member)
 * list_for_each(pos, head)
 * list_for_each_prev(pos, head)
 * list_for_each_safe(pos, n, head)
 * list_for_each_prev_safe(pos, n, head)
 *
 * struct hlist_head
 * struct hlist_node
 *
 * INIT_HLIST_HEAD(ptr)
 * INIT_HLIST_NODE(h)
 * hlist_unhashed(h)
 * hlist_empty(h)
 * hlist_del(h)
 * hlist_del_init(h)
 * hlist_add_head(n, h)
 * hlist_add_before(n, next)
 * hlist_add_after(n, next)
 * hlist_move_list(old, new)
 * 
 * hlist_entry(ptr, type, member)
 * hlist_for_each(pos, head)
 * hlist_for_each_safe(pos, n, head)
 */

#ifndef LIST_H
#define LIST_H

#include	<stddef.h>
#include    "arch.h"

// This definition is for compatibility
#define LIST_POISON1	NULL
#define LIST_POISON2	NULL
//#define prefetch(p)		(p)
//#define container_of(ptr, type, member)	((type *)( (char *)(ptr) - offsetof(type, member) ))

/*
 * Simple doubly linked list implementation.
 *
 * Some of the internal functions ("__xxx") are useful when
 * manipulating whole lists rather than single entries, as
 * sometimes we already know the next/prev entries and we can
 * generate better code by using them directly rather than
 * using the generic single-entry routines.
 */

typedef struct list_head {
	struct list_head *next, *prev;
} list_head_t;

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
	list_head_t name = LIST_HEAD_INIT(name)

static __inline void INIT_LIST_HEAD(list_head_t *list)
{
	list->next = list;
	list->prev = list;
}

/*
 * Insert a new entry between two known consecutive entries.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static __inline void __list_add(list_head_t *new,
			      list_head_t *prev,
			      list_head_t *next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

/**
 * list_add - add a new entry
 * @new: new entry to be added
 * @head: list head to add it after
 *
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */
static __inline void list_add(list_head_t *new, list_head_t *head)
{
	__list_add(new, head, head->next);
}


/**
 * list_add_tail - add a new entry
 * @new: new entry to be added
 * @head: list head to add it before
 *
 * Insert a new entry before the specified head.
 * This is useful for implementing queues.
 */
static __inline void list_add_tail(list_head_t *new, list_head_t *head)
{
	__list_add(new, head->prev, head);
}

/*
 * Delete a list entry by making the prev/next entries
 * point to each other.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static __inline void __list_del(list_head_t * prev, list_head_t * next)
{
	next->prev = prev;
	prev->next = next;
}

/**
 * list_del - deletes entry from list.
 * @entry: the element to delete from the list.
 * Note: list_empty() on entry does not return true after this, the entry is
 * in an undefined state.
 */
static __inline void list_del(list_head_t *entry)
{
	__list_del(entry->prev, entry->next);
}

/**
 * list_replace - replace old entry by new one
 * @old : the element to be replaced
 * @new : the new element to insert
 *
 * If @old was empty, it will be overwritten.
 */
static __inline void list_replace(list_head_t *old,
				list_head_t *new)
{
	new->next = old->next;
	new->next->prev = new;
	new->prev = old->prev;
	new->prev->next = new;
}

static __inline void list_replace_init(list_head_t *old,
					list_head_t *new)
{
	list_replace(old, new);
	INIT_LIST_HEAD(old);
}

/**
 * list_del_init - deletes entry from list and reinitialize it.
 * @entry: the element to delete from the list.
 */
static __inline void list_del_init(list_head_t *entry)
{
	__list_del(entry->prev, entry->next);
	INIT_LIST_HEAD(entry);
}

/**
 * list_move - delete from one list and add as another's head
 * @list: the entry to move
 * @head: the head that will precede our entry
 */
static __inline void list_move(list_head_t *list, list_head_t *head)
{
	__list_del(list->prev, list->next);
	list_add(list, head);
}


/**
 * list_move_tail - delete from one list and add as another's tail
 * @list: the entry to move
 * @head: the head that will follow our entry
 */
static __inline void list_move_tail(list_head_t *list,
				  list_head_t *head)
{
	__list_del(list->prev, list->next);
	list_add_tail(list, head);
}

/**
 * list_is_last - tests whether @list is the last entry in list @head
 * @list: the entry to test
 * @head: the head of the list
 */
static __inline int list_is_last(const list_head_t *list,
				const list_head_t *head)
{
	return list->next == head;
}

/**
 * list_empty - tests whether a list is empty
 * @head: the list to test.
 */
static __inline int list_empty(const list_head_t *head)
{
	return head->next == head;
}

/**
 * list_empty_careful - tests whether a list is empty and not being modified
 * @head: the list to test
 *
 * Description:
 * tests whether a list is empty _and_ checks that no other CPU might be
 * in the process of modifying either member (next or prev)
 *
 * NOTE: using list_empty_careful() without synchronization
 * can only be safe if the only activity that can happen
 * to the list entry is list_del_init(). Eg. it cannot be used
 * if another CPU could re-list_add() it.
 */
static __inline int list_empty_careful(const list_head_t *head)
{
	list_head_t *next = head->next;
	return (next == head) && (next == head->prev);
}

/**
 * list_is_singular - tests whether a list has just one entry.
 * @head: the list to test.
 */
static __inline int list_is_singular(const list_head_t *head)
{
	return !list_empty(head) && (head->next == head->prev);
}

static __inline void __list_cut_position(list_head_t *list,
		list_head_t *head, list_head_t *entry)
{
	list_head_t *new_first = entry->next;
	list->next = head->next;
	list->next->prev = list;
	list->prev = entry;
	entry->next = list;
	head->next = new_first;
	new_first->prev = head;
}

/**
 * list_cut_position - cut a list into two
 * @list: a new list to add all removed entries
 * @head: a list with entries
 * @entry: an entry within head, could be the head itself
 *	and if so we won't cut the list
 *
 * This helper moves the initial part of @head, up to and
 * including @entry, from @head to @list. You should
 * pass on @entry an element you know is on @head. @list
 * should be an empty list or a list you do not care about
 * losing its data.
 *
 */
static __inline void list_cut_position(list_head_t *list,
		list_head_t *head, list_head_t *entry)
{
	if (list_empty(head))
		return;
	if (list_is_singular(head) &&
		(head->next != entry && head != entry))
		return;
	if (entry == head)
		INIT_LIST_HEAD(list);
	else
		__list_cut_position(list, head, entry);
}

static __inline void __list_splice(const list_head_t *list,
				 list_head_t *prev,
				 list_head_t *next)
{
	list_head_t *first = list->next;
	list_head_t *last = list->prev;

	first->prev = prev;
	prev->next = first;

	last->next = next;
	next->prev = last;
}

/**
 * list_splice - join two lists, this is designed for stacks
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 */
static __inline void list_splice(const list_head_t *list,
				list_head_t *head)
{
	if (!list_empty(list))
		__list_splice(list, head, head->next);
}

/**
 * list_splice_tail - join two lists, each list being a queue
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 */
static __inline void list_splice_tail(list_head_t *list,
				list_head_t *head)
{
	if (!list_empty(list))
		__list_splice(list, head->prev, head);
}

/**
 * list_splice_init - join two lists and reinitialise the emptied list.
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 *
 * The list at @list is reinitialised
 */
static __inline void list_splice_init(list_head_t *list,
				    list_head_t *head)
{
	if (!list_empty(list)) {
		__list_splice(list, head, head->next);
		INIT_LIST_HEAD(list);
	}
}

/**
 * list_splice_tail_init - join two lists and reinitialise the emptied list
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 *
 * Each of the lists is a queue.
 * The list at @list is reinitialised
 */
static __inline void list_splice_tail_init(list_head_t *list,
					 list_head_t *head)
{
	if (!list_empty(list)) {
		__list_splice(list, head->prev, head);
		INIT_LIST_HEAD(list);
	}
}

/**
 * list_entry - get the struct for this entry
 * @ptr:	the &list_head_t pointer.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_struct within the struct.
 */
#define list_entry(ptr, type, member) \
	container_of(ptr, type, member)

/**
 * list_first_entry - get the first element from a list
 * @ptr:	the list head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_struct within the struct.
 *
 * Note, that list is expected to be not empty.
 */
#define list_first_entry(ptr, type, member) \
	list_entry((ptr)->next, type, member)

/**
 * list_last_entry - get the last element from a list
 * @ptr:       the list head to take the element from.
 * @type:      the type of the struct this is embedded in.
 * @member:    the name of the list_struct within the struct.
 *
 * Note, that list is expected to be not empty.
 */
#define list_last_entry(ptr, type, member) \
       list_entry((ptr)->prev, type, member)

/**
 * list_for_each	-	iterate over a list
 * @pos:	the &list_head_t to use as a loop cursor.
 * @head:	the head for your list.
 */
#define list_for_each(pos, head) \
	for (pos = (head)->next; prefetch(pos->next), pos != (head); \
        	pos = pos->next)

/**
 * __list_for_each	-	iterate over a list
 * @pos:	the &list_head_t to use as a loop cursor.
 * @head:	the head for your list.
 *
 * This variant differs from list_for_each() in that it's the
 * simplest possible list iteration code, no prefetching is done.
 * Use this for code that knows the list to be very short (empty
 * or 1 entry) most of the time.
 */
#define __list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

/**
 * list_for_each_prev	-	iterate over a list backwards
 * @pos:	the &list_head_t to use as a loop cursor.
 * @head:	the head for your list.
 */
#define list_for_each_prev(pos, head) \
	for (pos = (head)->prev; prefetch(pos->prev), pos != (head); \
        	pos = pos->prev)

/**
 * list_for_each_safe - iterate over a list safe against removal of list entry
 * @pos:	the &list_head_t to use as a loop cursor.
 * @n:		another &list_head_t to use as temporary storage
 * @head:	the head for your list.
 */
#define list_for_each_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); \
		pos = n, n = pos->next)

/**
 * list_for_each_prev_safe - iterate over a list backwards safe against removal of list entry
 * @pos:	the &list_head_t to use as a loop cursor.
 * @n:		another &list_head_t to use as temporary storage
 * @head:	the head for your list.
 */
#define list_for_each_prev_safe(pos, n, head) \
	for (pos = (head)->prev, n = pos->prev; \
	     prefetch(pos->prev), pos != (head); \
	     pos = n, n = pos->prev)

/*
 * Double linked lists with a single pointer list head.
 * Mostly useful for hash tables where the two pointer list head is
 * too wasteful.
 * You lose the ability to access the tail in O(1).
 */

typedef struct hlist_head {
	struct hlist_node *first;
} hlist_head_t;

typedef struct hlist_node {
	struct hlist_node *next, **pprev;
} hlist_node_t;

#define HLIST_HEAD_INIT { .first = NULL }
#define HLIST_HEAD(name) hlist_head_t name = {  .first = NULL }
#define INIT_HLIST_HEAD(ptr) ((ptr)->first = NULL)
static __inline void INIT_HLIST_NODE(hlist_node_t *h)
{
	h->next = NULL;
	h->pprev = NULL;
}

static __inline int hlist_unhashed(const hlist_node_t *h)
{
	return !h->pprev;
}

static __inline int hlist_empty(const hlist_head_t *h)
{
	return !h->first;
}

static __inline void __hlist_del(hlist_node_t *n)
{
	hlist_node_t *next = n->next;
	hlist_node_t **pprev = n->pprev;
	*pprev = next;
	if (next)
		next->pprev = pprev;
}

static __inline void hlist_del(hlist_node_t *n)
{
	__hlist_del(n);
	n->next = LIST_POISON1;
	n->pprev = LIST_POISON2;
}

static __inline void hlist_del_init(hlist_node_t *n)
{
	if (!hlist_unhashed(n)) {
		__hlist_del(n);
		INIT_HLIST_NODE(n);
	}
}

static __inline void hlist_add_head(hlist_node_t *n, hlist_head_t *h)
{
	hlist_node_t *first = h->first;
	n->next = first;
	if (first)
		first->pprev = &n->next;
	h->first = n;
	n->pprev = &h->first;
}

/* next must be != NULL */
static __inline void hlist_add_before(hlist_node_t *n,
					hlist_node_t *next)
{
	n->pprev = next->pprev;
	n->next = next;
	next->pprev = &n->next;
	*(n->pprev) = n;
}

static __inline void hlist_add_after(hlist_node_t *n,
					hlist_node_t *next)
{
	next->next = n->next;
	n->next = next;
	next->pprev = &n->next;

	if(next->next)
		next->next->pprev  = &next->next;
}

/*
 * Move a list from one list head to another. Fixup the pprev
 * reference of the first entry if it exists.
 */
static __inline void hlist_move_list(hlist_head_t *old,
				   hlist_head_t *new)
{
	new->first = old->first;
	if (new->first)
		new->first->pprev = &new->first;
	old->first = NULL;
}

#define hlist_entry(ptr, type, member) container_of(ptr,type,member)

//add by jeff
#define hlist_first_entry(hh, type, member) \
	 hlist_entry(hh->first, type, member);	

#define hlist_for_each(pos, head) \
	for (pos = (head)->first; pos; \
	     pos = pos->next)

#define hlist_for_each_safe(pos, n, head) \
    for (pos = (head)->first; \
         pos && ({ n = pos->next; 1; }); \
         pos = n)

#endif

