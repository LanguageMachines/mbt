/*
  $Id$
  $URL$
  Copyright (c) 1998 - 2010
  ILK  -  Tilburg University
  CNTS -  University of Antwerp
 
  This file is part of Mbt3

  Mbt3 is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mbt3 is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, see <http://www.gnu.org/licenses/>.

  For questions and suggestions, see:
      http://ilk.uvt.nl/software.html
  or send mail to:
      Timbl@uvt.nl
*/

#include <iostream>
#include <ctime>
#include <cstdlib>
#include "timbl/TimblAPI.h"
#include "config.h"
#include "mbt/Logging.h"
#include "mbt/Tagger.h"
#include "mbt/MbtAPI.h"

using std::ostream;
using std::cout;
using std::cerr;
using std::endl;
using std::string;

MbtAPI::MbtAPI( const std::string& opts ){
  // get all the commandline options in an TimblOpts structure
  //
  TimblOpts Opts( opts );
  tagger = TaggerClass::StartTagger( Opts );
}

MbtAPI::MbtAPI( const std::string& opts, LogStream& ls ){
  // get all the commandline options in an TimblOpts structure
  //
  TimblOpts Opts( opts );
  tagger = TaggerClass::StartTagger( Opts );
  tagger->setLog( ls );
}

MbtAPI::~MbtAPI(){
  RemoveTagger( tagger );
}

string MbtAPI::Tag( const std::string& inp ){
  if ( tagger )
    return tagger->Tag( inp );
  else {
    cerr << "No tagger initialized yet...." << endl;
    exit(EXIT_FAILURE);
  }
}

void gen_usage( char *progname ){
  cerr << "Usage is : " << progname << " option option ...\n"
       << "\t-s settingsfile\n"
       << "\t-% <percentage> Filter Treshold for ambitag construction (default 5%)\n"
       << "\t-E <enriched tagged training corpus file> \n"
       << "\t-T <tagged training corpus file> \n"
       << "\t-O\"Timbl options\" (Note: NO SPACE between O and \"!!!)\n"
       << "\t   <options>   options to use for both Known and Unknown Words Case Base\n"
       << "\t   K: <options>   options to use for Known Words Case Base\n"
       << "\t   U: <options>   options to use for Unknown Words Case Base\n"
       << "\t   valid Timl options: a d k m q v w x -\n"
       << "\t-p pattern for known words (default ddfa)\n"
       << "\t-P pattern for unknown words (default dFapsss)\n"
       << "\t-e <sentence delimiter> (default '<utt>')\n"
       << "\t-L <file with list of frequent words>\n"
       << "\t-M <number of most frequent words> (default 100)\n"
       << "\t-n <arity of Npaxes> (default 5)\n"
       << "\t-l <lexiconfile>\n"
       << "\t-r <ambitagfile>\n"
       << "\t-k <known words case base>\n"
       << "\t-u <unknown words case base>\n"
       << "\t-K <known words instances file>\n"
       << "\t-U <unknown words instances file>\n"
       << "\t-X keep intermediate files \n" << endl;
}

bool MbtAPI::GenerateTagger(int argc, char *argv[]) {
  cur_log->setlevel( Tagger_Log_Level );
  //
  // generate a tagger using argv.
  // Independent, static function so, don't use the internal _tagger here
  // 
  // present yourself to the user
  //
  cerr << "Mbtg " << VERSION << " (c) ILK and CLiPS 1998 - 2010." << endl
       << "Memory Based Tagger Generator" << endl
       << "Induction of Linguistic Knowledge Research Group,"
       << "Tilburg University" << endl
       << "CLiPS Computational Linguistics Group, University of Antwerp"
       << endl << endl
       << "Based on Timbl version " << TimblAPI::VersionInfo() 
       << endl << endl;
  // test for the right number of arguments
  //
  if(argc<3) {
    gen_usage( argv[0] );
    return false;
  }
  else {
    TimblOpts Opts( argc, argv );
    cerr << "options " << Opts << endl;
    time_t timebefore, timeafter, timediff;
    time(&timebefore);
    int nw = TaggerClass::CreateTagger( Opts );   
    time(&timeafter);
    timediff = timeafter - timebefore;
    if ( timediff == 0 )
      timediff = 1;
    cout << endl << "Ready:" << endl
	 << "  Time used: " << timediff << endl
	 << "  Words/sec: " << nw/(timediff) << endl; 
    return true;
  }
}
  
void run_usage( char *progname ){
  cerr << "Usage is : " << progname << " option option ... \n"
       << "\t-s settingsfile  ...or:\n\n"
       << "\t-l <lexiconfile>\n"
       << "\t-r <ambitagfile>\n"
       << "\t-k <known words case base>\n"
       << "\t-u <unknown words case base>\n"
       << "\t-e <sentence delimiter> (default '<utt>')\n"
       << "\t-E <enriched tagged testfile>\n "
       << "\t-t <testfile> | -T <tagged testfile> "
       << "(default is untagged stdin)\n" 
       << "\t-o <outputfile> (default stdout)\n"
       << "\t-O\"Timbl options\" (Note: NO SPACE between O and \"!!!)\n"
       << "\t  <options>   options to use for Both Known and Unknown Words Case Base\n"
       << "\t  K: <options>   options to use for Known Words Case Base\n"
       << "\t  U: <options>   options to use for Unknown Words Case Base\n"
       << "\t  valid Timbl options: a d k m q v w x -\n"
       << "\t-B <beamsize for search> (default = 1) \n"
       << "\t-v di add distance to output\n"
       << "\t-v db add distribution to output\n"
       << "\t-L <file with list of frequent words>\n" 
       << endl;
}

bool MbtAPI::RunTagger( int argc, char **argv ){
  // present yourself to the user
  //
  cerr << "Mbt " << VERSION << " (c) ILK and CLiPS 1998 - 2010." << endl
       << "Memory Based Tagger " << endl
       << "Tilburg University" << endl
       << "CLiPS Computational Linguistics Group, University of Antwerp"
       << endl << endl
       << "Based on Timbl version " << TimblAPI::VersionInfo() 
       << endl << endl;
  // test for the right number of arguments
  //
  if(argc<3) {
    run_usage( argv[0] );
    return false;
  }
  else {
    // get all the commandline options in an TimblOpts structure
    //
    TimblOpts Opts( argc, argv );
    time_t timebefore, timeafter, timediff;
    time(&timebefore);
    TaggerClass *tagger = TaggerClass::StartTagger( Opts );
    int nw = tagger->Run();
    time(&timeafter);
    timediff = timeafter - timebefore;
    if ( timediff == 0 )
      timediff = 1;
    cerr << "  Time used: " << timediff << endl;
    cerr << "  Words/sec: " << nw/(timediff) << endl;
    return true;
  }
}

