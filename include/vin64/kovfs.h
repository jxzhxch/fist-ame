#ifndef __VIN_KO_VFS__
#define __VIN_KO_VFS__

#include <string>
#include <set>

#include "kobase.h"

namespace vin
{
	struct KoMountable;

	namespace vfs
	{
		static bool operator < ( const std::string & l, const std::string& r )
		{
			return stricmp( l.c_str(), r.c_str() ) < 0;
		}

		struct node_t
		{
			typedef std::set<node_t> children_t;
			const std::string	_name;
			children_t			_children;
			node_t*				_parent;
			KoMountable*		_mounted;
			bool operator < ( const node_t & right ) const
			{
				return _name < right._name;
			}
			node_t() : _mounted(0), _parent(0){};
			node_t( const char * p) : _name(p), _mounted(0), _parent(0){};
			node_t( const std::string& n ) : _name(n), _mounted(0), _parent(0){};
			~node_t() { unmount(); };
			KoMountable* mounted() { return _mounted; };
			bool	mount( KoMountable* op );
			bool	unmount();
			node_t*	get_node( const std::string& name, bool force = false );
			node_t*	get_node( const char* name, bool force = false );
			node_t*	add_child( const std::string& name );
			node_t*	add_child( const char* name );
			long	erase_child( const char* name, KoMountable** pp );
			long	erase_child( const std::string& name, KoMountable** pp );
			node_t*	next_child( node_t * p );
			const char * name_c()
			{
				return _name.c_str();
			}
			const std::string& name_o()
			{
				return _name;
			}
			node_t* parent()
			{
				return _parent;
			}
			bool obtain_full_path( std::string & path ) 
			{
				node_t * p = this;
				for( ; p ; p = p->parent() )
				{
					path.insert( 0, p->name_c() );
					path.insert( path.begin(), '\\' );
				}
				return true;
			}
		};

		struct KoMountable : KoBase
		{
			vfs::node_t *_node;
			KoMountable() : _node(0) 
			{
			};
			virtual ~KoMountable() 
			{
				detech();
			};
			virtual void attach( vfs::node_t* node )
			{
				if( _node ) _node->_mounted = 0;
				node->_mounted = this;
				_node = node;
			};
			virtual void detech()
			{
				if( !_node ) return ;
				_node->_mounted = 0;
				_node = 0;
			};
		};

		template < KoClass cls >
		struct TKoMountable : KoMountable
		{
			enum { ObId = cls };
			virtual KoClass main_class() { return ObId; };
			virtual void* as_class( KoClass clsid ) { return ( clsid == ObId ) ? this : 0; }
		};


		struct pather
		{
			static size_t skip_slash( const char * & p )
			{
				const char * k = p;
				for( ; *p && *p == '\\'; ++ p );
				return  p - k;
			}
			static const char * skip_not_slash( const char *& p )
			{
				for( ; *p && *p != '\\'; ++ p );
				return p;
			}
			long parse( const char * & p )
			{
				skip_slash( p );
				const char * nb = p;
				while( *p )
				{
					const char * ne = skip_not_slash(p);

					std::string tmp( nb, ne );
					long lr = onPathPart( tmp );
					if( lr < 0 ) return lr;

					skip_slash(p);
					nb = p;
				}
				return VFS_S_OK;
			}
			virtual long onPathPart( const std::string& dir ) { return VFS_E_FAIL; };
		};
		
		
		struct creator : pather
		{
		protected:
			node_t *	_current;
			bool		_force;
		public:
			creator( node_t * current, bool force = true ) 
				: _current(current)
				, _force(force)
			{
			}
			node_t * current()
			{
				return _current;
			}
			void reset( node_t * p )
			{
				_current = p;
			}
			virtual long onPathPart( const std::string& dir )
			{
				node_t * update = _current;
				if( dir == "." )
				{
				}
				else if( dir == ".." )
				{
					update = _current->parent();
				}
				else
				{
					update = _current->get_node( dir );
				}

				if( !update )
				{
					if( !_force ) return VIN_PATH_NOT_FOUND;

					update = _current->add_child( dir );

					if( !update ) return VIN_PATH_NOT_FOUND;
				}

				_current = update;

				vin::KoBase* ob = update->mounted();
				if( ob && ob->as_class(vin::TKoSymbolLink) )
					return VIN_PATH_NOT_FOUND;

				return VIN_NO_ERROR;
			}
		};

		struct finder : pather
		{
		protected:
			node_t * _current;
			bool	 _reparse;
		public:
			finder( node_t * current, bool reparse = true ) 
				: _current(current)
				, _reparse(reparse)
			{
			}
			node_t * current()
			{
				return _current;
			}
			void reset( node_t * p )
			{
				_current = p;
			}
			virtual long onPathPart( const std::string& dir )
			{
				if( dir == "." )
				{
				}
				else if( dir == ".." )
				{
					_current = _current->parent();
				}
				else
				{
					_current = _current->get_node( dir );
				}

				if( !_current )
					return VIN_PATH_NOT_FOUND;

				if( _reparse )
				{
					vin::KoMountable* ob = _current->mounted();
					if( ob && ob->as_class(vin::TKoSymbolLink) )
						return VIN_NEED_REPARSE;
				}

				return VIN_NO_ERROR;
			}
		};
	};

	struct KsObTree
	{
	protected:
		vfs::node_t		_root;
	public:
		KsObTree();
		vfs::node_t*	get_root();
		vfs::node_t*	create_path( const char * path, bool force = true, KoMountable* ob = 0, vfs::node_t * from = 0 );
		long			eares_path( const char * path );
		vfs::node_t*	get_node_by_path( const char * path, bool reparse = true, vfs::node_t * root = 0);
		vfs::node_t*	get_next( vfs::node_t * parent, vfs::node_t * current );
	};

	struct KoSymbolLink : TKoMountable<TKoSymbolLink>
	{
		std::string	_target;
		bool set_target( const char * p ) 
		{
			_target = p ? p : "";
			return true;
		}
		bool set_target( const std::string& n )
		{
			_target = n;
			return true;
		}
		const char * target_c() 
		{ 
			return _target.c_str(); 
		};
		const std::string& target_o() 
		{ 
			return _target; 
		};
	};

};

#endif