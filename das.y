
// -*- C++ -*-

// (c) COPYRIGHT URI/MIT 1994-1999
// Please read the full copyright statement in the file COPYRIGHT.
//
// Authors:
//      jhrg,jimg       James Gallagher (jgallagher@gso.uri.edu)

/*
   Grammar for the DAS. This grammar can be used with the bison parser
   generator to build a parser for the DAS. It assumes that a scanner called
   `daslex()' exists and that the objects DAS and AttrTable also exist.

   jhrg 7/12/94 
*/

%{

#define YYSTYPE char *

#include "config_dap.h"

static char rcsid[] not_used = {"$Id: das.y,v 1.41 2001/01/26 19:48:09 jimg Exp $"};

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <assert.h>

#if defined(__GNUG__) || defined(WIN32)
#include <strstream>
#else
#include <sstream>
#endif
#include <vector>

#include "DAS.h"
#include "Error.h"
#include "debug.h"
#include "parser.h"
#include "das.tab.h"

#ifdef TRACE_NEW
#include "trace_new.h"
#endif

#ifdef WIN32
using std::ends;
using std::ostrstream;
#endif

// These macros are used to access the `arguments' passed to the parser. A
// pointer to an error object and a pointer to an integer status variable are
// passed in to the parser within a strucutre (which itself is passed as a
// pointer). Note that the ERROR macro explicitly casts OBJ to an ERROR *. 

#define DAS_OBJ(arg) ((DAS *)((parser_arg *)(arg))->_object)

#define YYPARSE_PARAM arg

extern int das_line_num;	/* defined in das.lex */

// No global static objects. We go through this every so often, I guess I
// should learn... 1/24/2000 jhrg
static string *name;	/* holds name in attr_pair rule */
static string *type;	/* holds type in attr_pair rule */

static vector<AttrTable *> *attr_tab_stack;

// I use a vector of AttrTable pointers for a stack

#define TOP_OF_STACK (attr_tab_stack->back())
#define PUSH(x) (attr_tab_stack->push_back((x)))
#define POP (attr_tab_stack->pop_back())
#define STACK_LENGTH (attr_tab_stack->size())
#define OUTER_TABLE_ONLY (attr_tab_stack->size() == 1)
#define STACK_EMPTY (attr_tab_stack->empty())

#define TYPE_NAME_VALUE(x) *type << " " << *name << " " << (x)

static char *ATTR_TUPLE_MSG = 
"Expected an attribute type (Byte, Int16, UInt16, Int32, UInt32, Float32,\n\
Float64, String or Url) followed by a name and value.";
static char *NO_DAS_MSG =
"The attribute object returned from the dataset was null\n\
Check that the URL is correct.";

typedef int checker(const char *);

#if 0
void mem_list_report();
#endif
int daslex(void);
static void daserror(char *s);
#if 0
static string attr_name(string name);
#endif
static void add_attribute(const string &type, const string &name, 
			  const string &value, checker *chk) throw (Error);
static void add_alias(AttrTable *das, AttrTable *current, const string &name, 
		      const string &src) throw (Error);

%}

%expect 24

%token SCAN_ATTR

%token SCAN_ID
%token SCAN_INT
%token SCAN_FLOAT
%token SCAN_STR
%token SCAN_ALIAS

%token SCAN_BYTE
%token SCAN_INT16
%token SCAN_UINT16
%token SCAN_INT32
%token SCAN_UINT32
%token SCAN_FLOAT32
%token SCAN_FLOAT64
%token SCAN_STRING
%token SCAN_URL

%%

/*
  Parser algorithm: 

  Look for a `variable' name (this can be any identifier, but by convention
  it is either the name of a variable in a dataset or the name of a grouping
  of global attributes). Create a new attribute table for this identifier and
  push the new attribute table onto a stack. If attribute tuples
  (type-name-value tuples) are found, intern them in the attribute table
  found on the top of the stack. If the start of a new attribute table if
  found (before the current table is closed), create the new table and push
  *it* on the stack. As attribute tables are closed, pop them off the stack.
  This algorithm ensures that we can nest attribute tables to an arbitrary
  depth.

  Aliases are handled using mfuncs of both the DAS and AttrTable objects. This
  is necessary because the first level of a DAS object can contain only
  AttrTables, not attribute tuples. Whereas, the subsequent levels can
  contain both. Thus the compete definition is split into two objects. In
  part this is also a hold over from an older design which did not
  have the recursive properties of the current design.

  Aliases can be made between attributes within a given lexical level, from
  one level to the next within a sub-hierarchy or across hierarchies.

  Tokens:

  BYTE, INT32, UINT32, FLOAT64, STRING and URL are tokens for the type
  keywords. The tokens INT, FLOAT, STR and ID are returned by the scanner to
  indicate the type of the value represented by the string contained in the
  global DASLVAL. These two types of tokens are used to implement type
  checking for the attributes. See the rules `bytes', etc. Additional tokens:
  ATTR (indicates the start of an attribute object) and ALIAS (indicates an
  alias). */

/* This rule makes sure the objects needed by this parser are built. Because
   the DODS DAP library is often used with linkers that are not C++-aware, we
   cannot use global objects (because their constructors might never be
   called). I had thought this was going to go away... 1/24/2000 jhrg */

attr_start:
                {
		    name = new string();
		    type = new string();
		    attr_tab_stack = new vector<AttrTable *>;
		    PUSH(DAS_OBJ(arg)); // push outermost AttrTable
		}
                attributes
                {
		    POP;	// pop the DAS/AttrTable before stack's dtor
		    delete name;
		    delete type;
		    delete attr_tab_stack;
		}
;

attributes:     attribute
    	    	| attributes attribute

;
    	    	
attribute:    	SCAN_ATTR '{' attr_list '}'
                | error
                {
		    parse_error((parser_arg *)arg, NO_DAS_MSG, das_line_num);
		}
;

attr_list:  	/* empty */
    	    	| attr_tuple
    	    	| attr_list attr_tuple
;

attr_tuple:	alias

                | SCAN_BYTE { *type = "Byte"; }
                SCAN_ID { *name = $3; } 
		bytes ';'

		| SCAN_INT16 { save_str(*type, "Int16", das_line_num); } 
                SCAN_ID { save_str(*name, $3, das_line_num); } 
		int16 ';'

		| SCAN_UINT16 { save_str(*type, "UInt16", das_line_num); } 
                SCAN_ID { save_str(*name, $3, das_line_num); } 
		uint16 ';'

		| SCAN_INT32 { save_str(*type, "Int32", das_line_num); } 
                SCAN_ID { save_str(*name, $3, das_line_num); } 
		int32 ';'

		| SCAN_UINT32 { save_str(*type, "UInt32", das_line_num); } 
                SCAN_ID { save_str(*name, $3, das_line_num); } 
		uint32 ';'

		| SCAN_FLOAT32 { save_str(*type, "Float32", das_line_num); } 
                SCAN_ID { save_str(*name, $3, das_line_num); } 
		float32 ';'

		| SCAN_FLOAT64 { save_str(*type, "Float64", das_line_num); } 
                SCAN_ID { save_str(*name, $3, das_line_num); } 
		float64 ';'

		| SCAN_STRING { *type = "String"; } 
                SCAN_ID { *name = $3; } 
		strs ';'

		| SCAN_URL { *type = "Url"; } 
                SCAN_ID { *name = $3; } 
		urls ';'

		| SCAN_ID 
                {
		    DBG(cerr << "Processing ID: " << $1 << endl);
		    
		    AttrTable *at = TOP_OF_STACK->get_attr_table($1);
		    if (!at) {
			try {
			    at = TOP_OF_STACK->append_container($1);
			}
			catch (Error &e) {
			    // rethrow with line number info
			    parse_error(e.error_message().c_str(), 
					das_line_num);
			}
		    }
		    PUSH(at);

		    DBG(cerr << " Pushed attr_tab: " << at << endl);

		}
		'{' attr_list 
                {
		    /* pop top of stack; store in attr_tab */
		    DBG(cerr << " Poped attr_tab: " << TOP_OF_STACK << endl);
		    POP;
		}
		'}'

		| error 
                { 
		    parse_error(ATTR_TUPLE_MSG, das_line_num, $1);
		} ';'
;

bytes:		SCAN_INT
		{
		    add_attribute(*type, *name, $1, &check_byte);
		}
		| bytes ',' SCAN_INT
		{
		    add_attribute(*type, *name, $3, &check_byte);
		}
;

int16:		SCAN_INT
		{
		    add_attribute(*type, *name, $1, &check_int16);
		}
		| int16 ',' SCAN_INT
		{
		    add_attribute(*type, *name, $3, &check_int16);
		}
;

uint16:		SCAN_INT
		{
		    add_attribute(*type, *name, $1, &check_uint16);
		}
		| uint16 ',' SCAN_INT
		{
		    add_attribute(*type, *name, $3, &check_uint16);
		}
;

int32:		SCAN_INT
		{
		    add_attribute(*type, *name, $1, &check_int32);
		}
		| int32 ',' SCAN_INT
		{
		    add_attribute(*type, *name, $3, &check_int32);
		}
;

uint32:		SCAN_INT
		{
		    add_attribute(*type, *name, $1, &check_uint32);
		}
		| uint32 ',' SCAN_INT
		{
		    add_attribute(*type, *name, $3, &check_uint32);
		}
;

float32:	float_or_int
		{
		    add_attribute(*type, *name, $1, &check_float32);
		}
		| float32 ',' float_or_int
		{
		    add_attribute(*type, *name, $3, &check_float32);
		}
;

float64:	float_or_int
		{
		    add_attribute(*type, *name, $1, &check_float64);
		}
		| float64 ',' float_or_int
		{
		    add_attribute(*type, *name, $3, &check_float64);
		}
;

strs:		str_or_id
		{
		    add_attribute(*type, *name, $1, 0);
		}
		| strs ',' str_or_id
		{
		    add_attribute(*type, *name, $3, 0);
		}
;

urls:		url
		{
		    add_attribute(*type, *name, $1, &check_url);
		}
		| urls ',' url
		{
		    add_attribute(*type, *name, $3, &check_url);
		}
;

url:		SCAN_ID | SCAN_STR
;

str_or_id:	SCAN_STR | SCAN_ID | SCAN_INT | SCAN_FLOAT
;

float_or_int:   SCAN_FLOAT | SCAN_INT
;

alias:          SCAN_ALIAS SCAN_ID 
                { 
		    *name = $2;
		} 
                SCAN_ID
                {
		    add_alias(DAS_OBJ(arg), TOP_OF_STACK, *name, string($4))
                }
                ';'
;
%%

// This function is required for linking, but DODS uses its own error
// reporting mechanism.

static void
daserror(char *)
{
}

// Return the rightmost component of name (where each component is separated
// by `.'.

#if 0
static string
attr_name(string name)
{
    string::size_type i = name.rfind('.');
    if(i == string::npos)
      return name;
    else
      return name.substr(i+1);
}
#endif

static string
a_or_an(const string &subject)
{
    string first_char(1, subject[0]);
    string::size_type pos = first_char.find_first_of("aeiouAEIOUyY");
    
    if (pos == string::npos)
	return "a";
    else
	return "an";
}

static void
add_attribute(const string &type, const string &name, const string &value,
	      checker *chk) throw (Error)
{
    DBG(cerr << "Adding: " << type << " " << name << " " << value \
	<< " to Attrtable: " << TOP_OF_STACK << endl);

    if (chk && !(*chk)(value.c_str())) {
	string msg = "`";
	msg += value + "' is not " + a_or_an(type) + " " + type + " value.";
	parse_error(msg.c_str(), das_line_num);	// throws Error.
    }
    
    if (STACK_EMPTY) {
	string msg = "Whoa! Attribute table stack empty when adding `" ;
	msg += name + "' .";
	parse_error(msg.c_str(), das_line_num);
    }
    
    try {
	TOP_OF_STACK->append_attr(name, type, value);
    }
    catch (Error &e) {
	// rethrow with line number
	parse_error(e.error_message().c_str(), das_line_num);
    }
}

static void
add_alias(AttrTable *das, AttrTable *current, const string &name, 
	  const string &src) throw (Error)
{
    DBG(cerr << "Adding an alias: " << name << ": " << src << endl);

    AttrTable *table = das->get_attr_table(src);
    if (table) {
	try {
	    current->add_container_alias(name, table);
	}
	catch (Error &e) {
	    parse_error(e.error_message().c_str(), das_line_num);
	}
    }
    else {
	try {
	    current->add_value_alias(das, name, src);
	}
	catch (Error &e) {
	    parse_error(e.error_message().c_str(), das_line_num);
	}
    }
}

/* 
 * $Log: das.y,v $
 * Revision 1.41  2001/01/26 19:48:09  jimg
 * Merged with release-3-2-3.
 *
 * Revision 1.40.4.2  2000/11/30 05:24:46  jimg
 * Significant changes and improvements to the AttrTable and DAS classes. DAS
 * now is a child of AttrTable, which makes attributes behave uniformly at
 * all levels of the DAS object. Alias now work. I've added unit tests for
 * several methods in AttrTable and some of the functions in parser-util.cc.
 * In addition, all of the DAS tests now work.
 *
 * Revision 1.40.4.1  2000/11/22 21:47:42  jimg
 * Changed the implementation of DAS; it now inherits from AttrTable
 *
 * Revision 1.40  2000/09/22 02:17:22  jimg
 * Rearranged source files so that the CVS logs appear at the end rather than
 * the start. Also made the ifdef guard symbols use the same naming scheme and
 * wrapped headers included in other headers in those guard symbols (to cut
 * down on extraneous file processing - See Lakos).
 *
 * Revision 1.39  2000/07/09 21:43:29  rmorris
 * Mods to increase portability, minimize ifdef's for win32
 *
 * Revision 1.38  2000/06/07 19:33:21  jimg
 * Merged with verson 3.1.6
 *
 * Revision 1.37  2000/06/07 18:07:00  jimg
 * Merged the pc port branch
 *
 * Revision 1.36.6.1  2000/06/02 18:36:38  rmorris
 * Mod's for port to Win32.
 *
 * Revision 1.35.6.3  2000/05/18 17:47:21  jimg
 * Fixed a bug in the AttrTable. Container attributes below the top level were
 * broken in the latest changes to the DAS code.
 *
 * Revision 1.36  2000/01/27 06:30:00  jimg
 * Resolved conflicts from merge with release-3-1-4
 *
 * Revision 1.35.6.2  2000/01/24 22:25:10  jimg
 * Removed static global objects
 *
 * Revision 1.35.6.1  1999/10/19 16:45:14  jimg
 * Fixed a minor bug in the check of int16 attributes. the check_int16 was
 * called when check_uint16 should have been called.
 *
 * Revision 1.35  1999/05/04 19:47:23  jimg
 * Fixed copyright statements. Removed more of the GNU classes.
 *
 * Revision 1.34  1999/04/29 02:29:35  jimg
 * Merge of no-gnu branch
 *
 * Revision 1.33  1999/03/24 23:33:44  jimg
 * Added support for the new Int16, UInt16 and Float32 types.
 *
 * Revision 1.32.12.2  1999/02/05 09:32:35  jimg
 * Fixed __unused__ so that it not longer clashes with Red Hat 5.2 inlined
 * math code. 
 *
 * Revision 1.32.12.1  1999/02/02 21:57:06  jimg
 * String to string version
 *
 * Revision 1.32  1997/07/01 00:13:23  jimg
 * Fixed a bug when vectors of UInt32 were used. I changed the way the type
 * name was passed to AttrTable::append_attr() so that the names were always
 * the same regardless of form of the name used in the DAS.
 * Fixed a bug when Urls are used as an attribute type.
 *
 * Revision 1.31  1997/05/21 00:10:35  jimg
 * Added a fix for aliases between top level groups of attributes.
 *
 * Revision 1.30  1997/05/13 23:32:19  jimg
 * Added changes to handle the new Alias and lexical scoping rules.
 *
 * Revision 1.29  1997/05/06 22:09:57  jimg
 * Added aliases to the grammar. An alias can appear in place of an attribute
 * and uses the syntax `alias <var1> <var2>'. If var1 exists, var2 becomes an
 * alias to it and vice versa. If neither var1 nor var2 exists or if they both
 * exist, and error is reported and parsing stops.
 *
 * Revision 1.28  1997/02/28 01:01:07  jimg
 * Tweaked error messages so that they no longer mumble about parse errors.
 *
 * Revision 1.27  1997/02/10 02:36:57  jimg
 * Fixed bug where attribute type of int32 was broken on 64bit machines.
 *
 * Revision 1.26  1996/10/28 23:04:46  jimg
 * Added unsigned int to set of possible attribute value types.
 *
 * Revision 1.25  1996/10/11 00:11:03  jimg
 * Fixed DODS_BISON_VER preprocessor statement. >= apparently is not recognized
 * by g++'s preprocessor.
 *
 * Revision 1.24  1996/10/08 17:04:40  jimg
 * Added a fix for Bison 1.25 so that PARSE_PARAM will still work
 *
 * Revision 1.23  1996/08/13 18:46:38  jimg
 * Added parser_arg object macros.
 * `Fixed' error messages.
 * Changed return typw of daserror() from int to void.
 *
 * Revision 1.22  1996/06/07 15:05:16  jimg
 * Removed old type checking code - use the type checkers in parser-util.cc.
 *
 * Revision 1.21  1996/05/31 23:30:52  jimg
 * Updated copyright notice.
 *
 * Revision 1.20  1996/04/05 00:22:13  jimg
 * Compiled with g++ -Wall and fixed various warnings.
 *
 * Revision 1.19  1995/12/06  19:46:29  jimg
 * Added definition of YYSTYPE.
 *
 * Revision 1.18  1995/10/23  22:54:39  jimg
 * Removed code that was NEVER'd.
 * Changed some rules so that they call functions in parser_util.cc .
 *
 * Revision 1.17  1995/09/05  23:19:45  jimg
 * Fixed a bug in check_float where `=' was used where `==' should have been.
 *
 * Revision 1.16  1995/08/23  00:25:54  jimg
 * Added copyright notice.
 * Fixed some bogus comments.
 *
 * Revision 1.15  1995/07/08  18:32:10  jimg
 * Edited comments.
 * Removed unnecessary declarations.
 *
 * Revision 1.14  1995/05/10  13:45:43  jimg
 * Changed the name of the configuration header file from `config.h' to
 * `config_dap.h' so that other libraries could have header files which were
 * installed in the DODS include directory without overwriting this one. Each
 * config header should follow the convention config_<name>.h.
 *
 * Revision 1.13  1995/02/16  15:30:46  jimg
 * Fixed bug which caused Byte, ... values which were out of range to be
 * added to the attribute table anyway.
 * Corrected the number of expected shift-reduce conflicts.
 *
 * Revision 1.12  1995/02/10  02:56:21  jimg
 * Added type checking.
 *
 * Revision 1.11  1994/12/22  04:30:56  reza
 * Made save_str static to avoid linking conflict.
 *
 * Revision 1.10  1994/12/16  22:06:23  jimg
 * Fixed a bug in save_str() where the global NAME was used instead of the
 * parameter DST.
 *
 * Revision 1.9  1994/12/07  21:19:45  jimg
 * Added a new rule (var) and modified attr_val to handle attribute vectors.
 * Each element in the vector is seaprated by a comma.
 * Replaces some old instrumentation code with newer code using the DGB
 * macros.
 *
 * Revision 1.8  1994/11/10  19:50:55  jimg
 * In the past it was possible to have a null file correctly parse as a
 * DAS or DDS. However, now that is not possible. It is possible to have
 * a file that contains no variables parse, but the keyword `Attribute'
 * or `Dataset' *must* be present. This was changed so that errors from
 * the CGIs could be detected (since they return nothing in the case of
 * a error).
 *
 * Revision 1.7  1994/10/18  00:23:18  jimg
 * Added debugging statements.
 *
 * Revision 1.6  1994/10/05  16:46:51  jimg
 * Modified the DAS grammar so that TYPE tokens (from the scanner) were
 * parsed correcly and added to the new AttrTable class.
 * Changed the code used to add entries based on changes to AttrTable.
 * Consoladated error reporting code.
 *
 * Revision 1.5  1994/09/27  23:00:39  jimg
 * Modified to use the new DAS class and new AttrTable class.
 *
 * Revision 1.4  1994/09/15  21:10:56  jimg
 * Added commentary to das.y -- how does it work.
 *
 * Revision 1.3  1994/09/09  16:16:38  jimg
 * Changed the include name to correspond with the class name changes (Var*
 * to DAS*).
 *
 * Revision 1.2  1994/08/02  18:54:15  jimg
 * Added C++ statements to grammar to generate a table of parsed attributes.
 * Added a single parameter to dasparse - an object of class DAS.
 * Solved strange `string accumulation' bug with $1 %2 ... by copying
 * token's semantic values to temps using mid rule actions.
 * Added code to create new attribute tables as each variable is parsed (unless
 * a table has already been allocated, in which case that one is used).
 *
 * Revision 1.2  1994/07/25  19:01:21  jimg
 * Modified scanner and parser so that they can be compiled with g++ and
 * so that they can be linked using g++. They will be combined with a C++
 * method using a global instance variable.
 * Changed the name of line_num in the scanner to das_line_num so that
 * global symbol won't conflict in executables/libraries with multiple
 * scanners.
 *
 * Revision 1.1  1994/07/25  14:26:45  jimg
 * Test files for the DAS/DDS parsers and symbol table software.
 */

