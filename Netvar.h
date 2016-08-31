#pragma once

#include <unordered_map>
#include <memory>

#include "dt_common2.h"
#include "dt_recv2.h"

// this and the cpp are creds to "Altimor"

class RecvTable;

class netvar_tree
{
	struct node;
	using map_type = std::unordered_map<std::string, std::shared_ptr<node>>;

	struct node
	{
		node(int offset, RecvProp *p) : offset(offset), prop(p) {}
		map_type nodes;
		int offset;
		RecvProp *prop;
	};

	map_type nodes;

public:
	//netvar_tree ( );

	void init();

private:
	void populate_nodes(class RecvTable *recv_table, map_type *map);

	/**
	* get_offset_recursive - Return the offset of the final node
	* @map:	Node map to scan
	* @acc:	Offset accumulator
	* @name:	Netvar name to search for
	*
	* Get the offset of the last netvar from map and return the sum of it and accum
	*/
	int get_offset_recursive(map_type &map, int acc, const char *name)
	{
		return acc + map[name]->offset;
	}

	/**
	* get_offset_recursive - Recursively grab an offset from the tree
	* @map:	Node map to scan
	* @acc:	Offset accumulator
	* @name:	Netvar name to search for
	* @args:	Remaining netvar names
	*
	* Perform tail recursion with the nodes of the specified branch of the tree passed for map
	* and the offset of that branch added to acc
	*/
	template <typename... args_t>
	int get_offset_recursive(map_type &map, int acc, const char *name, args_t... args)
	{
		const auto &node = map[name];
		return get_offset_recursive(node->nodes, acc + node->offset, args...);
	}


	RecvProp *get_prop_recursive(map_type &map, const char *name)
	{
		return map[name]->prop;
	}

	template<typename... args_t>
	RecvProp *get_prop_recursive(map_type &map, const char *name, args_t... args)
	{
		const auto &node = map[name];
		return get_prop_recursive(node->nodes, args...);
	}

	int find_offset_recursive( map_type &map, std::string name )
	{
		for( auto &n : nodes )
		{	
			if( n.first == name )
			{
				if( n.second != nullptr )
				{
					return n.second->offset;
				}
			}
			else
				find_offset_recursive( n.second->nodes, name );
		}
		
		return -1;
	}

public:
	/**
	* get_offset - Get the offset of a netvar given a list of branch names
	* @name:	Top level datatable name
	* @args:	Remaining netvar names
	*
	* Initiate a recursive search down the branch corresponding to the specified datable name
	*/
	template <typename... args_t>
	int get_offset(const char *name, args_t... args)
	{
		const auto &node = nodes[name];
		return get_offset_recursive(node->nodes, node->offset, args...);
	}

	RecvProp *get_prop( const char *name )
	{
		const auto &node = nodes[ name ];
		return node->prop;
	}

	template<typename... args_t>
	RecvProp *get_prop(const char *name, args_t... args)
	{
		const auto &node = nodes[name];
		return get_prop_recursive(node->nodes, args...);
	}

	int find_offset( std::string name )
	{
		try
		{
			return find_offset_recursive( nodes, name );
		}
		catch( ... )
		{
			return -1;
		}

	}


	void dump();
};

extern netvar_tree gNetvars;
