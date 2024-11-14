/*
  Copyright (c) 1998 - 2024
  CLST  - Radboud University
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
      https://github.com/LanguageMachines/mbt/issues
  or send mail to:
      lamasoftware (at ) science.ru.nl

*/

#include <algorithm>
#include <fstream>
#include <iostream>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <ctime>
#include <csignal>
#include <cassert>

#include "config.h"
#include "timbl/TimblAPI.h"
#include "ticcutils/Timer.h"
#include "mbt/Pattern.h"
#include "mbt/Sentence.h"
#include "mbt/Logging.h"
#include "mbt/Tagger.h"

#if defined(HAVE_PTHREAD)
#include <pthread.h>
#endif

using namespace TiCC;
using namespace std;
using namespace icu;

LogStream default_log( cerr );
LogStream *cur_log = &default_log;  // fill the externals

LogLevel internal_default_level = LogNormal;
LogLevel Tagger_Log_Level       = internal_default_level;

const double DATA_VERSION=2.0;

namespace Tagger {
  using namespace Hash;
  using namespace Timbl;

  string Version() { return VERSION; }
  string VersionName() { return PACKAGE_STRING; }
  double DataVersion() { return DATA_VERSION; }

  const string UNKSTR   = "UNKNOWN";

  class BeamData;

  TaggerClass::TaggerClass():
    cur_log(new LogStream( cerr ))
  {
    cur_log->setlevel( LogNormal );
    cur_log->setstamp( StampMessage );
    default_cout.setstamp( NoStamp );
    KnownTree = NULL;
    unKnownTree = NULL;
    TimblOptStr = "+vS -FColumns K: -a IGTREE +D U: -a IB1 ";
    FilterThreshold = 5;
    Npax = 5;
    TopNumber = 100;
    DoTop = false;
    DoNpax = true;
    DoTagList = false;
    KeepIntermediateFiles = false;

    KtmplStr = "ddfa";
    UtmplStr = "dFapsss";
    L_option_name = "";
    EosMark = "<utt>";
    Separators = "\t \n";
    initialized = false;
    Beam_Size = 1;
    Beam = NULL;
    MT_lexicon = new map<UnicodeString,UnicodeString>;
    kwordlist = new UnicodeHash();
    uwordlist = new UnicodeHash();
    piped_input = true;
    input_kind = UNTAGGED;
    lexflag = false;
    knowntreeflag = false;
    unknowntreeflag = false;
    knowntemplateflag = false;
    unknowntemplateflag = false;
    knownoutfileflag = false;
    unknownoutfileflag = false;
    reverseflag = false;
    dumpflag = false;
    distance_flag = false;
    distrib_flag = false;
    confidence_flag = false;
    klistflag= false;
    cloned = false;
  }

  TaggerClass::TaggerClass( const TaggerClass& in ):
    cur_log( in.cur_log ),         //!> is a pointer to avoid copies
    KnownTree( in.KnownTree ),
    unKnownTree( in.unKnownTree ),
    initialized( in.initialized ),
    kwordlist( in.kwordlist ),     //!> is a pointer to avoid copies
    uwordlist( in.uwordlist ),     //!> is a pointer to avoid copies
    Beam( 0 ),                     //!> reset pointer
    input_kind( in.input_kind ),
    piped_input( in.piped_input ),
    lexflag( in.lexflag ),
    knowntreeflag( in.knowntreeflag ),
    unknowntreeflag( in.unknowntreeflag ),
    knowntemplateflag( in.knowntemplateflag ),
    unknowntemplateflag( in.unknowntemplateflag ),
    knownoutfileflag( in.knownoutfileflag ),
    unknownoutfileflag( in.unknownoutfileflag ),
    reverseflag( in.reverseflag ),
    dumpflag( in.dumpflag ),
    distance_flag( in.distance_flag ),
    distrib_flag( in.distrib_flag ),
    confidence_flag( in.confidence_flag ),
    klistflag( in.klistflag ),
    Beam_Size( in.Beam_Size ),
    TimblOptStr( in.TimblOptStr ),
    FilterThreshold( in.FilterThreshold ),
    Npax( in.Npax ),
    TopNumber( in.TopNumber ),
    DoTop( in.DoTop ),
    DoNpax( in.DoNpax ),
    DoTagList( in.DoTagList ),
    KeepIntermediateFiles( in.KeepIntermediateFiles ),
    KtmplStr( in.KtmplStr ),
    UtmplStr( in.UtmplStr ),
    L_option_name( in.L_option_name ),
    EosMark( in.EosMark ),
    Separators( in.Separators ),
    Ktemplate( in.Ktemplate ),
    Utemplate( in.Utemplate ),
    MT_lexicon( in.MT_lexicon ),     //!> is a pointer to avoid copies
    UnknownTreeBaseName( in.UnknownTreeBaseName ),
    KnownTreeBaseName( in.KnownTreeBaseName ),
    LexFileBaseName( in.LexFileBaseName ),
    MTLexFileBaseName( in.MTLexFileBaseName ),
    TopNFileBaseName( in.TopNFileBaseName ),
    NpaxFileBaseName( in.NpaxFileBaseName),
    UnknownTreeName( in.UnknownTreeName),
    KnownTreeName( in.KnownTreeName),
    LexFileName( in.LexFileName),
    MTLexFileName( in.MTLexFileName),
    TopNFileName( in.TopNFileName),
    NpaxFileName( in.NpaxFileName),
    TestFileName( in.TestFileName),
    TestFilePath( in.TestFilePath),
    OutputFileName( in.OutputFileName),
    SettingsFileName( in.SettingsFileName),
    SettingsFilePath( in.SettingsFilePath ),
    cloned( true )
  {
  }

  bool TaggerClass::setLog( LogStream& os ){
    if ( !cloned ){
      delete cur_log;
    }
    cur_log = new LogStream( &os );
    cur_log->set_message( "mbt-" );
    return true;
  }

  UnicodeString TaggerClass::set_eos_mark( const UnicodeString& eos ){
    UnicodeString tmp = EosMark;
    EosMark = eos;
    return tmp;
  }

  const UnicodeString& indexlex( const unsigned int index,
				 UnicodeHash& aLex){
    return aLex.reverse_lookup( index );
  }

  TaggerClass::~TaggerClass(){
    DBG << "classify total took: " << timer1 << endl;
    DBG << "classify known took: " << timer2 << endl;
    DBG << "classify unknown took: " << timer3 << endl;
    if ( !cloned ){
      delete KnownTree;
      delete unKnownTree;
      delete MT_lexicon;
      delete kwordlist;
      delete uwordlist;
      delete cur_log;
    }
    delete Beam;
  }

  void get_weightsfile_name( string& opts, string& name ){
    name = "";
    string::size_type pos = opts.find( "-w" );
    if ( pos != string::npos ){
      string::size_type b_pos = opts.find_first_not_of( " \t\r", pos+2 );
      string::size_type e_pos = opts.find_first_of( " \t\r", b_pos );
      string tmp = opts.substr( b_pos, e_pos - b_pos );
      Weighting W;
      if ( !string_to( tmp, W ) ){
	// no weight, so assume a filename...
	name = tmp;
	opts.erase( pos, e_pos - pos );
      }
    }
  }


  void splits( const string& opts, string& common,
	       string& known, string& unknown ){
    xDBG << "splits, opts = " << opts << endl;
    known = "";
    unknown = "";
    common = " -FColumns ";
    bool done_u = false, done_k = false;
    string::size_type k_pos = opts.find( "K:" );
    string::size_type u_pos = opts.find( "U:" );
    xDBG << "K pos " << k_pos << endl;
    xDBG << "U pos " << u_pos << endl;
    if ( k_pos != string::npos ){
      if ( k_pos < u_pos ){
	common += opts.substr( 0, k_pos );
	known = opts.substr( k_pos+2, u_pos - k_pos - 2 );
      }
      else {
	known = opts.substr( k_pos+2 );
      }
      done_k = true;
    }
    if ( u_pos != string::npos ){
      if ( u_pos < k_pos ){
	common += opts.substr( 0, u_pos );
	unknown = opts.substr( u_pos+2, k_pos - u_pos - 2 );
      }
      else {
	unknown = opts.substr( u_pos+2 );
      }
      done_u = true;
    }
    if ( !done_u ){
      if ( !done_k ) {
	known = opts;
	unknown = opts;
      }
      else if ( k_pos != string::npos ){
	unknown = opts.substr( 0, k_pos );
      }
      else {
	unknown = known;
      }
    }
    else if ( !done_k ) {
      if ( u_pos != string::npos ){
	known = opts.substr( 0, u_pos );
      }
      else {
	known = unknown;
      }
    }
    xDBG << "resultaat splits, common = " << common << endl;
    xDBG << "resultaat splits, K = " << known << endl;
    xDBG << "resultaat splits, U = " << unknown << endl;
  }

  //**** stuff to process commandline options *********************************
  // used to convert relative paths to absolute paths

  /**
   * If you do 'mbt -s some-path/xxx.settingsfile' Timbl can not find the
   * tree files.
   *
   * Because the settings file can contain relative paths for files these
   * paths are converted to absolute paths.
   * The relative paths are taken relative to the pos ition of the settings
   * file, so the path of the settings file is prefixed to the realtive path.
   *
   * Paths that do not begin with '/' and do not have as second character ':'
   *      (C: or X: in windows cygwin) are considered to be relative paths
   */

  string prefixWithAbsolutePath( const string& fileName,
				 const string& prefix ) {
    //    default_log << fileName << endl;
    string result = fileName;
    if ( ( fileName.size() > 1 )
	 && ( fileName[0] != '/' && fileName[1] != ':' )
	 && !( fileName[0]== '.' && fileName[1] == '/' ) ){
      result = prefix + fileName;
    }
    //    default_log << result << endl;
    return result;
  }

  bool TaggerClass::set_default_filenames( ){
    //
    // and use them to setup the defaults...
    if ( !KtmplStr.empty() ) {
      if ( Ktemplate.set( KtmplStr ) ){
	knowntemplateflag = true;
      }
      else {
	cerr << "couldn't set Known Template from '" << KtmplStr
	     << "'" << endl;
	return false;
      }
    }
    if ( !UtmplStr.empty() ) {
      if ( Utemplate.set( UtmplStr ) ){
	unknowntemplateflag = true;
      }
      else {
	cerr << "couldn't set Unknown Template from '" << UtmplStr
	     << "'" << endl;
	return false;
      }
    }

    LexFileBaseName = TestFileName;
    LexFileBaseName += ".lex";
    LexFileName = prefixWithAbsolutePath( LexFileBaseName, SettingsFilePath );
    string affix = std::to_string( FilterThreshold );
    affix = TiCC::pad( affix, 2, '0' );
    affix = "." + affix;
    if ( !knownoutfileflag ){
      K_option_name = TestFileName + ".known.inst." + KtmplStr;
      K_option_name = prefixWithAbsolutePath( K_option_name,
					      SettingsFilePath );
    }
    if ( !knowntreeflag ){
      KnownTreeBaseName = TestFileName + ".known." + KtmplStr;
      KnownTreeName = prefixWithAbsolutePath( KnownTreeBaseName,
					      SettingsFilePath );
    }
    if ( !unknownoutfileflag ){
      U_option_name = TestFileName + ".unknown.inst." + UtmplStr;
      U_option_name = prefixWithAbsolutePath( U_option_name,
					      SettingsFilePath );
    }
    if ( !unknowntreeflag ){
      UnknownTreeBaseName = TestFileName + ".unknown." + UtmplStr;
      UnknownTreeName = prefixWithAbsolutePath( UnknownTreeBaseName,
						SettingsFilePath );
    }
    if ( lexflag ){
      MTLexFileBaseName = l_option_name;
      MTLexFileName = MTLexFileBaseName;
    }
    else {
      MTLexFileBaseName = TestFileName + ".lex.ambi" + affix;
      MTLexFileName = prefixWithAbsolutePath( MTLexFileBaseName,
					      SettingsFilePath );
    }
    if ( !L_option_name.empty() ){
      TopNFileBaseName = L_option_name;
      TopNFileName = TopNFileBaseName;
    }
    else {
      affix = std::to_string( TopNumber );
      affix = ".top" + affix;
      TopNFileBaseName = TestFileName + affix;
      TopNFileName = prefixWithAbsolutePath( TopNFileBaseName,
					     SettingsFilePath );
    }
    affix = std::to_string( Npax );
    affix = "." + affix + "paxes";
    NpaxFileBaseName = TestFileName + affix;
    NpaxFileName = prefixWithAbsolutePath( NpaxFileBaseName,
					   SettingsFilePath );
    return true;
  }

  TaggerClass *TaggerClass::clone() const {
    TaggerClass *ta = new TaggerClass( *this );
    ta->Beam = NULL; // own Beaming data
    ta->cloned = true;
    return ta;
  }

}
