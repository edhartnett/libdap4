
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2002,2003 OPeNDAP, Inc.
// Author: Patrick West <pwest@ucar.edu>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// 
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
 
// pwest 12/02/2002

#ifdef __GNUG__
#pragma implementation
#endif

#include "ArrayIterAdapter.h"

ArrayIterAdapter::ArrayIterAdapter( std::vector<Array::dimension> &vec ) :
    _vec( vec )
{
}

ArrayIterAdapter::~ArrayIterAdapter( )
{
}

void
ArrayIterAdapter::first( )
{
    _i = _vec.begin() ;
}

void
ArrayIterAdapter::next( )
{
    _i++ ;
}

ArrayIterAdapter::operator bool( )
{
    return ( _i != _vec.end() ) ;
}

bool
ArrayIterAdapter::operator==( const IteratorAdapter &i )
{ 
    return _i == ((const ArrayIterAdapter &)i)._i ; 
}

Array::dimension
ArrayIterAdapter::entry( )
{
    return *_i ;
}

ArrayIterAdapter::iter &
ArrayIterAdapter::getIterator( )
{
    return _i ;
}

// $Log: ArrayIterAdapter.cc,v $
// Revision 1.5  2003/04/02 19:14:19  pwest
// Added doxygen comments to iterator adapter classes
//
//
