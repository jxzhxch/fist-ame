#ifndef __VIN_ERROR__
#define __VIN_ERROR__

namespace vin
{
	
		enum
		{
			VFS_S_OK = 0,
			VFS_E_REPARSE_POINT_REARCHED = -1,
			VFS_E_PATH_NOT_FOUND = -2,
			VFS_E_FAIL = E_FAIL,
			VFS_E_UNEXPECTED = E_UNEXPECTED,
		};
	enum 
	{
		VIN_NO_ERROR,

		VIN_INVALID_PARAMETER,

		VIN_PATH_NOT_FOUND,

		VIN_NEED_REPARSE,

		VIN_PATH_IS_BUSY,

	};
};


#endif