
// (c) COPRIGHT URI/MIT 1996
// Please first read the full copyright statement in the file COPYRIGH.  
//
// Authors:
//	jhrg,jimg	James Gallagher (jgallagher@gso.uri.edu)

// $Log: getdap.cc,v $
// Revision 1.2  1996/05/28 15:49:55  jimg
// Removed code that read from the stream after the das/dds parser built the
// internal object (since there was nothing in that stream after the parse
// geturl would always crash).
//
// Revision 1.1  1996/05/22 23:34:21  jimg
// First version. Built to test the new WWW code in the class Connect.
//

static char rcsid[]={"$Id: getdap.cc,v 1.2 1996/05/28 15:49:55 jimg Exp $"};

#include <stdio.h>

#include <GetOpt.h>
#include <String.h>

#include "Connect.h"

void
usage(String name)
{
    cerr << "Usage: " << name 
	 << "[Ada] [v <codes>] [m <num>] <url> [<url> ...]" << endl;
    cerr << "       " << "A: Use Connect's asynchronous mode." << endl;
    cerr << "       " << "d: For each URL, get the DODS DDS." << endl;
    cerr << "       " << "a: For each URL, get the DODS DAS." << endl;
    cerr << "       " << "v: Verbose output." << endl;
    cerr << "       " << "m: Request the same URL <num> times." << endl;
    cerr << "       " << "Without A, use the synchronous mode." << endl;
    cerr << "       " << "Without d or a, print the URL." << endl;
}

bool
read_data(FILE *fp)
{
    char c;
    
    if (!fp) {
	cerr <<"Whoa!!! Null stream pointer." << endl;
	return false;
    }

    // Changed from a loop that used getc() to one that uses fread(). getc()
    // worked fine for transfers of text information, but *not* for binary
    // transfers. fread() will handle both.

    while (fread(&c, 1, 1, fp))
	printf("%c", c);	// stick with stdio 

    return true;
}


int
main(int argc, char * argv[])
{
    FILE * fp;
    GetOpt getopt (argc, argv, "Adav:m:");
    int option_char;
    bool async = false;
    bool get_das = false;
    bool get_dds = false;
    bool verbose = false;
    bool multi = false;
    int times = 1;
    char code[16];
    int vopts = 0;

    while ((option_char = getopt()) != EOF)
	switch (option_char)
	    {
              case 'A': async = true; break;
              case 'd': get_dds = true; break;
	      case 'a': get_das = true; break;
	      case 'v': 
		verbose = true; 
		vopts = strlen(getopt.optarg);
		if (vopts > 15)
		    cerr << "Too many options to verbose! Ignoring..." << endl;
		else
		    strcpy(code, getopt.optarg); 
		break;
	      case 'm': multi = true; times = atoi(getopt.optarg); break;
	      case 'h':
              case '?':
	      default:
		usage(argv[0]); break;
	    }

    if (verbose && vopts < 16 && vopts > 0)
	for (char c = code[vopts-1]; --vopts; c = code[vopts-1])
	    switch (c) {
	      case 'a': WWWTRACE |= SHOW_ANCHOR_TRACE; break;
	      case 'b': WWWTRACE |= SHOW_BIND_TRACE; break;
	      case 'c': WWWTRACE |= SHOW_CACHE_TRACE; break;
	      case 'l': WWWTRACE |= SHOW_SGML_TRACE; break;
	      case 'm': WWWTRACE |= SHOW_MEM_TRACE; break;
	      case 'p': WWWTRACE |= SHOW_PROTOCOL_TRACE; break;
	      case 's': WWWTRACE |= SHOW_STREAM_TRACE; break;
	      case 't': WWWTRACE |= SHOW_THREAD_TRACE; break;
	      case 'u': WWWTRACE |= SHOW_URI_TRACE; break;
	      case 'd': break;
	      default:
		cerr << "Unrecognized verbose option: " << c << endl;
		break;
	    }

    for (int i = getopt.optind; i < argc; ++i) {
	if (verbose)
	    cerr << "Fetching " << argv[i] << ":" << endl;
	
	String name = argv[i];
	Connect url(name);

	if (get_das) {
	    for (int j = 0; j < times; ++j) {
		if (!url.request_das())
		    exit(1);
		if (verbose)
		    cerr << "DAS:" << endl;
		url.das().print();
	    }
	}

	if (get_dds) {
	    for (int j = 0; j < times; ++j) {
		if (!url.request_dds())
		    exit(1);
		if (verbose)
		    cerr << "DDS:" << endl;
		url.dds().print();
	    }
	}

	if (!get_das && !get_dds) {
	    String url_string = argv[i];
	    for (int j = 0; j < times; ++j) {
		if (!url.fetch_url(url_string, async))
		    exit(1);
		FILE *fp = url.output();
		if (!read_data(fp))
		    exit(1);
		fclose(fp);
	    }
	}	    
    }
}
