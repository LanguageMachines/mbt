/*
  $Id: Tagger.cxx 18614 2015-09-10 15:26:12Z sloot $
  $URL: https://ilk.uvt.nl/svn/trunk/sources/Mbt3/src/Tagger.cxx $

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
#include "mbt/TagLex.h"
#include "mbt/Pattern.h"
#include "mbt/Sentence.h"
#include "mbt/Logging.h"
#include "mbt/Tagger.h"

#if defined(HAVE_PTHREAD)
#include <pthread.h>
#endif

using namespace TiCC;

namespace Tagger {
  using namespace std;
  using namespace Hash;
  using namespace Timbl;

  const string UNKSTR   = "UNKNOWN";



  void TaggerClass::ProcessTags( TagInfo *TI ){
    TI->Prune( FilterThreshold );
    TI->CreateStringRepr();
  }

  bool split_special( const string& Buffer, string& Word, string& Tag ){
    vector<string> subs;
    size_t len = split( Buffer, subs );
    if ( len > 1 ){
      Word = subs.front();
      Tag = subs.back();
      return true;
    }
    return false;
  }

  void TaggerClass::create_lexicons(){
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
	exit(EXIT_FAILURE);
      }
      cout << "Constructing a tagger from: " << filename << endl;
    }
    else {
      cerr << "couldn't open inputfile " << filename << endl;
      exit(EXIT_FAILURE);
    }
    string Word, Tag;
    while ( getline( lex_file, Buffer ) ){
      if ( split_special( Buffer, Word, Tag ) ){
	TaggedLexicon.Store( Word, Tag );
      }
    }
    int LexSize = TaggedLexicon.numOfLexiconEntries();
    vector<TagInfo *>TagVect = TaggedLexicon.CreateSortedVector();
    if ( (out_file.open( LexFileName, ios::out ),
	  out_file.good() ) ){
      cout << "  Creating lexicon: "  << LexFileName << " of "
	   << LexSize << " entries." << endl;
      for ( int i=0; i < LexSize; i++ )
	out_file << TagVect[i]->Freq() << " " << TagVect[i]->Word
		 << " " << TagVect[i]->DisplayTagFreqs() << endl;
      out_file.close();
    }
    else {
      cerr << "couldn't create lexiconfile " << LexFileName << endl;
      exit(EXIT_FAILURE);
    }
    for ( int i=0; i < LexSize; i++ )
      ProcessTags( TagVect[i] );
    if ( (out_file.open( MTLexFileName, ios::out ),
	  out_file.good() ) ){
      cout << "  Creating ambitag lexicon: "  << MTLexFileName << endl;
      for ( int j=0; j < LexSize; j++ ){
	out_file << TagVect[j]->Word << " " << TagVect[j]->stringRep() << endl;
	MT_lexicon->Store(TagVect[j]->Word, TagVect[j]->stringRep() );
      }
      out_file.close();
    }
    else {
      cerr << "couldn't create file: " << MTLexFileName << endl;
      exit(EXIT_FAILURE);
    }
    if ( (out_file.open( TopNFileName, ios::out ),
	  out_file.good() ) ){
      cout << "  Creating list of most frequent words: "  << TopNFileName << endl;
      for ( int k=0; k < LexSize && k < TopNumber; k++ ){
	out_file << TagVect[k]->Word << endl;
	kwordlist->Hash( TagVect[k]->Word );
      }
      out_file.close();
    }
    else {
      cerr << "couldn't open file: " << TopNFileName << endl;
      exit(EXIT_FAILURE);
    }
    if ( DoNpax ){
      if ( (out_file.open( NpaxFileName, ios::out ),
	    out_file.good() ) ){
	int np_cnt = 0;
	//	cout << "  Creating Npax file: "  << NpaxFileName;
	for ( int l=0; l < LexSize; l++ ){
	  if ( TagVect[l]->Freq() > Npax ) continue;
	  out_file << TagVect[l]->Word << endl;
	  uwordlist->Hash( TagVect[l]->Word );
	  np_cnt++;
	}
	out_file.close();
	//	cout << "( " << np_cnt << " entries)" << endl;
      }
      else {
	cerr << "couldn't open file: " << NpaxFileName << endl;
	exit(EXIT_FAILURE);
      }
    }
  }

  bool TaggerClass::InitLearning( ){
    // if not supplied on command line, make a default
    // name for both output files (concatenation of datafile
    // and pattern string)
    //
    create_lexicons();
    if ( TimblOptStr.empty() )
      Timbl_Options = "-a IB1 -G0";
    else
      Timbl_Options = TimblOptStr;
    splits( Timbl_Options, commonstr, knownstr, unknownstr );
    get_weightsfile_name( knownstr, kwf );
    get_weightsfile_name( unknownstr, uwf );
    return true;
  }

  int TaggerClass::makedataset( istream& infile, bool do_known ){
    int no_words=0;
    int no_sentences=0;
    int nslots=0;
    ofstream outfile;
    MatchAction Action;
    vector<int> TestPat;
    if( do_known ){
      nslots = Ktemplate.totalslots() - Ktemplate.skipfocus;
      outfile.open( K_option_name, ios::trunc | ios::out );
      Action = MakeKnown;
      TestPat.reserve(Ktemplate.totalslots());
    }
    else {
      nslots = Utemplate.totalslots() - Utemplate.skipfocus;
      outfile.open( U_option_name, ios::trunc | ios::out );
      Action = MakeUnknown;
      TestPat.reserve(Utemplate.totalslots());
    }
    // loop as long as you get sentences
    //
    int HartBeat = 0;
    size_t line_cnt = 0;
    sentence mySentence( Ktemplate, Utemplate );
    while ( mySentence.read( infile, input_kind, EosMark, line_cnt ) ){
      if ( mySentence.size() == 0 )
	continue;
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
	int thisTagCode;
	while( mySentence.nextpat( Action, TestPat,
				   *kwordlist, TheLex,
				   swcn ) ){
	  bool skip = false;
	  if ( DoNpax && !do_known ){
	    if((uwordlist->Lookup(mySentence.getword(swcn)))==0){
	      skip = true;
	    }
	  }
	  if ( !skip )
	    for( int f=0; f < nslots; f++){
	      outfile << indexlex( TestPat[f], TheLex ) << " ";
	    }
	  thisTagCode = TheLex.Hash( mySentence.gettag(swcn) );
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
	no_sentences++;
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
      if ( !Ktree->Valid() )
	exit(EXIT_FAILURE);
      COUT << "    Algorithm = " << to_string(Ktree->Algo()) << endl;
      if ( !piped_input ){
	string inname = TestFilePath + TestFileName;
	ifstream infile( inname, ios::in );
	if(infile.bad()){
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
      if ( !kwf.empty() )
	Ktree->SaveWeights( kwf );
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
      if ( !UKtree->Valid() )
	exit(EXIT_FAILURE);
      COUT << "    Algorithm = " << to_string(UKtree->Algo()) << endl;
      if ( !piped_input ){
	string inname = TestFilePath + TestFileName;
	ifstream infile( inname, ios::in );
	if(infile.bad()){
	  cerr << "Cannot read from " << inname << endl;
	  return 0;
	}
	// default_cout << "    Processing data from the file " << inname << "...";
	// default_cout.flush();
	nwords = makedataset( infile, false );
      }
      else {
	COUT << "Processing data from the standard input" << endl;
	nwords = makedataset( cin, false );
      }
      COUT << endl << "    Creating case base: " << UnknownTreeName << endl;
      UKtree->Learn( U_option_name );
      //      UKtree->ShowSettings( default_cout );
      UKtree->WriteInstanceBase( UnknownTreeName );
      if ( !uwf.empty() )
	UKtree->SaveWeights( uwf );
      delete UKtree;
      if ( !KeepIntermediateFiles ){
	remove(U_option_name.c_str());
	COUT << "    Deleted intermediate file: " << U_option_name << endl;
      }
    }
    return nwords;
  }

  void TaggerClass::CreateSettingsFile(){
    if ( SettingsFileName.empty() ) {
      SettingsFileName = TestFileName + ".settings";
    }
    ofstream out_file;
    if ( ( out_file.open( SettingsFileName, ios::out ),
	   !out_file.good() ) ){
      cerr << "couldn't create Settings-File `"
	   << SettingsFileName << "'" << endl;
      exit(EXIT_FAILURE);
    }
    else {
      if ( input_kind == ENRICHED )
	out_file << "ENRICHED" << endl;
      out_file << "e " << EosMark << endl;
      out_file << "l " << MTLexFileName << endl;
      out_file << "k " << KnownTreeName << endl;
      out_file << "u " << UnknownTreeName << endl;
      out_file << "p " << KtmplStr << endl;
      out_file << "P " << UtmplStr << endl;
      out_file << "O " << Timbl_Options << endl;
      out_file << "L " << TopNFileName << endl;
      out_file.close();
      cout << endl << "  Created settings file '"
			 << SettingsFileName << "'" << endl;
    }
  }

  //**** stuff to process commandline options *****************************

  const string mbt_create_short = "hV%:d:e:E:k:K:l:L:m:M:n:o:O:p:P:r:s:T:u:U:XD:";
  const string mbt_create_long = "version";

  bool TaggerClass::parse_create_args( TiCC::CL_Options& opts ){
    string value;
    if ( opts.is_present( 'V' ) ||
	 opts.is_present( "version" ) ){
      // we already identified ourself. just bail out.
      return false;
    }
    if ( opts.extract( '%', value ) ){
      FilterThreshold = stringTo<int>( value );
    }
    if ( opts.extract( 'd', value ) ){
      dumpflag=true;
      cout << "  Dumpflag ON" << endl;
    }
    if ( opts.extract( 'e', value ) ){
      EosMark = value;
      cout << "  Sentence delimiter set to '" << EosMark << "'" << endl;
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
      TopNumber = stringTo<int>(value);
    }
    if ( opts.extract( 'n', value ) ){
      Npax = stringTo<int>(value);
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
      if ( lastSlash != string::npos )
	SettingsFilePath = SettingsFileName.substr( 0, lastSlash+1 );
      else
	SettingsFilePath = "";
    }
    if ( opts.extract( 'E', value ) ){
      TestFileName = value;
      // extract the absolute path to the testfile
      string::size_type lastSlash = TestFileName.rfind('/');
      if ( lastSlash != string::npos ){
	TestFilePath = TestFileName.substr( 0, lastSlash+1 );
	TestFileName = TestFileName.substr( lastSlash+1 );
      }
      else
	TestFilePath = "";
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
      else
	TestFilePath = "";
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
      if ( value == "LogNormal" )
	cur_log->setlevel( LogNormal );
      else if ( value == "LogDebug" )
	cur_log->setlevel( LogDebug );
      else if ( value == "LogHeavy" )
	cur_log->setlevel( LogHeavy );
      else if ( value == "LogExtreme" )
	cur_log->setlevel( LogExtreme );
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

  void gen_usage( const string& progname ){
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

  int TaggerClass::CreateTagger( TiCC::CL_Options& opts ){
    string value;
    if ( opts.is_present( 'h' ) ||
	 opts.is_present( "help" ) ){
      gen_usage( "mbtg" );
      return true;
    }
    //
    // present yourself to the user
    //
    cerr << "mbtg " << VERSION << " (c) ILK and CLiPS 1998 - 2015." << endl
	 << "Memory Based Tagger Generator" << endl
	 << "Induction of Linguistic Knowledge Research Group,"
	 << "Tilburg University" << endl
	 << "CLiPS Computational Linguistics Group, University of Antwerp"
	 << endl
	 << "Based on " << Timbl::VersionName()
	 << endl << endl;
    TaggerClass tagger;
    if ( !tagger.parse_create_args( opts ) )
      return -1;
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
    cout << "      ready: " << kwords << " words processed."
	 << endl;
    tagger.CreateSettingsFile();
    return kwords + uwords;
  }

  int TaggerClass::CreateTagger( const string& opt_string ){
    TiCC::CL_Options opts;
    opts.allow_args( mbt_create_short, mbt_create_long );
    opts.parse_args( opt_string );
    return CreateTagger( opts );
  }

  int TaggerClass::CreateTagger( int argc, char* argv[] ){
    TiCC::CL_Options opts;
    opts.allow_args( mbt_create_short, mbt_create_long );
    opts.parse_args( argc, argv );
    return CreateTagger( opts );
  }

}
