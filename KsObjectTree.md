
```
	vin::kvfs obfs;

	obfs.createPath( "\\Device\\HarddiskVolume1\\Windows\\System32\\drivers\\etc" );
	obfs.createPath( "\\.Images\\Device\\HarddiskVolume1\\Windows\\System32\\kernel32.dll", true, new KoSection() );

	obfs.createPath( "\\Device\\HarddiskVolume1\\Windows\\System32\\kernel32.dll", true, new vin::KoWkmImage() );
	obfs.createPath( "\\Device\\HarddiskVolume1\\windows\\System32\\ntdll.dll", true, new vin::KoWkmImage() );
	obfs.createPath( "\\Device\\HarddiskVolume1\\windows\\System32\\advapi32.dll", true, new vin::KoWkmImage() );
	obfs.createPath( "\\Device\\HarddiskVolume1\\windows\\System32\\user32.dll", true, new vin::KoWkmImage() );
	obfs.createPath( "\\Device\\HarddiskVolume1\\windows\\System32\\gdi32.dll", true, new vin::KoWkmImage() );

	obfs.createPath( "\\BaseNamedObjects\\c:", true, new vin::KoSymbolLink("\\Device\\HarddiskVolume1") );
	obfs.createPath( "\\BaseNamedObjects\\d:", true, new vin::KoSymbolLink("\\Device\\harddiskvolume2") );

	obfs.createPath( "\\Global", true, new vin::KoSymbolLink("\\BaseNamedObjects") );
	obfs.createPath( "\\Local", true, new vin::KoSymbolLink("\\BaseNamedObjects") );
	obfs.createPath( "\\Session", true, new vin::KoSymbolLink("\\Session\\BNOLINKS") );

	obfs.createPath( "\\SystemRoot", true, new vin::KoSymbolLink("\\Device\\BootDevice") );
	obfs.createPath( "\\??", true, new vin::KoSymbolLink("\\BaseNamedObjects") );

	// all dos device need put at root, 
	obfs.createPath( "\\c:", true, new vin::KoSymbolLink("\\Device\\HarddiskVolume1") );
	obfs.createPath( "\\d:", true, new vin::KoSymbolLink("\\Device\\harddiskvolume2") );

	obfs.createPath( "\\DosDevice", true, new vin::KoSymbolLink("\\") );

	// all dos drives put to \\.DosDrives, help converting UNC style path to Dos style path, 
	obfs.createPath( "\\.DosDrives\\c:", true, new vin::KoSymbolLink("\\Device\\HarddiskVolume1") );
	obfs.createPath( "\\.DosDrives\\d:", true, new vin::KoSymbolLink("\\Device\\HarddiskVolume2") );

	obfs.createPath( "\\Device\\BootDevice", true, new vin::KoSymbolLink("\\Device\\HarddiskVolume1") );

	obfs.createPath( "\\Device\\HarddiskVolume1\\Windows\\SysWOW64", true, new vin::KoSymbolLink("\\Device\\HarddiskVolume1\\windows\\system32") );

	obfs.createPath( "\\Device\\HarddiskVolume1\\Windows\\SysWOW64", true, new vin::KoSymbolLink("\\Device\\HarddiskVolume1\\windows\\system32") );

	//vin::vfs::node_t * kernel32 = obfs.getNodeByPath( "\\\\knowndlls\\lsp.dll" );
	//vin::KoSymbolLink * x =  kernel32->mounted()->instantiate<vin::KoSymbolLink>();

	vin::vfs::node_t * system32 = obfs.getNodeByPath( "\\??\\c:\\windows\\system32\\" );
	vin::vfs::node_t * user32 = obfs.getNodeByPath( "\\\\.\\c:\\windows\\SysWOW64\\kernel32.dll\\" );
	vin::KoWkmImage* of = ( user32->mounted()->instantiate<vin::KoWkmImage>() );
	if( of )
	{
		of->setAccess( FILE_READ_DATA );
	}
	vin::vfs::node_t * drives = obfs.getNodeByPath( "\\.DosDrives" );

	//vin::vfs::node_t * vol_c = obfs.getNodeByPath( "c:", false );
	std::string fname;
	user32->getFullPath( fname );
	printf( "%s\n", fname.c_str() );

	vin::vfs::node_t * file = 0;
	while( file = drives->next( file ) )
	{
		std::string tmp;
		file->getFullPath(tmp);
		printf( "%s\n", tmp.c_str() );
	}
```