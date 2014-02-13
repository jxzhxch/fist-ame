#include "stdafx.h"
#include <vin64/kovfs.h>
#include <set>

namespace vin
{
	namespace vfs
	{
		node_t::node_t( const char * p ) 
		{
			_name = p ? p : "";
		};
		node_t::node_t( const std::string& n )
		{
			_name = n;
		}
		node_t * node_t::get_node( const char* name, bool force /* = false */ )
		{
			if( !name ) return NULL;
			return get_node( std::string(name), force );
		}
		node_t * node_t::get_node( const std::string& name, bool force /* = false */ )
		{
			node_t tmp( name );
			return get_node( std::string(name), force );
		}
		
	};


	KsObTree::KsObTree() : _root("\\")
	{}

	vfs::node_t * KsObTree::get_root()
	{
		return &_root;
	}

	long KsObTree::eares_path( const char * path )
	{
		vfs::node_t* node = get_node_by_path( path );
		if( !node ) return VIN_PATH_NOT_FOUND;

		if( node->_children.size() ) 
			return VIN_PATH_IS_BUSY;

		return node->parent()->erase_child( node->name_o() );
	}

	vfs::node_t * KsObTree::create_path( const char * path, bool force = true, KoMountable* ob = 0, vfs::node_t * from = 0 )
	{
		vfs::creator mkpath( from ? from : get_root(), force );
		for( ; *path; )
		{
			long lr = mkpath.parse( path );
			if( lr == VIN_NEED_REPARSE )
			{
				refp<vin::KoSymbolLink> lnk = (vin::KoSymbolLink*)
					(fdpath.current()->mounted()->asType( vin::KoSymbolLink::ObId ));

				vfs::node_t * node = get_node_by_path( lnk->target.c_str() );
				if( !node ) return NULL;

				fdpath.reset(node);
				
				vfs::skip_slash(path);
				
				continue;
			}

			if( lr < 0 ) return NULL;

			vfs::node_t * cur = mkpath.current();
			if( !cur ) return NULL;

			if( ob ) 
			{
				lr = cur->mount( ob );
				if( lr < 0 ) return NULL;
			}
			return cur;
		}
		return NULL;
	}

	vfs::node_t * KsObTree::get_node_by_path( const char * path, bool reparse = true, vfs::node_t * root = 0)
	{
		vfs::finder fdpath( root ? root:get_root(), reparse );
		for( ; *path; )
		{
			long lr = fdpath.parse( path );
			if( lr == VIN_NEED_REPARSE )
			{
				refp<vin::KoSymbolLink> lnk = (vin::KoSymbolLink*)
					(fdpath.current()->mounted()->asType( vin::KoSymbolLink::ObId ));

				vfs::node_t * node = get_node_by_path( lnk->target.c_str() );
				if( !node ) return NULL;

				fdpath.reset(node);
				vfs::skip_slash(path);

				continue;
			}
			else if( lr < 0 ) 
			{
				return NULL;
			}
		}
		return fdpath.current();
	}

	vfs::node_t * KsObTree::get_next( vfs::node_t * parent, vfs::node_t * current )
	{
		if( !parent ) return NULL;
		return parent->next( current );
	}

};