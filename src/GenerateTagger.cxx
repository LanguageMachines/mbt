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
#include <vector>
#include <map>
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
#include "mbt/TagLex.h"
#include "mbt/Pattern.h"
#include "mbt/Sentence.h"
#include "mbt/Logging.h"
#include "mbt/Tagger.h"

#if defined(HAVE_PTHREAD)
#include <pthread.h>
#endif

namespace Tagger {
  using namespace std;
  using namespace icu;
  using namespace Hash;
  using namespace Timbl;

  const string UNKSTR   = "UNKNOWN";



  void TaggerClass::ProcessTags( TagInfo *TI ){
    TI->Prune( FilterThreshold );
    TI->CreateStringRepr();
  }

  bool split_special( const UnicodeString& buffer,
		      UnicodeString& word,
		      UnicodeString& tag ){
    vector<UnicodeString> subs = TiCC::split( buffer );
    if ( subs.size() > 1 ){
      word = subs.front();
      tag = subs.back();
      return true;
    }
    return false;
  }

  template <typename T1, typename T2>
  struct more_second {
    typedef pair<T1, T2> type;
    bool operator ()(type const& a, type const& b) const {
      return a.second > b.second;
    }
  };

  bool TaggerClass::create_lexicons(){
    TagLex TaggedLexicon;
    ifstream lex_file;
    ofstream out_file;
    string Buffer;
    string filename = TestFilePath + TestFileName;
    if ( filename != "" ){
      if ( ( lex_file.open( filename, ios::in ),
	     !lex_file.good() ) ){
	cerr << "couldn't open tagged lexicon file `"
	     << filename << "'" << endl;
	return false;
      }
      COUT << "Constructing a tagger from: " << filename << endl;
    }
    else {
      cerr << "couldn't open inputfile " << filename << endl;
      return false;
    }
    map<UnicodeString,unsigned int> TagList;
    UnicodeString buffer;
    while ( TiCC::getline( lex_file, buffer ) ){
      UnicodeString word, tag;
      if ( split_special( buffer, word, tag ) ){
	TaggedLexicon.Store( word, tag );
	TagList[tag]++;
      }
    }
    vector<TagInfo *>TagVect = TaggedLexicon.CreateSortedVector();
    if ( (out_file.open( LexFileName, ios::out ),
	  out_file.good() ) ){
      COUT << "  Creating lexicon: "  << LexFileName << " of "
	   << TagVect.size() << " entries." << endl;
      for ( auto const *tv : TagVect ){
	out_file << tv->Freq() << " " << tv->Word
		 << " " << tv->DisplayTagFreqs() << endl;
      }
      out_file.close();
    }
    else {
      cerr << "couldn't create lexiconfile " << LexFileName << endl;
      return false;
    }
    for ( auto const& tv : TagVect ){
      ProcessTags( tv );
    }
    if ( (out_file.open( MTLexFileName, ios::out ),
	  out_file.good() ) ){
      COUT << "  Creating ambitag lexicon: "  << MTLexFileName << endl;
      for ( const auto& tv : TagVect ){
	out_file << tv->Word << " " << tv->stringRep() << endl;
	MT_lexicon->insert( make_pair(tv->Word,
				      tv->stringRep() ) );
      }
      out_file.close();
    }
    else {
      cerr << "couldn't create file: " << MTLexFileName << endl;
      return false;
    }
    if ( (out_file.open( TopNFileName, ios::out ),
	  out_file.good() ) ){
      COUT << "  Creating list of most frequent words: "  << TopNFileName << endl;
      int k = 0;
      for ( auto const& tv : TagVect ){
	if ( ++k > TopNumber ){
	  break;
	}
	out_file << tv->Word << endl;
	kwordlist->hash( tv->Word );
      }
      out_file.close();
    }
    else {
      cerr << "couldn't open file: " << TopNFileName << endl;
      return false;
    }
    if ( DoNpax ){
      if ( (out_file.open( NpaxFileName, ios::out ),
	    out_file.good() ) ){
	for ( const auto& tv : TagVect ){
	  if ( tv->Freq() > Npax ){
	    continue;
	  }
	  out_file << tv->Word << endl;
	  uwordlist->hash( tv->Word );
	}
	out_file.close();
      }
      else {
	cerr << "couldn't open file: " << NpaxFileName << endl;
	return false;
      }
    }
    if ( DoTagList ){
      vector<pair<UnicodeString,unsigned int>> si_vec( TagList.begin(), TagList.end() );
      sort(si_vec.begin(), si_vec.end(), more_second<UnicodeString, unsigned int>());
      ofstream os( TagListName );
      if ( os ){
	for ( const auto& it: si_vec ){
	  os << it.first << "\t" << it.second << endl;
	}
      }
      else {
	cerr << "couldn't open outputfile: " << TagListName << endl;
	return false;
      }
    }
    return true;
  }

  bool TaggerClass::InitLearning( ){
    // if not supplied on command line, make a default
    // name for both output files (concatenation of datafile
    // and pattern string)
    //
    if ( !create_lexicons() ){
      return false;
    }
    if ( TimblOptStr.empty() ){
      Timbl_Options = "-a IB1 -G0";
    }
    else {
      Timbl_Options = TimblOptStr;
    }
    splits( Timbl_Options, commonstr, knownstr, unknownstr );
    get_weightsfile_name( knownstr, kwf );
    get_weightsfile_name( unknownstr, uwf );
    return true;
  }

  int TaggerClass::makedataset( istream& infile, bool do_known ){
    int no_words=0;
    ofstream outfile;
    MatchAction Action;
    vector<int> TestPat;
    if ( do_known ){
      outfile.open( K_option_name, ios::trunc | ios::out );
      Action = MakeKnown;
    }
    else {
      outfile.open( U_option_name, ios::trunc | ios::out );
      Action = MakeUnknown;
    }
    // loop as long as you get sentences
    //
    int HartBeat = 0;
    size_t line_cnt = 0;
    sentence mySentence( Ktemplate, Utemplate );
    while ( mySentence.read( infile, input_kind, EosMark, Separators, line_cnt ) ){
      if ( mySentence.size() == 0 ){
	continue;
      }
      // cerr << mySentence << endl;
      if ( mySentence.getword(0) == EosMark ){
	// only possible for ENRICHED!
	continue;
      }
      if ( ++HartBeat % 100 == 0 ) {
	if ( do_known ){
	  COUT << "+";
	  default_cout.flush();
	}
	else {
	  COUT << "-";
	  default_cout.flush();
	}
      }
      if ( mySentence.init_windowing( *MT_lexicon, TheLex ) ) {
	// we initialize the windowing procedure, this entails lexical lookup
	// of the words in the dictionary and the values
	// of the features are stored in the testpattern
	int swcn = 0;
	vector<int> dummy(1,0);
	while( mySentence.nextpat( Action, TestPat,
				   *kwordlist, TheLex,
				   swcn, dummy ) ){
	  bool skip = false;
	  if ( DoNpax && !do_known ){
	    if ( (uwordlist->lookup( mySentence.getword(swcn))) == 0 ){
	      skip = true;
	    }
	  }
	  if ( !skip ){
	    for ( const auto& pat: TestPat ){
	      outfile << indexlex( pat, TheLex ) << " ";
	    }
	  }
	  int thisTagCode = -1;
#pragma omp critical (hasher)
	  {
	    thisTagCode = TheLex.hash( mySentence.gettag(swcn) );
	  }
	  if ( !skip ){
	    for ( auto const& it : mySentence.getEnrichments(swcn) ){
	      outfile << it << " ";
	    }
	    outfile << mySentence.gettag( swcn ) << '\n';
	  }
	  mySentence.assign_tag(thisTagCode, swcn );
	  ++swcn;
	  ++no_words;
	}
      }
    }
//      default_cout << "Output written to ";
//      if ( do_known )
//        default_cout << K_option_name << endl;
//      else
//        default_cout << U_option_name << endl;
    return no_words;
  }

  int TaggerClass::CreateKnown(){
    int nwords = 0;
    if ( knowntemplateflag ){
      COUT << "  Create known words case base,"
	   << "   Timbl options: '" << knownstr + commonstr << "'" << endl;
      TimblAPI *Ktree = new TimblAPI( knownstr + commonstr );
      if ( !Ktree->Valid() ){
	cerr << "unable to create Timbl(" << knownstr + commonstr
	     << ") for Known words." << endl;
	return -1; // signal a failure
      }
      COUT << "    Algorithm = " << to_string(Ktree->Algo()) << endl;
      if ( !piped_input ){
	string inname = TestFilePath + TestFileName;
	ifstream infile( inname, ios::in );
	if ( infile.bad() ){
	  cerr << "Cannot read from " << inname << endl;
	  return 0;
	}
	COUT << "    Processing data from the file " << inname << "...";
	default_cout.flush();
	nwords = makedataset( infile, true );
      }
      else {
	COUT << "Processing data from the standard input" << endl;
	nwords = makedataset( cin, true );
      }
      COUT << endl << "    Creating case base: " << KnownTreeName << endl;
      Ktree->Learn( K_option_name );
      //      UKtree->ShowSettings( default_cout );
      Ktree->WriteInstanceBase( KnownTreeName );
      if ( !kwf.empty() ){
	Ktree->SaveWeights( kwf );
      }
      delete Ktree;
      if ( !KeepIntermediateFiles ){
	remove(K_option_name.c_str());
	COUT << "    Deleted intermediate file: " << K_option_name << endl;
      }
    }
    return nwords;
  }

  int TaggerClass::CreateUnknown(){
    int nwords = 0;
    if ( unknowntemplateflag ){
      COUT << "  Create unknown words case base,"
	   << " Timbl options: '" << unknownstr + commonstr << "'" << endl;
      TimblAPI *UKtree = new TimblAPI( unknownstr + commonstr );
      if ( !UKtree->Valid() ){
	cerr << "unable to create Timbl(" << unknownstr + commonstr
	     << ") for Unknown words." << endl;
	return -1; // signal a failure
      }
      COUT << "    Algorithm = " << to_string(UKtree->Algo()) << endl;
      if ( !piped_input ){
	string inname = TestFilePath + TestFileName;
	ifstream infile( inname, ios::in );
	if( infile.bad() ){
	  cerr << "Cannot read from " << inname << endl;
	  return 0;
	}
	// COUT << "    Processing data from the file " << inname << "...";
	// default_cout.flush();
	nwords = makedataset( infile, false );
      }
      else {
	COUT << "Processing data from the standard input" << endl;
	nwords = makedataset( cin, false );
      }
      COUT << endl << "    Creating case base: " << UnknownTreeName << endl;
      UKtree->Learn( U_option_name );
      //      UKtree->ShowSettings( COUT );
      UKtree->WriteInstanceBase( UnknownTreeName );
      if ( !uwf.empty() ){
	UKtree->SaveWeights( uwf );
      }
      delete UKtree;
      if ( !KeepIntermediateFiles ){
	remove(U_option_name.c_str());
	COUT << "    Deleted intermediate file: " << U_option_name << endl;
      }
    }
    return nwords;
  }

  bool TaggerClass::CreateSettingsFile(){
    if ( SettingsFileName.empty() ) {
      SettingsFileName = TestFileName + ".settings";
    }
    ofstream out_file;
    if ( ( out_file.open( SettingsFileName, ios::out ),
	   !out_file.good() ) ){
      cerr << "couldn't create Settings-File `"
	   << SettingsFileName << "'" << endl;
      return false;
    }
    else {
      if ( input_kind == ENRICHED ){
	out_file << "ENRICHED" << endl;
      }
      out_file << "DATA_VERSION " << DataVersion() << endl;
      out_file << "e " << EosMark << endl;
      out_file << "l " << MTLexFileBaseName << endl;
      out_file << "k " << KnownTreeBaseName << endl;
      out_file << "u " << UnknownTreeBaseName << endl;
      out_file << "p " << KtmplStr << endl;
      out_file << "P " << UtmplStr << endl;
      out_file << "O " << Timbl_Options << endl;
      out_file << "L " << TopNFileBaseName << endl;
      out_file.close();
      COUT << endl << "  Created settings file '"
			 << SettingsFileName << "'" << endl;
    }
    return true;
  }

  //**** stuff to process commandline options *****************************

  const string mbt_create_short = "hV%:d:e:E:k:K:l:L:m:M:n:o:O:p:P:r:s:t:T:u:U:XD:";
  const string mbt_create_long = "version,tabbed";

  bool TaggerClass::parse_create_args( TiCC::CL_Options& opts ){
    string value;
    if ( opts.extract( '%', value ) ){
      FilterThreshold = TiCC::stringTo<int>( value );
    }
    if ( opts.extract( 'd', value ) ){
      dumpflag=true;
      cout << "  Dumpflag ON" << endl;
    }
    if ( opts.extract( 'e', value ) ){
      EosMark = TiCC::UnicodeFromUTF8(value);
      cout << "  Sentence delimiter set to '" << EosMark << "'" << endl;
    }
    if ( opts.extract( "tabbed" ) ){
      Separators = "\t";
    }
    if ( opts.extract( 'K', value ) ){
      K_option_name = value;
      knownoutfileflag = true; // there is a knownoutfile specified
    }
    if ( opts.extract( 'k', value ) ){
      KnownTreeName = value;
      knowntreeflag = true; // there is a knowntreefile specified
    }
    if ( opts.extract( 'l', value ) ){
      l_option_name = value;
      lexflag = true; // there is a lexicon specified
    }
    if ( opts.extract( 'L', value ) ){
      L_option_name = value;
      klistflag = true;
    }
    if ( opts.extract( 'M', value ) ){
      TopNumber = TiCC::stringTo<int>(value);
    }
    if ( opts.extract( 'n', value ) ){
      Npax = TiCC::stringTo<int>(value);
      if ( Npax == 0 )
	DoNpax = false;
    }
    if ( opts.extract( 'O', value ) ){
      TimblOptStr = value; // Option string for Timbl
    }
    if ( opts.extract( 'p', value ) ){
      KtmplStr = value;  // windowing pattern for known words
    }
    if ( opts.extract( 'P', value ) ){
      UtmplStr = value;  // windowing pattern for unknown words
    }
    if ( opts.extract( 'r', value ) ){
      r_option_name = value;
      reverseflag = true;
    }
    if ( opts.extract( 's', value ) ){
      // if a settingsfile option has been specified, use that name
      SettingsFileName = value;
      // extract the absolute path to the settingsfile
      string::size_type lastSlash = SettingsFileName.rfind('/');
      if ( lastSlash != string::npos ){
	SettingsFilePath = SettingsFileName.substr( 0, lastSlash+1 );
      }
      else {
	SettingsFilePath = "";
      }
    }
    if ( opts.extract( 't', value ) ){
      TagListName = value;
      DoTagList = true; // we want a TagList
    }
    if ( opts.extract( 'E', value ) ){
      TestFileName = value;
      // extract the absolute path to the testfile
      string::size_type lastSlash = TestFileName.rfind('/');
      if ( lastSlash != string::npos ){
	TestFilePath = TestFileName.substr( 0, lastSlash+1 );
	TestFileName = TestFileName.substr( lastSlash+1 );
      }
      else {
	TestFilePath = "";
      }
      piped_input = false;
      input_kind = ENRICHED; // an enriched tagged test file specified
    }
    if ( opts.extract( 'T', value ) ){
      TestFileName = value;
      // extract the absolute path to the testfile
      string::size_type lastSlash = TestFileName.rfind('/');
      if ( lastSlash != string::npos ){
	TestFilePath = TestFileName.substr( 0, lastSlash+1 );
	TestFileName = TestFileName.substr( lastSlash+1 );
      }
      else {
	TestFilePath = "";
      }
      piped_input = false;
      input_kind = TAGGED; // there is a tagged test file specified
    }
    if ( opts.extract( 'u', value ) ){
      UnknownTreeName = value;
      unknowntreeflag = true; // there is a unknowntreefile file specified
    }
    if ( opts.extract( 'U', value ) ){
      U_option_name = value;
      unknownoutfileflag = true; // there is a unknownoutfile specified
    }
    if ( opts.extract( 'X', value ) ){
      KeepIntermediateFiles = true;
    }
    if ( opts.extract( 'D', value ) ){
      if ( value == "LogSilent" ){
	cur_log->set_level( LogSilent );
	default_cout.set_level( LogSilent );
      }
      else if ( value == "LogNormal" ){
	cur_log->set_level( LogNormal );
	default_cout.set_level( LogNormal );
      }
      else if ( value == "LogDebug" ){
	cur_log->set_level( LogDebug );
	default_cout.set_level( LogDebug );
      }
      else if ( value == "LogHeavy" ){
	cur_log->set_level( LogHeavy );
	default_cout.set_level( LogHeavy );
      }
      else if ( value == "LogExtreme" ){
	cur_log->set_level( LogExtreme );
	default_cout.set_level( LogExtreme );
      }
      else {
	cerr << "Unknown Debug mode! (-D " << value << ")" << endl;
      }
    }
    if ( TestFileName.empty() ){
      cerr << "Missing required options. see 'mbt -h'" << endl;
      return false;
    }
    return true;
  }

  void TaggerClass::gen_usage( const string& progname ){
    cerr << "Usage is : " << progname << " option option ...\n"
	 << "\t-s settingsfile\n"
	 << "\t-% <percentage> Filter Threshold for ambitag construction (default 5%)\n"
	 << "\t-E <enriched tagged training corpus file> \n"
	 << "\t-T <tagged training corpus file> \n"
	 << "\t--tabbed ONLY use tabs as separator in TAGGED input. (default is all whitespace)\n"
	 << "\t-O\"Timbl options\" (Note: NO SPACE between O and \"!!!)\n"
	 << "\t   <options>   options to use for both Known and Unknown Words Case Base\n"
	 << "\t   K: <options>   options to use for Known Words Case Base\n"
	 << "\t   U: <options>   options to use for Unknown Words Case Base\n"
	 << "\t   valid Timl options: a d k m q v w x -\n"
	 << "\t-p pattern for known words (default ddfa)\n"
	 << "\t-P pattern for unknown words (default dFapsss)\n"
	 << "\t-D <loglevel> (possible values are 'LogSilent', 'LogNormal', 'LogDebug', 'LogHeavy' and 'LogExtreme')\n"
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

  int TaggerClass::CreateTagger( TiCC::CL_Options& opts ){
    if ( opts.is_present( 'h' ) ||
	 opts.is_present( "help" ) ){
      gen_usage( "mbtg" );
      return 0;
    }
    //
    // present yourself to the user
    //
    if ( opts.is_present( 'V' ) ||
	 opts.is_present( "version" ) ){
      TaggerClass::manifest( "mbtg" );
      return 0;
    }
    TaggerClass tagger;
    if ( !tagger.parse_create_args( opts ) ){
      return -1;
    }
    tagger.set_default_filenames();
    tagger.InitLearning();
    // process the test material
    // and do the timing
    int kwords = 0;
    int uwords = 0;
#pragma omp parallel sections
    {
#pragma omp section
      {
	kwords = tagger.CreateKnown();
      }
#pragma omp section
      {
	uwords = tagger.CreateUnknown();
      }
    }
    if ( kwords < 0 || uwords < 0 ){
      cerr << "Generating a tagger failed" << endl;
      return -1;
    }
    COUT << "      ready: " << kwords << " words processed."
	 << endl;
    if ( !tagger.CreateSettingsFile() ){
      return -1;
    }
    return kwords + uwords;
  }

  int TaggerClass::CreateTagger( const string& opt_string ){
    TiCC::CL_Options opts;
    opts.allow_args( mbt_create_short, mbt_create_long );
    try {
      opts.parse_args( opt_string );
    }
    catch ( exception& e ){
      cerr << e.what() << endl;
      return -1;
    }
    return CreateTagger( opts );
  }

  int TaggerClass::CreateTagger( int argc, char* argv[] ){
    TiCC::CL_Options opts;
    opts.allow_args( mbt_create_short, mbt_create_long );
    try {
      opts.parse_args( argc, argv );
    }
    catch ( exception& e ){
      cerr << e.what() << endl;
      return -1;
    }
    return CreateTagger( opts );
  }

}
