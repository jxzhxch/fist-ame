#ifndef __D_LINK_LIST__
#define __D_LINK_LIST__

#include "../include/refable.h"

// struct T : dllist::node
template<class T>
struct dllist
{
	struct node
	{
		T*	_prev;
		T*	_next;
		node() : _prev(0), _next(0)
		{}
	};

	T*	_first;
	T*	_last;

	dllist() : _first(0), _last(0){}

	~dllist()
	{
		destroy();
	}

	void destroy()
	{
		while( _first )
		{
			T* todel = _first;
			_first = todel->_next;
			delete todel;
		}
		_first = 0;
		_last = 0;
	}

	void setFirst( T* self )
	{
		destroy();
		_first = self;
		_last = self;
	}

	void linkNext( T* self, T* insert )
	{
		insert->_next = self->_next;
		insert->_prev = self;
		self->_next = insert;
		if( !insert->_next ) _last = insert;
	}
	void linkPrev( T* self, T* insert )
	{
		insert->_prev = self->_prev;
		insert->_next = self;
		self->_prev = insert;
		if( !insert->_prev ) _first = insert;
	}
	void unlink( T* self )
	{
		if( self->_prev ) 
		{
			self->_prev->_next = self->_next;
		}
		else
		{
			_first = self->_next;
		}
		if( self->_next )
		{
			self->_next->_prev = self->_prev;
		}
		else
		{
			_last = self->_prev;
		}
		self->_next = 0;
		self->_prev = 0;
		delete self;
	}
};

#endif