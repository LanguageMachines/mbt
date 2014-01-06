/*
  $Id$
  $URL$
  Copyright (c) 1998 - 2014
  ILK   - Tilburg University
  CLiPS - University of Antwerp
 
  This file is part of mbt

  mbt is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  mbt is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, see <http://www.gnu.org/licenses/>.

  For questions and suggestions, see:
      http://ilk.uvt.nl/software.html
  or send mail to:
      timbl@uvt.nl
*/

#include <iostream>
#include <ctime>
#include <cstdlib>
#include <stdexcept> 
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
using std::vector;

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
  tagger = TaggerClass::StartTagger( Opts, &ls );
}

MbtAPI::~MbtAPI(){
  RemoveTagger( tagger );
}

bool MbtAPI::isInit() const{
  return tagger && tagger->isInit();
}

string MbtAPI::Tag( const std::string& inp ){
  if ( tagger )
    return tagger->Tag( inp );
  else {
    throw std::runtime_error( "No tagger initialized yet...." );
  }
}

vector<TagResult> MbtAPI::TagLine( const string& inp ){
  if ( tagger )
    return tagger->tagLine( inp );
  else {
    throw std::runtime_error( "No tagger initialized yet...." );
  }
}

string MbtAPI::getResult( const vector<TagResult>& v ) const {
  if ( tagger )
    return tagger->TRtoString( v );
  else {
    throw std::runtime_error( "No tagger initialized yet...." );
  }
}

void gen_usage( const char *progname ){
  cerr << "Usage is : " << progname << " option option ...\n"
       << "\t-s settingsfile\n"
       << "\t-% <percentage> Filter Threshold for ambitag construction (default 5%)\n"
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
       << "\t-V show Version info\n"
       << "\t-X keep intermediate files \n" << endl;
}

bool MbtAPI::GenerateTagger( TimblOpts& Opts ) {
  cur_log->setlevel( Tagger_Log_Level );
  //
  // generate a tagger 
  // Independent, static function so, don't use the internal _tagger here
  //
  string value;
  bool mood;
  if ( Opts.Find( 'h', value, mood ) ||
       Opts.Find( "help", value, mood ) ){
    gen_usage( "mbtg" );
    return true;
  }
  time_t timebefore, timeafter, timediff;
  time(&timebefore);
  int nw = TaggerClass::CreateTagger( Opts );
  if ( nw < 0 ){
    return false;
  }
  time(&timeafter);
  timediff = timeafter - timebefore;
  if ( timediff == 0 )
    timediff = 1;
  cout << endl << "Ready:" << endl
       << "  Time used: " << timediff << endl
       << "  Words/sec: " << nw/(timediff) << endl; 
  return true;
}
  
bool MbtAPI::GenerateTagger(int argc, char *argv[]) {
  // generate a tagger using argv.
  // Independent, static function so, don't use the internal _tagger here
  //
  TimblOpts Opts( argc, argv );
  return GenerateTagger( Opts );
}
  
bool MbtAPI::GenerateTagger( const std::string& arg ) {
  // generate a tagger using a string.
  // Independent, static function so, don't use the internal _tagger here
  //
  TimblOpts Opts( arg );
  return GenerateTagger( Opts );
}
  
void run_usage( char *progname ){
  cerr << "Usage is : " << progname << " option option ... \n"
       << "\t-s settingsfile  ...or:\n\n"
       << "\t-l <lexiconfile>\n"
       << "\t-r <ambitagfile>\n"
       << "\t-k <known words case base>\n"
       << "\t-u <unknown words case base>\n"
       << "\t-D <loglevel> (possible values are 'LogNormal', 'LogDebug', 'LogHeavy' and 'LogExtreme')\n"
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
       << "\t-v cf add confidence to output\n"
       << "\t-V show Version info\n"
       << "\t-L <file with list of frequent words>\n" 
       << endl;
}

bool MbtAPI::RunTagger( int argc, char **argv ){
  // get all the commandline options in an TimblOpts structure
  //
  TimblOpts Opts( argc, argv );
  string value;
  bool mood;
  if ( Opts.Find( 'h', value, mood ) ||
       Opts.Find( "help", value, mood ) ){
    run_usage( argv[0] );
    return true;
  }
  time_t timebefore, timeafter, timediff;
  time(&timebefore);
  TaggerClass *tagger = TaggerClass::StartTagger( Opts );
  if ( !tagger ){
    return false;
  }
  int nw = tagger->Run();
  time(&timeafter);
  timediff = timeafter - timebefore;
  if ( timediff == 0 )
    timediff = 1;
  cerr << "  Time used: " << timediff << endl;
  cerr << "  Words/sec: " << nw/(timediff) << endl;
  delete tagger;
  return true;
}

