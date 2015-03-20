
```

	struct KoSymbolLink : KoMountable
	{
		enum { ObId = TKoSymbolLink };
		std::string target;
		KoSymbolLink( const char * p ) 
		{
			target = p;
		}
		virtual size_t mainType()
		{
			return ObId;
		}
		virtual void* asType( KoType faceid )
		{
			if( faceid != ObId ) return 0;
			return this;
		}
	};


```