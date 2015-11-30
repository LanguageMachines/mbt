/*
  $Id$
  $URL$
  Copyright (c) 1998 - 2015
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
  tagger = TaggerClass::StartTagger( opts );
}

MbtAPI::MbtAPI( const std::string& opts, LogStream& ls ){
  tagger = TaggerClass::StartTagger( opts, &ls );
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

string MbtAPI::set_eos_mark( const std::string& eos ){
  if ( tagger )
    return tagger->set_eos_mark( eos );
  else {
    throw std::runtime_error( "No tagger initialized yet...." );
  }
}

bool MbtAPI::GenerateTagger(int argc, char *argv[]) {
  // generate a tagger using argv.
  // Independent, static function so, don't use the internal _tagger here
  //
  cur_log->setlevel( Tagger_Log_Level );
  time_t timebefore, timeafter, timediff;
  time(&timebefore);
  int nw = TaggerClass::CreateTagger( argc, argv );
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

bool MbtAPI::GenerateTagger( const std::string& arg ) {
  // generate a tagger using a string.
  // Independent, static function so, don't use the internal _tagger here
  //
  cur_log->setlevel( Tagger_Log_Level );
  time_t timebefore, timeafter, timediff;
  time(&timebefore);
  int nw = TaggerClass::CreateTagger( arg );
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


bool MbtAPI::RunTagger( int argc, char **argv ){
  time_t timebefore, timeafter, timediff;
  time(&timebefore);
  TaggerClass *tagger = TaggerClass::StartTagger( argc, argv );
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
