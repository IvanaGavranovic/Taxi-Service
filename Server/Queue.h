#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include "Defs.h"

typedef struct cvor_st
{
	CLIENT_MESSAGE vrednost;
	struct cvor_st* sledeci;
}ORDER_NODE;

void Init(ORDER_NODE** head, CRITICAL_SECTION cs)
{
	EnterCriticalSection(&cs);
	*head = NULL;
	LeaveCriticalSection(&cs);
}

void Enqueue(ORDER_NODE** head, CLIENT_MESSAGE vrednost, CRITICAL_SECTION cs)
{
	ORDER_NODE* novi = (ORDER_NODE*)malloc(sizeof(ORDER_NODE));
	if (novi == NULL)
	{
		printf("nema memorije...\n");
		return;
	}
	novi->vrednost = vrednost;
	novi->sledeci = NULL;

	novi->sledeci = *head;

	EnterCriticalSection(&cs);
	*head = novi;
	LeaveCriticalSection(&cs);
}

CLIENT_MESSAGE Dequeue(ORDER_NODE** head, CRITICAL_SECTION cs)
{
	ORDER_NODE* temp = *head;
	if (temp == NULL)
	{
		printf("Lista je vec prazna.\n");
		CLIENT_MESSAGE v;
		v.type = -1;
		return v;
	}
	if (temp->sledeci == NULL)
	{
		CLIENT_MESSAGE ret = temp->vrednost;

		EnterCriticalSection(&cs);
		free(temp);
		*head = NULL;
		LeaveCriticalSection(&cs);

		return ret;
	}
	ORDER_NODE* prethodni = temp;
	temp = temp->sledeci;
	while (temp->sledeci != NULL)
	{
		prethodni = temp;
		temp = temp->sledeci;
	}
	CLIENT_MESSAGE ret = temp->vrednost;
	EnterCriticalSection(&cs);
	free(temp);
	prethodni->sledeci = NULL;
	LeaveCriticalSection(&cs);

	return ret;
}