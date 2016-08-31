#include "Netvar.h"

#include "SDK.h"

#include <memory>

#undef GetProp

/**
* netvar_tree - Constructor
*
* Call populate_nodes on every RecvTable under client->GetAllClasses()
*/
void netvar_tree::init()
{
	const auto *client_class = gInts.Client->GetAllClasses();
	while(client_class != nullptr)
	{
		const auto class_info = std::make_shared<node>(0, nullptr);
		auto *recv_table = client_class->Table;
		populate_nodes(recv_table, &class_info->nodes);
		nodes.emplace(recv_table->GetName(), class_info);

		client_class = client_class->pNextClass;
	}
}

/**
* populate_nodes - Populate a node map with brances
* @recv_table:	Table the map corresponds to
* @map:	Map pointer
*
* Add info for every prop in the recv table to the node map. If a prop is a
* datatable itself, initiate a recursive call to create more branches.
*/
void netvar_tree::populate_nodes(RecvTable *recv_table, map_type *map)
{
	for(auto i = 0; i < recv_table->GetNumProps(); i++)
	{
		auto *prop = recv_table->GetProp(i);
		const auto prop_info = std::make_shared<node>(prop->GetOffset(), prop);

		if(prop->GetType() == DPT_DataTable)
			populate_nodes(prop->GetDataTable(), &prop_info->nodes);

		map->emplace(prop->GetName(), prop_info);
	}
}

netvar_tree gNetvars;