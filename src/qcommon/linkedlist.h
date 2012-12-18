#pragma once

typedef struct linkedList_s
{
	void *data;
	struct linkedList_s *next;
} linkedList_t;

linkedList_t *LinkedList_PushObject( linkedList_t **root, void *data );
void LinkedList_RemoveObject( linkedList_t **root, linkedList_t *node );
linkedList_t *LinkedList_Traverse( linkedList_t *node );
