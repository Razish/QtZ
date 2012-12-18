#include "q_shared.h"
#include "linkedlist.h"

linkedList_t *LinkedList_PushObject( linkedList_t **root, void *data )
{
	if ( *root == NULL )
	{//No root object
		linkedList_t *newNode = (linkedList_t *)malloc( sizeof( linkedList_t ) );
		memset( newNode, 0, sizeof( linkedList_t ) );

		newNode->data = data;

		*root = newNode;
		return newNode;
	}
	else
	{//Add new object at end of list
		linkedList_t *currNode = *root;

		while ( currNode->next )
			currNode = currNode->next;

		currNode->next = (linkedList_t *)malloc( sizeof( linkedList_t ) );
		currNode = currNode->next;
		memset( currNode, 0, sizeof( linkedList_t ) );
		currNode->data = data;

		return currNode;
	}
}

void LinkedList_RemoveObject( linkedList_t **root, linkedList_t *node )
{
	linkedList_t *currNode = *root;

	if ( node == *root )
	{//hm o.o
		*root = (*root)->next;
		free( currNode );
		return;
	}

	while ( currNode != NULL )
	{
		if ( currNode->next == node )
		{
			currNode->next = node->next;
			free( node );
			return;
		}
		currNode = currNode->next;
	}
}

// for ( node=root; node; node=Traverse( node ) )
linkedList_t *LinkedList_Traverse( linkedList_t *node )
{
	return node ? node->next : NULL;
}
