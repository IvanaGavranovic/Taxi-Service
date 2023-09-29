#pragma once

enum ETopicType { STATUS, ANALOG };
enum EValType { SWG, CRB, MER };

typedef struct client_message_st
{
	char type;
	char adress[30];
	char clientName[30];
}CLIENT_MESSAGE;