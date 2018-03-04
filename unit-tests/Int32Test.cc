// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2002,2003 OPeNDAP, Inc.
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

#include "config.h"

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include <sstream>

#include "crc.h"
#include "Byte.h"
#include "Int16.h"
#include "UInt16.h"
#include "Int32.h"
#include "UInt32.h"
#include "Float32.h"
#include "Float64.h"
#include "Str.h"
#include "Url.h"
#include "Array.h"
#include "Structure.h"
#include "Sequence.h"
#include "Grid.h"

#include "DDS.h"

#include "GNURegex.h"
#include "GetOpt.h"
#include "util.h"
#include "debug.h"

#include "testFile.h"
#include "test_config.h"

static bool debug = false;

#undef DBG
#define DBG(x) do { if (debug) {x;} } while(false)

using namespace CppUnit;
using namespace std;

namespace libdap {

class Int32Test: public TestFixture {
private:
    Int32 *i1, *i2;
    
public:
    Int32Test() : i1(0), i2(0)
    {
    }
    ~Int32Test()
    {
    }

    void setUp()
    {
        i1 = new Int32("a", "b");
        i2 = new Int32("e");
    }

    void tearDown()
    {
        delete i1;
        delete i2;
    }

    CPPUNIT_TEST_SUITE(Int32Test);

    CPPUNIT_TEST(cons_Int32_test);
    CPPUNIT_TEST(checksum_test);
    CPPUNIT_TEST(val2buf_test);
    CPPUNIT_TEST(buf2val_test);
    CPPUNIT_TEST(set_value_test);
    CPPUNIT_TEST(equals_test);

    CPPUNIT_TEST_SUITE_END();

    void cons_Int32_test()
    {
        CPPUNIT_ASSERT(1);
        CPPUNIT_ASSERT(i1->value() == 0 && i1->dataset() == "b" && i1->name() == "a" &&
                       i1->type() == dods_int32_c);
        CPPUNIT_ASSERT(i2->value() == 0);
    }

    void checksum_test()
    {
        Crc32 cs;
        i2->compute_checksum(cs);
    }

    void val2buf_test()
    {
        int i = 42;
        i2->val2buf(&i, true);
        CPPUNIT_ASSERT(i2->value() == 42);        
        CPPUNIT_ASSERT_THROW(i2->val2buf(NULL, true), InternalErr);
    }

    void buf2val_test()
    {
        int i = 42;
        void *v = &i;
        void *v2 = NULL;
        CPPUNIT_ASSERT(i2->set_value(0));
        CPPUNIT_ASSERT(i2->buf2val(&v) == 4 && i == 0);
        CPPUNIT_ASSERT_THROW(i2->buf2val(NULL), InternalErr);
        CPPUNIT_ASSERT(i2->buf2val(&v2) == 4 && *(int *)v2 == 0);
    }

    void set_value_test()
    {
        CPPUNIT_ASSERT(i2->set_value(42) && i2->value() == 42);        
    }

    void equals_test()
    {
        Int32 *i3 = new Int32("a", "b");
        Int32 *i4 = new Int32("e");
        CPPUNIT_ASSERT(i4->set_value(42) && i4->value() == 42);
        i4 = i3;
        CPPUNIT_ASSERT(i4->value() == 0 && i4->dataset() == "b" && i4->name() == "a" &&
                       i4->type() == dods_int32_c);
    }    
};

CPPUNIT_TEST_SUITE_REGISTRATION(Int32Test);

} // namespace libdap

int main(int argc, char *argv[])
{
    GetOpt getopt(argc, argv, "dh");
    int option_char;

    while ((option_char = getopt()) != -1)
        switch (option_char) {
        case 'd':
            debug = 1;  // debug is a static global
            break;

        case 'h': {     // help - show test names
            cerr << "Usage: Int32Test has the following tests:" << endl;
            const std::vector<Test*> &tests = libdap::Int32Test::suite()->getTests();
            unsigned int prefix_len = libdap::Int32Test::suite()->getName().append("::").length();
            for (std::vector<Test*>::const_iterator i = tests.begin(), e = tests.end(); i != e; ++i) {
                cerr << (*i)->getName().replace(0, prefix_len, "") << endl;
            }
            return 1;
            break;
        }

        default:
            break;
        }

    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    bool wasSuccessful = true;
    string test = "";
    int i = getopt.optind;
    if (i == argc) {
        // run them all
        wasSuccessful = runner.run("");
    }
    else {
        for (; i < argc; ++i) {
            if (debug) cerr << "Running " << argv[i] << endl;
            test = libdap::Int32Test::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}

