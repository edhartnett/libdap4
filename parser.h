
// -*- c++ -*-

// (c) COPYRIGHT URI/MIT 1994-1999
// Please read the full copyright statement in the file COPYRIGHT.
//
// Authors:
//      jhrg,jimg       James Gallagher (jgallagher@gso.uri.edu)

// Constants, types and function prototypes for use with the DAP parsers. 
//
// jhrg 2/3/96

#ifndef _parser_h
#define _parser_h

#ifndef _error_h
#include "Error.h"
#endif

#ifndef _gseclause_h
#include "GSEClause.h"
#endif

#define YYDEBUG 1
#undef YYERROR_VERBOSE
#define YY_NO_UNPUT 1

#define ID_MAX 256

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

/** #parser_arg# is used to pass parameters to the bison parsers and get
    error codes and objects in return. If #status()# is true, then the
    #object()# returns a pointer to the object built during the parse
    process. If #status()# is false, then the #error()# returns a pointer to
    an Error object.

    Note that the #object()# mfunc returns a void pointer. 
    @memo Pass parameters by reference to a parser.
    */

struct parser_arg {
    void *_object;		// nominally a pointer to an object
    Error *_error;		// a pointer to an Error object
    int _status;		// parser status

    parser_arg() : _object(0), _error(0), _status(1) {}
    parser_arg(void *obj) : _object(obj), _error(0), _status(1) {}
    virtual ~parser_arg() {if (_error) delete _error;}

    void *object() { return _object; }
    void set_object(void *obj) { _object = obj; }
    Error *error() { return _error; }
    void set_error(Error *obj) { _error = obj; }
    int status() { return _status; }
    void set_status(int val = 0) { _status = val; }
};

/** Argument to the GSE parser. Assumes all errors will be signalled by
    throws; Error objects are not returned. */
struct gse_arg {
    GSEClause *_gsec;		// The gse parsed.
    Grid *_grid;		// The Grid being constrained.
    int _status;		// The parser's status.

    gse_arg(): _gsec(0), _grid(0), _status(1) {}
    gse_arg(Grid *g): _gsec(0), _grid(g), _status(1) {}
    virtual ~gse_arg() {}

    void set_gsec(GSEClause *gsec) { _gsec = gsec; }
    GSEClause *get_gsec() { return _gsec; }
    void set_grid(Grid *g) { _grid = g; }
    Grid *get_grid() { return _grid; }
    void set_status(int stat) { _status = stat; }
    int get_status() { return _status; }
};


/** #parser_error()# generates error messages for the various parsers used by
    the DODS core. There are two versions of the function, one which takes a
    #const char *message# and a #const int line_num# and writes the message
    and line number too stderr and a second which takes an additional
    #parser_arg *arg# parameter and writes the error message into an Error
    object which is returned to the caller via the #arg# parameter.

    \note{The second version of this function also accepts a third parameter
    (#const char *context#) which can be used to provide an additional line
    of information beyond what is in the string #message#.}

    @return void 
    @memo Generate error messages for the various parsers.
    */
//@{
void parse_error(parser_arg *arg, const char *s, const int line_num = 0,
		 const char *context = 0);
void parse_error(const char *s, const int line_num = 0, 
		 const char *context = 0) throw (Error);
void parse_error(const string &msg, const int line_num, 
		 const char *context = 0) throw (Error);
//@}

/** Given a string (#const char *src#), save it to the temporary variable
    pointed to by #dst#. If the string is longer than #ID_MAX#, generate and
    error indicating that #src# was truncated to #ID_MAX# characters during
    the copy operation. There are two versions of this function; one calls
    the version of #parser_error()# which writes to stderr. The version which
    accepts the #parser_arg *arg# argument calls the version of
    #parser_error()# which generates and Error object.

    @return void 
    @memo Save a string to a temporary variable during the parse.
    */

void save_str(char *dst, const char *src, const int line_num) throw (Error);
void save_str(string &dst, const char *src, const int);

/** Check to see if #val# is a valid byte value. If not, generate an error
    message using #parser_error()#. There are two versions of #check_byte()#,
    one which calls #parser_error()# and prints an error message to stderr an
    one which calls #parser_error()# and generates and Error object.

    @return Returns: True if #val# is a byte value, False otherwise. 
    @memo Is the value a valid byte?
    */

int check_byte(const char *val);

/** Like #check_byte()# but for 32-bit integers (#check_uint()# is for
    unsigned integers). 

    @memo Is the value a valid integer?
    */

int check_int16(const char *val);
int check_uint16(const char *val);
int check_int32(const char *val);
int check_uint32(const char *val);

/** Like #check_byte()# but for 64-bit float values.

    @memo Is the value a valid float? */

int check_float32(const char *val);
int check_float64(const char *val);

/** Currently this function always returns true.

    @memo Is the value a valid URL? */

int check_url(const char *val);

// $Log: parser.h,v $
// Revision 1.15  2001/01/26 19:48:10  jimg
// Merged with release-3-2-3.
//
// Revision 1.14.4.1  2000/11/30 05:24:46  jimg
// Significant changes and improvements to the AttrTable and DAS classes. DAS
// now is a child of AttrTable, which makes attributes behave uniformly at
// all levels of the DAS object. Alias now work. I've added unit tests for
// several methods in AttrTable and some of the functions in parser-util.cc.
// In addition, all of the DAS tests now work.
//
// Revision 1.14  2000/09/22 02:17:23  jimg
// Rearranged source files so that the CVS logs appear at the end rather than
// the start. Also made the ifdef guard symbols use the same naming scheme and
// wrapped headers included in other headers in those guard symbols (to cut
// down on extraneous file processing - See Lakos).
//
// Revision 1.13  2000/08/02 22:46:50  jimg
// Merged 3.1.8
//
// Revision 1.12.4.1  2000/08/01 21:09:36  jimg
// Destructor is now virtual
//
// Revision 1.12  1999/05/24 20:35:31  jimg
// Moved GSEClause.h and removed second Error.h (what was that doing?) includes
// to fix warning about redefinition of TRUE and FALSE (from within rpc/type.h).
//
// Revision 1.11  1999/04/29 02:29:37  jimg
// Merge of no-gnu branch
//
// Revision 1.10  1999/03/24 23:28:36  jimg
// Added support for the new Int16, UInt16 and Float32 types.
// Removed some unused versions of the error printing functions.
//
// Revision 1.9  1999/01/21 20:42:01  tom
// Fixed comment formatting problems for doc++
//
// Revision 1.8  1999/01/21 02:14:37  jimg
// Added a new struct for the GSE (Grid Selection Expression) parser. This
// parser uses exceptions to signal all errors except parse errors.
//
// Revision 1.7  1998/11/10 00:45:15  jimg
// Fixed a memory leak in the parser_arg dtor. If the Error object is created
// the dtor must delete it.
//
// Revision 1.6.14.1  1999/02/05 09:32:36  jimg
// Fixed __unused__ so that it not longer clashes with Red Hat 5.2 inlined
// math code. 
//
// Revision 1.6  1997/02/28 01:30:53  jimg
// Tweaked comments.
//
// Revision 1.5  1996/10/28 18:54:05  jimg
// Added prototypes for check_uint(...); functions that test unsigned integers.
//
// Revision 1.4  1996/08/13 20:45:44  jimg
// Extended definition of parser_arg so that it can serve as the interface for
// all the parsers.
// Added prototypes for all the new check_* functions.
//
// Revision 1.3  1996/05/31 23:31:07  jimg
// Updated copyright notice.
//
// Revision 1.2  1996/05/14 15:39:08  jimg
// These changes have already been checked in once before. However, I
// corrupted the source repository and restored it from a 5/9/96 backup
// tape. The previous version's log entry should cover the changes.
//
// Revision 1.1  1996/02/03 07:35:44  jimg
// Added.

#endif // _parser_h
 
