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
#if !defined(COMMON_LIST_H)
#define COMMON_LIST_H

#include "com_define.h"

/*
 * Simple doubly linked list implementation.
 *
 * Some of the internal functions ("__xxx") are useful when
 * manipulating whole lists rather than single entries, as
 * sometimes we already know the next/prev entries and we can
 * generate better code by using them directly rather than
 * using the generic single-entry routines.
 */


/*
 *
 *
 * Data Structure
 *
 *
 */
typedef struct _List_Head {
	struct _List_Head *prev, *next;
} List_Head, * PList_Head;

typedef struct _Counted_List_Head {
	struct _List_Head *prev, *next;
	MV_U32 node_count;
} Counted_List_Head, * PCounted_List_Head;

/*
 *
 *
 * Exposed Functions
 *
 *
 */
 
#define MV_LIST_HEAD(name) \
	List_Head name = { &(name), &(name) }

#define MV_LIST_HEAD_INIT(ptr) do { \
	(ptr)->next = (ptr)->prev = (ptr); \
} while (0)

#define MV_COUNTED_LIST_HEAD_INIT(ptr) do { \
	(ptr)->next = (ptr)->prev = (List_Head *)(ptr); \
	(ptr)->node_count = 0; \
} while (0)

/**
 * LIST_ENTRY - get the struct for this entry
 * @ptr:	the &List_Head pointer.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_struct within the struct.
 */

#define CONTAINER_OF(ptr, type, member) 			\
        ( (type *)( (char *)(ptr) - OFFSET_OF(type,member) ) )

#define LIST_ENTRY(ptr, type, member) \
	CONTAINER_OF(ptr, type, member)


/**
 * LIST_FOR_EACH	-	iterate over a list
 * @pos:	the &List_Head to use as a loop counter.
 * @head:	the head for your list.
 */
#define LIST_FOR_EACH(pos, head) \
	for (pos = (head)->next; pos != (List_Head *)(head); pos = pos->next)

/**
 * LIST_FOR_EACH_PREV	-	iterate over a list backwards
 * @pos:	the &List_Head to use as a loop counter.
 * @head:	the head for your list.
 */
#define LIST_FOR_EACH_PREV(pos, head) \
	for (pos = (head)->prev; pos != (List_Head *)(head); pos = pos->prev)

/**
 * LIST_FOR_EACH_ENTRY	-	iterate over list of given type
 * @pos:	the type * to use as a loop counter.
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 */
#define LIST_FOR_EACH_ENTRY(pos, head, member)				\
	for (pos = LIST_ENTRY((head)->next, typeof(*pos), member);	\
	     &pos->member != (List_Head *)(head); 	\
	     pos = LIST_ENTRY(pos->member.next, typeof(*pos), member))

/**
 * list_for_each_entry_safe - iterate over list of given type safe against removal of list entry
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member: the name of the list_struct within the struct.
 */
#define LIST_FOR_EACH_ENTRY_SAFE(pos, n, head, member)			\
			for (pos = LIST_ENTRY((head)->next, typeof(*pos), member),	\
				n = LIST_ENTRY(pos->member.next, typeof(*pos), member); \
				 &pos->member != (List_Head *) (head);					\
				 pos = n, n = LIST_ENTRY(n->member.next, typeof(*n), member))
	


/**
 * LIST_FOR_EACH_ENTRY_TYPE	-	iterate over list of given type
 * @pos:	the type * to use as a loop counter.
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 * @type:	the type of the struct this is embedded in.
*/
#define LIST_FOR_EACH_ENTRY_TYPE(pos, head, type, member)       \
	for (pos = LIST_ENTRY((head)->next, type, member);	\
	     &pos->member != (List_Head *) (head); 	                        \
	     pos = LIST_ENTRY(pos->member.next, type, member))

/**
 * LIST_FOR_EACH_ENTRY_PREV - iterate backwards over list of given type.
 * @pos:	the type * to use as a loop counter.
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 */
#define LIST_FOR_EACH_ENTRY_PREV(pos, head, member)			\
	for (pos = LIST_ENTRY((head)->prev, typeof(*pos), member);	\
	     &pos->member != (List_Head *)(head); 	\
	     pos = LIST_ENTRY(pos->member.prev, typeof(*pos), member))


#include "com_list.c"

#define List_GetFirstEntry(head, type, member)	\
	LIST_ENTRY(List_GetFirst(head), type, member)

#define Counted_List_GetFirstEntry(head, type, member) \
	LIST_ENTRY(Counted_List_GetFirst(head), type, member)

#define List_GetLastEntry(head, type, member)	\
	LIST_ENTRY(List_GetLast(head), type, member)

#define Counted_List_GetLastEntry(head, type, member) \
	LIST_ENTRY(Counted_List_GetLast(head), type, member)

#endif /* COMMON_LIST_H */
