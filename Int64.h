
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2012 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

#ifndef _int64_h
#define _int64_h 1

#ifndef _dods_datatypes_h
#include "dods-datatypes.h"
#endif

#ifndef _basetype_h
#include "BaseType.h"
#endif

namespace libdap
{

class ConstraintEvaluator;
class Marshaller;
class UnMarshaller;

/** @brief Holds a64-bit signed integer.

    @see BaseType
    */

class Int64: public BaseType
{
	virtual unsigned int val2buf(void *val, bool)  {
    	set_value(*reinterpret_cast<dods_int64*>(val));
    	return sizeof(dods_int64);
    }
    virtual unsigned int buf2val(void **) { throw InternalErr(__FILE__, __LINE__, "Not implemented for Int64"); }
    virtual void print_val(FILE *, string, bool) { throw InternalErr(__FILE__, __LINE__, "Not implemented for Int64"); }

protected:
    dods_int64 d_buf;

public:
    Int64(const string &n);
    Int64(const string &n, const string &d);

    Int64(const Int64 &copy_from);

    Int64 &operator=(const Int64 &rhs);

    virtual ~Int64();

    virtual BaseType *ptr_duplicate();

    virtual unsigned int width(bool constrained = false) const;

    // DAP4
    virtual void compute_checksum(Crc32 &checksum);
    virtual void serialize(D4StreamMarshaller &m, DMR &dmr, /*ConstraintEvaluator &eval,*/ bool filter = false);
    virtual void deserialize(D4StreamUnMarshaller &um, DMR &dmr);

    virtual bool set_value(dods_int64 i);
    virtual dods_int64 value() const;

    virtual void print_val(ostream &out, string space = "", bool print_decl_p = true);

    virtual bool ops(BaseType *b, int op);
    virtual bool d4_ops(BaseType *b, int op);
    virtual std::vector<BaseType *> *transform_to_dap2(AttrTable *parent_attr_table);

    virtual void dump(ostream &strm) const ;
};

} // namespace libdap

#endif // _int64_h

