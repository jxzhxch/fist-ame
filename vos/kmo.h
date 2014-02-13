#ifndef __KMO__
#define __KMO__

#include <set>
#include "vin64.h"
#include "kbo.h"

namespace vin
{
	struct KmoSectionUser
	{
		virtual void onBlockInvalid( uint64_t offset ) = 0;
	};

	class KmoSection : vram::RamBlockOwner
	{
	protected:

		enum { WINDOW_BLOCKS = 64 };

		struct Block : vram::RamBlock
		{
			uint64_t offset;
		};

		typedef std::set<KmoSectionUser*> users_t;

		Block		_wnd[WINDOW_BLOCKS];
		uint64_t	_size;
		uint32_t	_acc;
		KboFileIO*	_file;
		users_t		_users;

	public:

		KmoSection();

		HRESULT create( KboFileIO* f, uint64_t size, uint32_t acc );

		HRESULT close( );

		void insertUser( KmoSectionUser* p );

		void removeUser( KmoSectionUser* p );

		vram::RamBlock* getRamBlock( uint64_t offset );

		virtual void onBlockInvalid( vram::RamBlock* pb );
	};



};

#endif