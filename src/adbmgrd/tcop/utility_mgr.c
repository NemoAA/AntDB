#include "postgres.h"

#include "mgr/mgr_cmds.h"
#include "nodes/nodes.h"
#include "nodes/pg_list.h"
#include "parser/mgr_node.h"
#include "tcop/utility.h"


const char *mgr_CreateCommandTag(Node *parsetree)
{
	const char *tag;
	AssertArg(parsetree);

	switch(nodeTag(parsetree))
	{
	case T_MGRAddHost:
		tag = "ADD HOST";
		break;
	case T_MGRDropHost:
		tag = "DROP HOST";
		break;
	case T_MGRAlterHost:
		tag = "ALTER HOST";
		break;
	case T_MGRAddGtm:
		tag = "ADD GTM";
		break;
	case T_MGRAlterGtm:
		tag = "ALTER GTM";
		break;
	case T_MGRDropGtm:
		tag = "DROP GTM";
		break;
	case T_MGRAlterParm:
		tag = "ALTER PARM";
		break;
	case T_MGRAddNode:
		tag = "ADD NODE";
		break;
	case T_MGRAlterNode:
		tag = "ALTER NODE";
		break;
	case T_MGRDropNode:
		tag = "DROP NODE";
		break;
	case T_MGRDeplory:
		tag = "DEPLORY";
		break;
	default:
		ereport(WARNING, (errmsg("unrecognized node type: %d", (int)nodeTag(parsetree))));
		tag = "???";
		break;
	}
	return tag;
}

void mgr_ProcessUtility(Node *parsetree, const char *queryString,
									ProcessUtilityContext context, ParamListInfo params,
									DestReceiver *dest,
									char *completionTag)
{
	AssertArg(parsetree);
	switch(nodeTag(parsetree))
	{
	case T_MGRAddHost:
		mgr_add_host((MGRAddHost*)parsetree, params, dest);
		break;
	case T_MGRDropHost:
		mgr_drop_host((MGRDropHost*)parsetree, params, dest);
		break;
	case T_MGRAlterHost:
		mgr_alter_host((MGRAlterHost*)parsetree, params, dest);
		break;
	case T_MGRAddGtm:
		mgr_add_gtm((MGRAddGtm*)parsetree, params, dest);
		break;
	case T_MGRAlterGtm:
		mgr_alter_gtm((MGRAlterGtm*)parsetree, params, dest);
		break;
	case T_MGRDropGtm:
		mgr_drop_gtm((MGRDropGtm*)parsetree, params, dest);
		break;
	case T_MGRAlterParm:
		mgr_alter_parm((MGRAlterParm*)parsetree, params, dest);
		break;
	case T_MGRAddNode:
		mgr_add_node((MGRAddNode*)parsetree, params, dest);
		break;
	case T_MGRAlterNode:
		mgr_alter_node((MGRAlterNode*)parsetree, params, dest);
		break;
	case T_MGRDropNode:
		mgr_drop_node((MGRDropNode*)parsetree, params, dest);
		break;
	case T_MGRDeplory:
		mgr_deplory((MGRDeplory*)parsetree, params, dest);
		break;
	default:
		ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR)
			,errmsg("unrecognized node type: %d", (int)nodeTag(parsetree))));
		break;
	}
}