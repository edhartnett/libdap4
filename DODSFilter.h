
// -*- C++ -*-

// (c) COPYRIGHT URI/MIT 1997
// Please first read the full copyright statement in the file COPYRIGH.  
//
// Authors:
//	jhrg,jimg	James Gallagher (jgallagher@gso.uri.edu)

// $Log: DODSFilter.h,v $
// Revision 1.4  1998/08/06 16:11:47  jimg
// Added cache_dir member (from jeh).
//
// Revision 1.3  1998/02/04 14:55:32  tom
// Another draft of documentation.
//
// Revision 1.2  1997/09/22 23:04:59  jimg
// Added doc++ style comments.
//
// Revision 1.1  1997/08/28 20:39:02  jimg
// Created
//

#ifndef _DODSFilter_h
#define _DODSFilter_h

#ifdef __GNUG__
#pragma "interface"
#endif

#include <String.h>

/** When a DODS server receives a request from a DODS client, the
    server CGI script dispatches the request to one of several
    ``filter'' programs.  Each filter is responsible for returning a
    different aspect of the dataset information: one is for data, one
    is for the dataset DDS, one is for the dataset DAS, and a fourth
    is for a usage message describing the server itself.  Some
    installations may have additional optional filters.

    The filter program receives a data request from the dispatch
    script. It receives its operating parameters from the command
    line, like any UNIX command, and it returns its output to standard
    output, which the httpd server packages up into a reply to the
    client. 

    This class contains some common functions for the filter programs
    used to make up the DODS data servers. The filter programs do not
    {\it have} to be called by a CGI program, but that is the normal
    mechanism by which they are invoked.
  
    @memo Common functions for DODS server filter programs.
    @author jhrg 8/26/97 */

class DODSFilter {
private:
    bool comp;			// True if the output should be compressed.
    bool ver;			// True if the caller wants version info.
    bool bad_options;		// True if the options (argc,argv) are bad.

    String program_name;	// Name of the filter program
    String dataset;		// Name of the dataset/database 
    String ce;			// Constraint expression 
    String cgi_ver;		// Version of CGI script (caller)
    String anc_dir;		// Look here for ancillary files
    String anc_file;		// Use this for ancillary file name
    String cache_dir;		// Use this for cache files

    DODSFilter() {}		// Private default ctor.

public:
  /** Create an instance of DODSFilter using the command line
      arguments passed by the CGI (or other) program.  The default
      constructor is private; this and the copy constructor (which is
      just the default constructor) are the only way to create an
      instance of DODSFilter.

      These are the valid options:
      \begin{description}
      \item[{\it filename}]
      The name of the file on which the filter is to operate.  Usually
      this would be the file whose data has been requested.
      \item[#-c#]
      Send compressed data.  This is not implemented as of DODS
      version 2.14.
      \item[#-e# {\it expression}]
      This option specifies a non-blank constraint expression used to
      subsample a dataset.
      \item[#-v# {\it cgi-version}]
      Specifies that this request is just for version information.
      The {\it cgi-version} is the version of the dispatch CGI that
      invoked this command.  This is passed to the filter program so
      that it can be formatted into the returned message.
      \item[#-d# {\it ancdir}]
      Specifies that ancillary data be sought in the {\it ancdir}
      directory. 
      \item[#-f# {\it ancfile}]
      Specifies that ancillary data may be found in a file called {\it
      ancfile}.
      \end{description}

      @memo DODSFilter constructor.
      */
    DODSFilter(int argc, char *argv[]);

    virtual ~DODSFilter();

  /** Use this function to test whether the options passed via argc
      and argv are valid. 

      @memo Check whether the DODSFilter was initialized with valid
      arguments. 
      @return True if the class state is OK, false otherwise. 

      */
    bool OK();

  /** Use this function to check whether the client requested version
      information.  In addition to returning version information about
      the DODS software, the server can also provide version
      information about the dataset itself.

      @memo Should the filter send version information to the client
      program?

      @return TRUE if the -v option was given indicating that the filter
      should send version information back to the client, FALSE
      otherwise. 
      @see DODSFilter::send_version_info
      */
    bool version();

  /** Return the entire constraint expression in a String.  This
      includes both the projection and selection clauses, but not the
      question mark.

      @memo Get the constraint expression. 
      @return A String object that contains the constraint expression. */
    String get_ce();

  /** The ``dataset name'' is the filename or other string that the
      filter program will use to access the data. In some cases this
      will indicate a disk file containing the data.  In others, it
      may represent a database query or some other exotic data
      access method. 

      @memo Get the dataset name. 
      @return A String object that contains the name of the dataset. */
    String get_dataset_name();

  /** To read version information that is specific to a certain
      dataset, override this method with an implementation that does
      what you want. By default, this returns an empty string.

      @memo Get the version information for the dataset.  
      @return A String object that contains the dataset version
      information.  */ 
    virtual String get_dataset_version();

  /** The cache_dir is used to hold the cached .dds and .das files.
      By default, this returns an empty string (store cache files in
      current directory.

      @memo Get the cache directory.
      @return A String object that contains the cache file directory.  */
    virtual String get_cache_dir();

  /** Read the ancillary DAS information and merge it into the input
      DAS object.

      @memo Test if ancillary data must be read.
      @param das A DAS object that will be augmented with the
      ancillary data attributes.
      @return TRUE if an ancillary DAS was found, FALSE otherwise. 
      @see DAS
      */
    bool read_ancillary_das(DAS &das);

  /** Read the ancillary DDS information and merge it into the input
      DDS object. 

      @memo Test if ancillary data must be read.
      @param dds A DDS object that will be augmented with the
      ancillary data properties.
      @return TRUE if an ancillary DDS was found, FALSE otherwise. 
      @see DDS
      */
    bool read_ancillary_dds(DDS &dds);

  /** This message is printed when the filter program is incorrectly
      invoked by the dispatch CGI.  This is an error in the server
      installation or the CGI implementation, so the error message is
      written to stderr instead of stdout.  A server's stderr messages
      show up in the httpd log file. In addition, an error object is
      sent back to the client program telling them that the server is
      broken. 

      @memo Print usage information for a filter program.
      */
    void print_usage();

  /** This function formats and sends to stdout version
      information from the httpd server, the server dispatch scripts,
      the DODS core software, and (optionally) the dataset.

      @memo Send version information back to the client program. */ 
    void send_version_info();

  /** This function formats and prints an ASCII representation of a
      DAS on stdout.  This has the effect of sending the DAS object
      back to the client program.

      @memo Transmit a DAS.
      @param das The DAS object to be sent.
      @return TRUE if the operation succeeded, FALSE otherwise. If the
      send fails, an error object may still be sent. 
      @see DAS */
    bool send_das(DAS &das);

  /** This function formats and prints an ASCII representation of a
      DDS on stdout.  When called by a CGI program, this has the
      effect of sending a DDS object back to the client
      program. Either an entire DDS or a constrained DDS may be sent.

      @memo Transmit a DDS.
      @param dds The DDS to send back to a client.
      @param constrained If this argument is true, evaluate the
      current constraint expression and send the `constrained DDS'
      back to the client. 
      @return TRUE if the operation succeeded, FALSE otherwise. If the
      send fails, an error object may still be sent. 
      @see DDS */
    bool send_dds(DDS &dds, bool constrained = false);

  /** Send the data in the DDS object back to the client
      program.  The data is encoded in XDR format, and enclosed in a
      MIME document which is all sent to stdout.  This has the effect
      of sending it back to the client.

      @memo Transmit data.
      @param dds A DDS object containing the data to be sent.
      @param data_stream A pointer to the XDR sink into which the data
      is to be put for encoding and transmission.
      @return TRUE if the operation succeeded, FALSE otherwise. If the
      send fails, an error object may still be sent. 
      @see DDS
      */
    bool send_data(DDS &dds, FILE *data_stream);
};

#endif // _DODSFilter_h
