// ============================================================================
//
//     Copyright (c) Marvell Corporation 2000-2015  -  All rights reserved
//
//  This computer program contains confidential and proprietary information,
//  and  may NOT  be reproduced or transmitted, in whole or in part,  in any
//  form,  or by any means electronic, mechanical, photo-optical, or  other-
//  wise, and  may NOT  be translated  into  another  language  without  the
//  express written permission from Marvell Corporation.
//
// ============================================================================
// =      C O M P A N Y      P R O P R I E T A R Y      M A T E R I A L       =
// ============================================================================
#include "com_define.h"
#include "com_list.h"
#include "com_dbg.h"
/*
 * Insert a new entry between two known consecutive entries.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static MV_INLINE void __List_Add(List_Head *new_one,
			      List_Head *prev,
			      List_Head *next)
{
	if (unlikely(next->prev != prev)) {
			MV_DPRINT(( "list_add corruption. next->prev should be "
				"prev (0x%p), but was 0x%p. (next=0x%p).\n",
				prev, next->prev, next));

			MV_DPRINT(( "prev=0x%p, prev->next=0x%p,  prev->prev=0x%p"
				"next=0x%p, next->next=0x%p, next->prev=0x%p.\n",
				prev, prev->next, prev->prev, next, next->next, next->prev));

		}
		if (unlikely(prev->next != next)) {
			MV_DPRINT(( "list_add corruption. prev->next should be "
				"next (0x%p), but was 0x%p. (prev=0x%p).\n",
				next, prev->next, prev));

			MV_DPRINT(( "prev=0x%p, prev->next=0x%p,  prev->prev=0x%p,"
				"next=0x%p, next->next=0x%p, next->prev=0x%p.\n",
				prev, prev->next, prev->prev, next, next->next, next->prev));

		}

	next->prev = new_one;
	new_one->next = next;
	new_one->prev = prev;
	prev->next = new_one;
}

/**
 * List_Add - add a new entry
 * @new_one: new entry to be added
 * @head: list head to add it after
 *
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */
static MV_INLINE void List_Add(List_Head *new_one, List_Head *head)
{
	__List_Add(new_one, head, head->next);
}

static MV_INLINE void Counted_List_Add(List_Head *new_one, Counted_List_Head *head)
{
	List_Add(new_one, (List_Head *)head);
	head->node_count++;
}

/**
 * List_AddTail - add a new entry
 * @new_one: new entry to be added
 * @head: list head to add it before
 *
 * Insert a new entry before the specified head.
 * This is useful for implementing queues.
 */
static MV_INLINE void List_AddTail(List_Head *new_one, List_Head *head)
{
	__List_Add(new_one, head->prev, head);
}

static MV_INLINE void Counted_List_AddTail(List_Head *new_one, Counted_List_Head *head)
{
	List_AddTail(new_one, (List_Head *) head);
	head->node_count++;
}

/*
 * Delete a list entry by making the prev/next entries
 * point to each other.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static MV_INLINE void __List_Del(List_Head * prev, List_Head * next)
{
	next->prev = prev;
	prev->next = next;
}

/**
 * List_Del - deletes entry from list.
 * @entry: the element to delete from the list.
 * Note: List_Empty on entry does not return true after this, the entry is
 * in an undefined state.
 */
static void List_Del(List_Head *entry)
{
	if(entry->prev && entry->prev){
		if (unlikely(entry->prev->next != entry)) {
                        MV_DASSERT(MV_FALSE);
			MV_DPRINT(("list_del corruption. prev->next should be %p, "
					"but was %p\n", entry, entry->prev->next));
			MV_DPRINT(( "entry->next=%p, entry->next->next=%p,	entry->next->prev=%p.\n"
					"entry->prev=%p, entry->prev->next=%p, entry->prev->prev=%p.\n",
				entry->prev, entry->next->next, entry->next->prev, entry->prev, entry->prev->next, entry->prev->prev));

		}
		if (unlikely(entry->next->prev != entry)) {
                        MV_DASSERT(MV_FALSE);
			MV_DPRINT(( "list_del corruption. next->prev should be %p, "
					"but was %p\n", entry, entry->next->prev));

			MV_DPRINT(("entry->next=%p, entry->next->next=%p,  entry->next->prev=%p.\n"
					"entry->prev=%p, entry->prev->next=%p, entry->prev->prev=%p.\n",
				entry->next, entry->next->next, entry->next->prev, entry->prev, entry->prev->next, entry->prev->prev));

		}
	}

	__List_Del(entry->prev, entry->next);
	entry->next = NULL;
	entry->prev = NULL;
}

static MV_INLINE void Counted_List_Del(List_Head *entry, Counted_List_Head *head)
{
	List_Del(entry);
	head->node_count--;
}


/**
 * List_DelInit - deletes entry from list and reinitialize it.
 * @entry: the element to delete from the list.
 */
static MV_INLINE void List_DelInit(List_Head *entry)
{
	__List_Del(entry->prev, entry->next);
	MV_LIST_HEAD_INIT(entry);
}

/**
 * List_Move - delete from one list and add as another's head
 * @list: the entry to move
 * @head: the head that will precede our entry
 */
static MV_INLINE void List_Move(List_Head *list, List_Head *head)
{
        __List_Del(list->prev, list->next);
        List_Add(list, head);
}

/**
 * List_MoveTail - delete from one list and add as another's tail
 * @list: the entry to move
 * @head: the head that will follow our entry
 */
static MV_INLINE void List_MoveTail(List_Head *list,
				  List_Head *head)
{
        __List_Del(list->prev, list->next);
        List_AddTail(list, head);
}

/**
 * List_Empty - tests whether a list is empty
 * @head: the list to test.
 */
static MV_INLINE int List_Empty(const List_Head *head)
{
	return head->next == head;
}

static MV_INLINE int Counted_List_Empty(const Counted_List_Head *head)
{
	return (head->node_count == 0);
}

static MV_INLINE int List_GetCount(const List_Head *head)
{
	int i=0;
	List_Head *pos;
	LIST_FOR_EACH(pos, head) {
		i++;
	}
	return i;
}

static MV_INLINE int Counted_List_GetCount(const Counted_List_Head *head, const MV_BOOLEAN traverse_list)
{
	int i=0;
	List_Head *pos;

	if (traverse_list) {
		LIST_FOR_EACH(pos, head) {
			i++;
		}
		return i;
	}
	else {
		return head->node_count;
	}
}

static MV_INLINE List_Head* List_GetFirst(List_Head *head)
{
	List_Head * one = NULL;
	if ( List_Empty(head) ) return NULL;

	one = head->next;
	List_Del(one);
	return one;
}

static MV_INLINE List_Head* Counted_List_GetFirst(Counted_List_Head *head)
{
	List_Head *one = NULL;
	if ( Counted_List_Empty(head) ) return NULL;
	one = head->next;
	Counted_List_Del(one, head);
	return one;
}

static MV_INLINE List_Head* List_GetLast(List_Head *head)
{
	List_Head * one = NULL;
	if ( List_Empty(head) ) return NULL;

	one = head->prev;
	List_Del(one);
	return one;
}

static MV_INLINE List_Head* Counted_List_GetLast(Counted_List_Head *head)
{
	List_Head * one = NULL;
	if ( Counted_List_Empty(head) ) return NULL;

	one = head->prev;
	Counted_List_Del(one, head);
	return one;
}

static MV_INLINE void __List_Splice(List_Head *list,
	List_Head *head)
{
	List_Head *first = list->next;
	List_Head *last = list->prev;
	List_Head *at = head->next;

	first->prev = head;
	head->next = first;

	last->next = at;
	at->prev = last;
}

static MV_INLINE void __List_SpliceTail(List_Head *list,
	List_Head *head)
{
	List_Head *first = list->next;
	List_Head *last = list->prev;
	List_Head *at = head->prev;

	first->prev = at;
	at->next = first;

	last->next = head;
	head->prev = last;
}

/**
 * List_Splice - join two lists
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 */
static MV_INLINE void List_Splice(List_Head *list, List_Head *head)
{
	if (!List_Empty(list))
		__List_Splice(list, head);
}

/**
 * List_AddList - join two lists and reinitialise the emptied list.
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 *
 * The list at @list is reinitialised
 */

static MV_INLINE void List_AddList(List_Head *list,
				 List_Head *head)
{
	if (!List_Empty(list)) {
		__List_Splice(list, head);
		MV_LIST_HEAD_INIT( list );
	}
}

static MV_INLINE void List_AddCountedList(Counted_List_Head *list, List_Head *head)
{
	if (!Counted_List_Empty(list)) {
		__List_Splice((List_Head *)list, head);
		MV_COUNTED_LIST_HEAD_INIT( list );
	}

}

static MV_INLINE void Counted_List_AddList(List_Head *list, Counted_List_Head *head)
{
	if (!List_Empty(list)) {
		head->node_count += List_GetCount(list);
		__List_Splice(list, (List_Head *) head);
		MV_LIST_HEAD_INIT( list );
	}
}

static MV_INLINE void Counted_List_AddCountedList(Counted_List_Head *list, Counted_List_Head *head)
{
	if (!Counted_List_Empty(list)) {
		head->node_count += list->node_count;
		__List_Splice((List_Head *)list, (List_Head *) head);
		MV_COUNTED_LIST_HEAD_INIT( list );
	}
}

static MV_INLINE void List_AddListTail(List_Head *list, List_Head *head)
{
	if (!List_Empty(list)) {
		__List_SpliceTail(list, head);
		MV_LIST_HEAD_INIT( list );
	}
}

static MV_INLINE void List_AddCountedList_Tail(Counted_List_Head *list, List_Head *head)
{
	if (!Counted_List_Empty(list)) {
		__List_SpliceTail((List_Head *) list, head);
		MV_COUNTED_LIST_HEAD_INIT(list);
	}
}

static MV_INLINE void Counted_List_AddList_Tail(List_Head *list, Counted_List_Head *head)
{
	if (!List_Empty(list)) {
		head->node_count += List_GetCount(list);
		__List_SpliceTail(list, (List_Head *) head);
		MV_LIST_HEAD_INIT(list);
	}
}

static MV_INLINE void Counted_List_AddCountedList_Tail(Counted_List_Head *list, Counted_List_Head *head)
{
	if (!Counted_List_Empty(list)) {
		head->node_count += list->node_count;
		__List_SpliceTail((List_Head *) list, (List_Head *) head);
		MV_COUNTED_LIST_HEAD_INIT(list);
	}
}

