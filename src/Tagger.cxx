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

LogStream default_log( std::cerr ); // fall-back
LogStream default_cout( std::cout, "", NoStamp ); // guard cout too

#define COUT *Log(default_cout)

LogStream *cur_log = &default_log;  // fill the externals

LogLevel internal_default_level = LogNormal;
LogLevel Tagger_Log_Level       = internal_default_level;

namespace Tagger {
  using namespace std;
  using namespace Hash;
  using namespace Timbl;

  string Version() { return VERSION; }
  string VersionName() { return PACKAGE_STRING; }

  const string UNKSTR   = "UNKNOWN";

  class BeamData;

  TaggerClass::TaggerClass( ){
    cur_log = new LogStream( cerr );
    cur_log->setlevel( LogNormal );
    cur_log->setstamp( StampMessage );
    default_cout.setstamp( NoStamp );
    KnownTree = NULL;
    unKnownTree = NULL;
    TimblOptStr = "+vS -FColumns K: -a IGTREE +D U: -a IB1 ";
    FilterThreshold = 5;
    Npax = 5;
    TopNumber = 100;
    DoSort = false;
    DoTop = false;
    DoNpax = true;
    KeepIntermediateFiles = false;

    KtmplStr = "ddfa";
    UtmplStr = "dFapsss";
    L_option_name = "";
    EosMark = "<utt>";

    UnknownTreeName = "";
    KnownTreeName = "";
    LexFileName = "";
    MTLexFileName = "";
    TopNFileName = "";
    NpaxFileName = "";
    TestFileName = "";
    TestFilePath = "";
    OutputFileName = "";
    SettingsFileName = "";
    SettingsFilePath = "";

    initialized = false;
    Beam_Size = 1;
    Beam = NULL;
    MT_lexicon = new Lexicon();
    kwordlist = new StringHash();
    uwordlist = new StringHash();
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

  TaggerClass::TaggerClass( const TaggerClass& in ){
    cur_log = in.cur_log;
    KnownTree = in.KnownTree;
    unKnownTree = in.unKnownTree;
    TimblOptStr = in.TimblOptStr;
    FilterThreshold = in.FilterThreshold;
    Npax = in.Npax;
    TopNumber = in.TopNumber;
    DoSort = in.DoSort;
    DoTop = in.DoTop;
    DoNpax = in.DoNpax;
    KeepIntermediateFiles = in.KeepIntermediateFiles;

    KtmplStr = in.KtmplStr;
    UtmplStr = in.UtmplStr;
    L_option_name = in.L_option_name;
    EosMark = in.EosMark;

    Ktemplate = in.Ktemplate;
    Utemplate = in.Utemplate;

    UnknownTreeName = in.UnknownTreeName;
    KnownTreeName = in.KnownTreeName;
    LexFileName = in.LexFileName;
    MTLexFileName = in.MTLexFileName;
    TopNFileName = in.TopNFileName;
    NpaxFileName = in.NpaxFileName;
    TestFileName = in.TestFileName;
    TestFilePath = in.TestFilePath;
    OutputFileName = in.OutputFileName;
    SettingsFileName = in.SettingsFileName;
    SettingsFilePath = in.SettingsFilePath;

    initialized = in.initialized;
    Beam_Size = in.Beam_Size;
    Beam = 0;
    MT_lexicon = in.MT_lexicon;
    kwordlist = in.kwordlist;
    piped_input = in.piped_input;
    input_kind = in.input_kind;
    lexflag = in.lexflag;
    knowntreeflag = in.knowntreeflag;
    unknowntreeflag = in.unknowntreeflag;
    knowntemplateflag = in.knowntemplateflag;
    unknowntemplateflag = in.unknowntemplateflag;
    knownoutfileflag = in.knownoutfileflag;
    unknownoutfileflag = in.unknownoutfileflag;
    reverseflag = in.reverseflag;
    dumpflag = in.dumpflag;
    distance_flag = in.distance_flag;
    distrib_flag = in.distrib_flag;
    confidence_flag = in.confidence_flag;
    klistflag = in.klistflag;
    cloned = true;
  }

  bool TaggerClass::setLog( LogStream& os ){
    if ( !cloned )
      delete cur_log;
    cur_log = new LogStream( os, "mbt-" );
    return true;
  }

  BeamData::BeamData(){
    size = 0;
    paths = 0;
    temppaths = 0;
    path_prob = 0;
    n_best_array = 0;
  }

  BeamData::~BeamData(){
    if ( paths ){
      for ( int q=0; q < size; q++ ){
	delete n_best_array[q];
	delete [] paths[q];
	delete [] temppaths[q];
      }
    }
    delete [] paths;
    delete [] temppaths;
    delete [] path_prob;
    delete [] n_best_array;
  }

  const string& indexlex( const unsigned int index,
			  StringHash& aLex){
    return aLex.ReverseLookup( index );
  }


  bool BeamData::Init( int Size, unsigned int noWords ){
    // Beaming Stuff...
    if ( path_prob == 0 ){
      // the first time
      if ( (path_prob = new double[Size]) == 0 ||
	   (n_best_array = new n_best_tuple*[Size]) == 0 ||
	   (paths = new int*[Size]) == 0 ||
	   (temppaths = new int*[Size]) == 0 ){
	throw runtime_error( "Beam: not enough memory for N-best search tables" );
	return false;
      }
      else {
	for ( int q=0; q < Size; q++ ){
	  paths[q] = 0;
	  temppaths[q] = 0;
	  if ( (n_best_array[q] = new n_best_tuple) == 0 ){
	    throw runtime_error( "Beam: not enough memory for N-best search tables" );
	    return false;
	  }
	}
      }
    }
    else {
      for ( int q=0; q < Size; ++q ){
	delete [] paths[q];
	delete [] temppaths[q];
      }
    }
    for ( int q=0; q < Size; q++ ){
      if ( (paths[q] = new int[noWords]) == 0 ||
	   (temppaths[q] = new int[noWords]) == 0 ){
	throw runtime_error( "Beam: not enough memory for N-best search tables" );
	return false;
      }
    }
    size = Size;
    return true;
  }

  void BeamData::ClearBest(){
    DBG << "clearing n_best_array..." << endl;
    for ( int i=0; i < size; i++ )
      n_best_array[i]->clean();
  }

  void BeamData::Shift(	int no_words, int i_word ){
    for ( int q1 = 0; q1 < no_words; q1++ ){
      for ( int jb = 0; jb < size; jb++ ){
	path_prob[jb] = n_best_array[jb]->prob;
	if ( n_best_array[jb]->path != EMPTY_PATH ){
	  if ( q1 < i_word ){
	    DBG << "shift paths[" << n_best_array[jb]->path << ","
		<< q1 << "] into paths[" << jb << "," << q1 << "]" << endl;
	    temppaths[jb][q1] = paths[n_best_array[jb]->path][q1];
	  }
	  else if ( q1 == i_word ){
	    DBG << "shift tag " <<  n_best_array[jb]->tag
		<< " into paths[" << jb << "," << q1 << "]" << endl;
	    temppaths[jb][q1] = n_best_array[jb]->tag;
	  }
	  else
	    temppaths[jb][q1] = EMPTY_PATH;
	}
	else
	  temppaths[jb][q1] = EMPTY_PATH;
      }
    }
    for ( int jb = 0; jb < size; jb++ ){
      for ( int q1=0; q1 < no_words; q1++ )
	paths[jb][q1] = temppaths[jb][q1];
    }
  }

  void BeamData::Print( ostream& os, int i_word, StringHash& TheLex ){
    for ( int i=0; i < size; ++i ){
      os << "path_prob[" << i << "] = " << path_prob[i] << endl;
    }
    for ( int j=0; j <= i_word; ++j ){
      for ( int i=0; i < size; ++i ){
	if (  paths[i][j] != EMPTY_PATH ){
	  LOG << "    paths[" << i << "," << j << "] = "
	      << indexlex( paths[i][j], TheLex ) << endl;
	}
	else {
	  LOG << "    paths[" << i << "," << j << "] = EMPTY" << endl;
	}
      }
    }
  }

  void BeamData::PrintBest( ostream& os, StringHash& TheLex ){
    for ( int i=0; i < size; ++i ){
      if (  n_best_array[i]->path != EMPTY_PATH ){
	os << "n_best_array[" << i << "] = "
	   << n_best_array[i]->prob << " "
	   << n_best_array[i]->path << " "
	   << indexlex( n_best_array[i]->tag, TheLex ) << endl;
      }
      else {
	os << "n_best_array[" << i << "] = "
	    << n_best_array[i]->prob << " EMPTY " << endl;
      }
    }
  }

  class name_prob_pair{
  public:
    name_prob_pair( const string& n, double p ){
      name = n; prob = p; next = 0;
    }
    ~name_prob_pair(){};
    string name;
    double prob;
    name_prob_pair *next;
  };

  name_prob_pair *add_descending( name_prob_pair *n, name_prob_pair *lst ){
    name_prob_pair *result;
    if ( lst == 0 )
      result = n;
    else if ( n->prob - lst->prob >= 0 ){
      result = n;
      n->next = lst;
    }
    else {
      result = lst;
      result->next = add_descending( n, result->next );
    }
    return result;
  }


  name_prob_pair *break_down( const ValueDistribution *Dist,
			      const TargetValue *PrefClass ){
    // split a distribution into names/probabilities  AND sort them descending
    // But put preferred in front.
    // While we will use only the first BeamSize entries, don't forget
    // the most important one...!
    name_prob_pair *result = 0, *tmp, *Pref = 0;
    double sum_freq = 0.0;
    ValueDistribution::dist_iterator it = Dist->begin();
    while ( it != Dist->end() ){
      string name = (*it).second->Value()->Name();
      double freq = (*it).second->Weight();
      sum_freq += freq;
      tmp = new name_prob_pair( name, freq );
      if ( name == PrefClass->Name() )
	Pref = tmp;
      else
	result = add_descending( tmp, result );
      ++it;
    }
    if ( Pref ){
      Pref->next = result;
      result = Pref;
    }
    //
    // Now we must Normalize te get real Probalilities
    tmp = result;
    while ( tmp ){
      tmp->prob = tmp->prob / sum_freq;
      tmp = tmp->next;
    }
    return result;
  }

  void BeamData::InitPaths( StringHash& TheLex,
			    const TargetValue *answer,
			    const ValueDistribution *distrib ){
    if ( size == 1 ){
      paths[0][0] = TheLex.Hash( answer->Name() );
      path_prob[0] = 1.0;
    }
    else {
      name_prob_pair *d_pnt, *tmp_d_pnt, *Distr;
      Distr = break_down( distrib, answer );
      d_pnt = Distr;
      int jb = 0;
      while ( d_pnt ){
	if ( jb < size ){
	  paths[jb][0] =  TheLex.Hash( d_pnt->name );
	  path_prob[jb] = d_pnt->prob;
	}
	tmp_d_pnt = d_pnt;
	d_pnt = d_pnt->next;
	delete tmp_d_pnt;
	jb++;
      }
      for ( ; jb < size; jb++ ){
	paths[jb][0] = EMPTY_PATH;
	path_prob[jb] = 0.0;
      }
    }
  }

  void BeamData::NextPath( StringHash& TheLex,
			   const TargetValue *answer,
			   const ValueDistribution *distrib,
			   int beam_cnt ){
    if ( size == 1 ){
      n_best_array[0]->prob = 1.0;
      n_best_array[0]->path = beam_cnt;
      n_best_array[0]->tag = TheLex.Hash( answer->Name() );
    }
    else {
      DBG << "BeamData::NextPath[" << beam_cnt << "] ( " << answer << " , "
	  << distrib << " )" << endl;
      name_prob_pair *d_pnt, *tmp_d_pnt, *Distr;
      Distr = break_down( distrib, answer );
      d_pnt = Distr;
      int ab = 0;
      while ( d_pnt ){
	if ( ab < size ){
	  double thisWProb = d_pnt->prob;
	  double thisPProb = thisWProb * path_prob[beam_cnt];
	  int dtag = TheLex.Hash( d_pnt->name );
	  for( int ane = size-1; ane >=0; ane-- ){
	    if ( thisPProb <= n_best_array[ane]->prob )
	      break;
	    if ( ane == 0 ||
		 thisPProb <= n_best_array[ane-1]->prob ){
 	      if ( ane == 0 )
 		DBG << "Insert, n=0" << endl;
 	      else
 		DBG << "Insert, n=" << ane << " Prob = " << thisPProb
		    << " after prob = " << n_best_array[ane-1]->prob
		    << endl;
	      // shift
	      n_best_tuple *keep = n_best_array[size-1];
	      for ( int ash = size-1; ash > ane; ash-- ){
		n_best_array[ash] = n_best_array[ash-1];
	      }
	      n_best_array[ane] = keep;
	      n_best_array[ane]->prob = thisPProb;
	      n_best_array[ane]->path = beam_cnt;
	      n_best_array[ane]->tag = dtag;
	    }
	  }
	}
	tmp_d_pnt = d_pnt;
	d_pnt = d_pnt->next;
	delete tmp_d_pnt;
	++ab;
      }
    }
  }

  TaggerClass::~TaggerClass(){
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

  bool TaggerClass::InitBeaming( unsigned int no_words ){
    if ( !Beam ){
      Beam = new BeamData();
    }
    return Beam->Init( Beam_Size, no_words );
  }

  int TaggerClass::ProcessLines( istream &is, ostream& os ){
    int no_words=0;
    // loop as long as you get non empty sentences
    //
    string tagged_sentence;
    string line;
    while ( getline(is, line ) ){
      vector<TagResult> res = tagLine( line );
      int num = res.size();
      if ( num > 0 ){
	no_words += num;
	os << TRtoString( res ) << endl;
      }
    } // end of while looping over sentences
    cerr << endl << "Done:" << endl
	 << "  " << no_words << " words processed." << endl;
    return no_words;
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
      else
	known = opts.substr( k_pos+2 );
      done_k = true;
    }
    if ( u_pos != string::npos ){
      if ( u_pos < k_pos ){
	common += opts.substr( 0, u_pos );
	unknown = opts.substr( u_pos+2, k_pos - u_pos - 2 );
      }
      else
	unknown = opts.substr( u_pos+2 );
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
      else
	unknown = known;
    }
    else if ( !done_k ) {
      if ( u_pos != string::npos ){
	known = opts.substr( 0, u_pos );
      }
      else
	known = unknown;
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

  void prefixWithAbsolutePath( string& fileName,
			       const string& prefix ) {
    //    default_log << fileName << endl;
    if ( ( fileName[0] != '/' && fileName[1] != ':' )
	 && !( fileName[0]== '.' && fileName[1] == '/' ) ){
      fileName = prefix + fileName;
    }
    //    default_log << fileName << endl;
  }

  bool TaggerClass::set_default_filenames( ){
    //
    // and use them to setup the defaults...
    if( !KtmplStr.empty() ) {
      if ( Ktemplate.set( KtmplStr ) )
	knowntemplateflag = true;
      else {
	cerr << "couldn't set Known Template from '" << KtmplStr
	     << "'" << endl;
	return false;
      }
    }
    if ( !UtmplStr.empty() ) {
      if ( Utemplate.set( UtmplStr ) )
	unknowntemplateflag = true;
      else {
	cerr << "couldn't set Unknown Template from '" << UtmplStr
	     << "'" << endl;
	return false;
      }
    }
    char affix[32];
    LexFileName = TestFileName;
    prefixWithAbsolutePath( LexFileName, SettingsFilePath );
    LexFileName += ".lex";
    if ( FilterThreshold < 10 )
      sprintf( affix, ".0%1i",  FilterThreshold );
    else
      sprintf( affix, ".%2i",  FilterThreshold );
    if( !knownoutfileflag ){
      K_option_name = TestFileName;
      prefixWithAbsolutePath( K_option_name, SettingsFilePath );
      K_option_name += ".known.inst.";
      K_option_name += KtmplStr;
    }
    if ( !knowntreeflag ){
      KnownTreeName = TestFileName;
      prefixWithAbsolutePath( KnownTreeName, SettingsFilePath );
      KnownTreeName += ".known.";
      KnownTreeName += KtmplStr;
    }
    if( !unknownoutfileflag ){
      U_option_name = TestFileName;
      prefixWithAbsolutePath( U_option_name, SettingsFilePath );
      U_option_name += ".unknown.inst.";
      U_option_name += UtmplStr;
    }
    if ( !unknowntreeflag ){
      UnknownTreeName = TestFileName;
      prefixWithAbsolutePath( UnknownTreeName, SettingsFilePath );
      UnknownTreeName += ".unknown.";
      UnknownTreeName += UtmplStr;
    }
    if( lexflag ){
      MTLexFileName = l_option_name;
    }
    else {
      MTLexFileName = TestFileName;
      prefixWithAbsolutePath( MTLexFileName, SettingsFilePath );
      MTLexFileName += ".lex.ambi";
      MTLexFileName +=  affix;
    }
    if ( !L_option_name.empty() )
      TopNFileName = L_option_name;
    else {
      sprintf( affix, ".top%d",  TopNumber );
      TopNFileName = TestFileName + affix;
      prefixWithAbsolutePath( TopNFileName, SettingsFilePath );
    }
    sprintf( affix, ".%dpaxes",  Npax );
    NpaxFileName = TestFileName + affix;
    prefixWithAbsolutePath( NpaxFileName, SettingsFilePath );
    return true;
  }

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

  void TaggerClass::ShowCats( ostream& os, const vector<int>& Pat, int slots ){
    os << "Pattern : ";
    for( int slot=0; slot < slots; slot++){
      os << indexlex( Pat[slot], TheLex )<< " ";
    }
    os << endl;
  }

  string TaggerClass::pat_to_string( const sentence& mySentence,
				     const vector<int>& pat,
				     MatchAction action,
				     int word ){
    int slots;
    if ( action == Unknown )
      slots = Utemplate.totalslots() - Utemplate.skipfocus;
    else
      slots = Ktemplate.totalslots() - Ktemplate.skipfocus;
    string line;
    for( int f=0; f < slots; f++ ){
      line += indexlex( pat[f], TheLex );
      line += " ";
    }
    for (vector<string>::iterator it = mySentence.getWord(word)->extraFeatures.begin(); it != mySentence.getWord(word)->extraFeatures.end(); it++)
    {
      line += *it;
      line += " ";
    }
    if ( input_kind == TAGGED )
      line += mySentence.gettag(word);
    else
      line += "??";
    if ( IsActive(DBG) ){
      ShowCats( LOG, pat, slots );
    }
    // dump if desired
    //
    if ( dumpflag ){
      for( int slot=0; slot < slots; slot++){
	cout << indexlex( pat[slot], TheLex );
      }
      cout << endl;
    }
    return line;
  }

  void TaggerClass::read_lexicon( const string& FileName ){
    string wordbuf;
    string valbuf;
    int no_words=0;
    ifstream lexfile( FileName, ios::in);
    while ( lexfile >> wordbuf >> valbuf ){
      MT_lexicon->Store( wordbuf, valbuf );
      no_words++;
      lexfile >> ws;
    }
    LOG << "  Reading the lexicon from: " << FileName << " ("
	<< no_words << " words)." << endl;
  }

  bool old_style( const string& name ){
    string line;
    ifstream in( name );
    if ( in ){
      getline( in, line );
      vector<string> tmp;
      size_t count = split_at_first_of( line, tmp, ",." );
      if ( count <= 0 )
	return false;
      for( size_t i = 0; i < count; ++i ){
	if ( tmp[i].length() < 2 || tmp[i].length() > 3 )
	  return false;
	if ( !isalpha(tmp[i][0]) || !isalpha(tmp[i][1] ) )
	  return false;
	if ( tmp[i].length() == 3 && !isalpha(tmp[i][2] ) )
	  return false;
      }
      return true;
    }
    else
      return false;
  }

  //
  // File should contain one word per line.
  //
  void TaggerClass::read_listfile( const string& FileName, StringHash *words ){
    string wordbuf;
    int no_words=0;
    ifstream wordfile( FileName, ios::in);
    while( wordfile >> wordbuf ) {
      words->Hash( wordbuf );
      ++no_words;
    }
    LOG << "  Read frequent words list from: " << FileName << " ("
	<< no_words << " words)." << endl;
  }

  bool TaggerClass::InitTagging( ){
    if ( !cloned ){
      if ( !cur_log->set_single_threaded_mode() ){
// 	LOG << "PROBLEM setting to single threaded Failed" << endl;
// 	LOG << "Tagging might be slower than hoped for" << endl;
      }
    }
    // read the lexicon
    //
    read_lexicon( MTLexFileName );
    //
    read_listfile( TopNFileName, kwordlist );

    if ( TimblOptStr.empty() )
      Timbl_Options = "-FColumns ";
    else
      Timbl_Options = TimblOptStr;

    // we want Timbl to run silently ALWAYS
    // so overrule all -vS settings from old versions
    // smart users still can circumvent this ;)
    string::size_type pos = Timbl_Options.find( "-vS" );
    while ( pos != string::npos ){
      Timbl_Options[pos] = '+';
      pos = Timbl_Options.find( "-vS", pos+1 );
    }
    splits( Timbl_Options, commonstr, knownstr, unknownstr );
    if ( confidence_flag ){
      if ( commonstr.find("-G") == string::npos ){
	cerr << "-vcf is specified, but -G is missing in the common Timbl Options" << endl;
	return false;
      }
    }
    if( !knowntreeflag ){
      cerr << "<knowntreefile> not specified" << endl;
      return false;
    }
    else if( !unknowntreeflag){
      cerr << "<unknowntreefile> not specified" << endl;
      return false;
    }
    KnownTree = new TimblAPI( knownstr + commonstr );
    if ( !KnownTree->Valid() )
      return false;
    unKnownTree = new TimblAPI( unknownstr + commonstr );
    if ( !unKnownTree->Valid() )
      return false;
    // read a previously stored InstanceBase for known words
    //
    LOG << "  Reading case-base for known words from: " << KnownTreeName
	<< "... " << endl;
    if ( !KnownTree->GetInstanceBase( KnownTreeName) ){
      cerr << "Could not read the known tree from "
	   << KnownTreeName << endl;
      return false;
    }
    else {
      get_weightsfile_name( knownstr, kwf );
      get_weightsfile_name( unknownstr, uwf );
      if ( !kwf.empty() ){
	if ( !KnownTree->GetWeights( kwf ) ){
	  cerr << "Couldn't read known weights from " << kwf << endl;
	  return false;
	}
	else
	  cerr << "\n  Read known weights from " << kwf << endl;
      }
      LOG << "  case-base for known words read." << endl;
      // read  a previously stored InstanceBase for unknown words
      //
      LOG << "  Reading case-base for unknown words from: "
	  << UnknownTreeName << "... " << endl;
      if( !unKnownTree->GetInstanceBase( UnknownTreeName) ){
	LOG << "Could not read the unknown tree from "
	     << UnknownTreeName << endl;
	return false;
      }
      else {
	if ( !uwf.empty() ){
	  if ( !unKnownTree->GetWeights( uwf ) ){
	    cerr << "Couldn't read unknown weights from " << uwf << endl;
	    return false;
	  }
	  else
	    LOG << "\n  Read unknown weights from " << uwf << endl;
	}
	LOG << "  case-base for unknown word read" << endl;
      }
    }
    LOG << "  Sentence delimiter set to '" << EosMark << "'" << endl;
    LOG << "  Beam size = " << Beam_Size << endl;
    LOG << "  Known Tree, Algorithm = "
	<< to_string( KnownTree->Algo() ) << endl;
    LOG << "  Unknown Tree, Algorithm = "
	<< to_string( unKnownTree->Algo() ) << endl << endl;
    // the testpattern is of the form given in Ktemplate and Utemplate
    // here we allocate enough space for the larger of them to serve both
    //
    initialized = true;
    return true;
  }

  TaggerClass *TaggerClass::clone() const {
    TaggerClass *ta = new TaggerClass( *this );
    ta->Beam = NULL; // own Beaming data
    ta->cloned = true;
    return ta;
  }

  int TaggerClass::Run(){
    int result = -1;
    if ( initialized ){
      ostream *os;
      if ( OutputFileName != "" ){
	os = new ofstream( OutputFileName );
      }
      else
	os = &default_cout;
      ifstream infile;
      if ( !piped_input ){
	string inname = TestFilePath + TestFileName;
	infile.open(inname, ios::in);
	if( infile.bad( )){
	  cerr << "Cannot read from " << inname << endl;
	  result = 0;
	}
	else {
	  cerr << "Processing data from the file " << inname
	       << ":" <<  endl;
	  result = ProcessFile(infile, *os );
	}
      }
      else {
	cerr << "Processing data from the standard input" << endl;
	if ( input_kind == ENRICHED ){
	  cerr << "Enriched Inputformat not supported for stdin, sorry"
	       << endl;
	}
	else
	  result = ProcessFile( cin, *os );
      }
      if ( OutputFileName != "" ){
	delete os;
      }
    }
    return result;
  }

#if defined(HAVE_PTHREAD)
  static pthread_mutex_t timbl_lock = PTHREAD_MUTEX_INITIALIZER;
#endif

  const TargetValue *TaggerClass::Classify( MatchAction Action,
					    const string& teststring,
					    const ValueDistribution **distribution,
					    double& distance ){
    const TargetValue *answer = 0;
#if defined(HAVE_PTHREAD)
    if ( cloned )
      pthread_mutex_lock( &timbl_lock );
#endif
    if ( Action == Known ){
      answer = KnownTree->Classify( teststring, *distribution, distance );
    }
    else {
      answer = unKnownTree->Classify( teststring, *distribution, distance );
    }
#if defined(HAVE_PTHREAD)
    if ( cloned )
      pthread_mutex_unlock( &timbl_lock );
#endif
    if ( !answer ){
      throw runtime_error( "Tagger: A classifying problem prevented continuing. Sorry!" );
      exit(EXIT_FAILURE);
    }
    return answer;
  }

  void TaggerClass::InitTest( const sentence& mySentence,
			      vector<int>& TestPat,
			      MatchAction Action ){
    // Now make a testpattern for Timbl to process.
    string teststring = pat_to_string( mySentence, TestPat, Action, 0 );
    const ValueDistribution *distribution = 0;
    double distance;
    const TargetValue *answer = Classify( Action, teststring, &distribution, distance );
    distance_array.resize( mySentence.size() );
    distribution_array.resize( mySentence.size() );
    confidence_array.resize( mySentence.size() );
    if ( distance_flag )
      distance_array[0] = distance;
    if ( distribution ){
      if ( distrib_flag )
	distribution_array[0] = distribution->DistToString();
      if ( confidence_flag )
	confidence_array[0] = distribution->Confidence( answer );
    }
    if ( IsActive( DBG ) ){
      LOG << "BeamData::InitPaths( " << mySentence << endl;
      LOG << " , " << answer << " , " << distribution << " )" << endl;
    }
    Beam->InitPaths( TheLex, answer, distribution );
    if ( IsActive( DBG ) ){
      Beam->Print( LOG, 0, TheLex );
    }
  }


  bool TaggerClass::NextBest( const sentence& mySentence,
			      vector<int>& TestPat,
			      int i_word, int beam_cnt ){
    MatchAction Action = Unknown;
    if ( Beam->paths[beam_cnt][i_word-1] == EMPTY_PATH ){
      return false;
    }
    else if ( mySentence.nextpat( &Action, TestPat,
				  *kwordlist, TheLex,
				  i_word, Beam->paths[beam_cnt] ) ){
      // Now make a testpattern for Timbl to process.
      string teststring = pat_to_string( mySentence, TestPat, Action, i_word );
      // process teststring to predict a category, using the
      // appropriate tree
      //
      //      cerr << "teststring '" << teststring << "'" << endl;
      const ValueDistribution *distribution = 0;
      double distance;
      const TargetValue *answer = Classify( Action, teststring,
					    &distribution, distance );
      if ( beam_cnt == 0 ){
	if ( distance_flag )
	  distance_array[i_word] = distance;
	if ( distribution ){
	  if ( distrib_flag )
	    distribution_array[i_word] = distribution->DistToString();
	  if ( confidence_flag )
	    confidence_array[i_word] = distribution->Confidence( answer );
	}
      }
      Beam->NextPath( TheLex, answer, distribution, beam_cnt );
      if ( IsActive( DBG ) )
	Beam->PrintBest( LOG, TheLex );
      return true;
    }
    else {
      return false;
    }
  }

  int TaggerClass::TagLine( const std::string& inp, string& result ){
    vector<TagResult> res = tagLine( inp );
    result = TRtoString( res );
    return res.size();
  }

  vector<TagResult> TaggerClass::tagLine( const string& line ){
    vector<TagResult> result;
    sentence mySentence;
    mySentence.fill( line, input_kind );
    return tagSentence( mySentence );
  }

  vector<TagResult> TaggerClass::tagSentence( sentence& mySentence ){
    vector<TagResult> result;
    if ( mySentence.size() != 0 ){
      if ( !initialized ||
	   !InitBeaming( mySentence.size() ) ){
	throw runtime_error( "Tagger not initialized" );
      }
      DBG << mySentence << endl;

      if ( mySentence.init_windowing(&Ktemplate,&Utemplate,
				     *MT_lexicon, TheLex ) ) {
	// here the word window is looked up in the dictionary and the values
	// of the features are stored in the testpattern
	MatchAction Action = Unknown;
	vector<int> TestPat;
	TestPat.reserve(Utemplate.totalslots());
	if ( mySentence.nextpat( &Action, TestPat,
				 *kwordlist, TheLex,
				 0 )){

	  DBG << "Start: " << mySentence.getword( 0 ) << endl;
	  InitTest( mySentence, TestPat, Action );
	  for ( unsigned int iword=1; iword < mySentence.size(); iword++ ){
	    // clear best_array
	    DBG << endl << "Next: " << mySentence.getword( iword ) << endl;
	    Beam->ClearBest();
	    for ( int beam_count=0; beam_count < Beam_Size; beam_count++ ){
	      if ( !NextBest( mySentence, TestPat, iword, beam_count ) )
		break;
	    }
	    Beam->Shift( mySentence.size(), iword );
	    if ( IsActive( DBG ) ){
	      LOG << "after shift:" << endl;
	      Beam->Print( LOG, iword, TheLex );
	    }
	  }
	} // end one sentence
      }
      // get output
      for ( unsigned int Wcnt=0; Wcnt < mySentence.size(); ++Wcnt ){
	TagResult res;
	// get the original word
	res._word= mySentence.getword(Wcnt);
	// get the original tag
	res._inputTag = mySentence.gettag(Wcnt);
	// lookup the assigned tag
	res._tag = indexlex( Beam->paths[0][Wcnt], TheLex );
	// is it known/unknown
	res._known = mySentence.known(Wcnt);
	if ( input_kind == ENRICHED )
	  res._enrichment = mySentence.getenr(Wcnt);
	if ( confidence_flag )
	  res._confidence = confidence_array[Wcnt];
	if ( distrib_flag )
	  res._distribution = distribution_array[Wcnt];
	if ( distance_flag )
	  res._distance = distance_array[Wcnt];
	result.push_back( res );
      }
    } // end of output loop through one sentence
    return result;
  }

  string TaggerClass::TRtoString( const vector<TagResult>& tr ) const {
    string result;
    for ( unsigned int Wcnt=0; Wcnt < tr.size(); ++Wcnt ){
      // lookup the assigned category
      result += tr[Wcnt].word();
      if ( tr[Wcnt].isKnown() ){
	if ( input_kind == UNTAGGED )
	  result += "/";
	else
	  result += "\t/\t";
      }
      else {
	if ( input_kind == UNTAGGED )
	  result += "//";
	else
	  result += "\t//\t";
      }
      // output the correct tag if possible
      //
      if ( input_kind == ENRICHED )
	result = result + tr[Wcnt].enrichment() + "\t";
      if ( input_kind == TAGGED ||
	   input_kind == ENRICHED ){
	result += tr[Wcnt].inputTag() + "\t" + tr[Wcnt].assignedTag();
	if ( confidence_flag )
	  result += " [" + toString( tr[Wcnt].confidence() ) + "]";
	if ( distrib_flag )
	  result += " " + tr[Wcnt].distribution();
	if ( distance_flag )
	  result += " " + toString( tr[Wcnt].distance() );
	result += "\n";
      }
      else {
	result += tr[Wcnt].assignedTag();
	if ( confidence_flag )
	  result += "/" + toString( tr[Wcnt].confidence() );
	result += " ";
      }
    } // end of output loop through one sentence
    if ( input_kind != ENRICHED )
      result = result + EosMark;
    return result;
  }

  void TaggerClass::statistics( const sentence& mySentence,
				int& no_known, int& no_unknown,
				int& no_correct_known,
				int& no_correct_unknown ){
    string result;
    string tagstring;
    //now some output
    for ( unsigned int Wcnt=0; Wcnt < mySentence.size(); Wcnt++ ){
      tagstring = indexlex( Beam->paths[0][Wcnt], TheLex );
      if ( mySentence.known(Wcnt) ){
	no_known++;
	if ( input_kind != UNTAGGED ){
	  if ( mySentence.gettag(Wcnt) == tagstring )
	    no_correct_known++;
	}
      }
      else {
	no_unknown++;
	if ( input_kind != UNTAGGED ){
	  if ( mySentence.gettag(Wcnt) == tagstring )
	    no_correct_unknown++;
	}
      }
    } // end of output loop through one sentence
  }

  int TaggerClass::ProcessFile( istream& infile, ostream& outfile ){
    bool go_on = true;
    int no_words=0;
    int no_correct_known=0;
    int no_correct_unknown=0;
    int no_known=0;
    int no_unknown=0;

    // loop as long as you get sentences
    //
    int HartBeat = 0;
    sentence mySentence;
    while ( go_on &&
	    mySentence.read(infile, input_kind, EosMark ) ){
      if ( mySentence.size() == 0 )
	continue;
      string tagged_sentence;
      if ( ++HartBeat % 100 == 0 ) {
	cerr << "."; cerr.flush();
      }
      if ( mySentence.getword(0) == EosMark ){
	// only possible for ENRICHED!
	outfile << EosMark << endl;
	continue;
      }
      vector<TagResult> res = tagSentence( mySentence );
      tagged_sentence = TRtoString( res );
      if ( !tagged_sentence.empty() ){
	// show the results of 1 sentence
	statistics( mySentence,
		    no_known, no_unknown,
		    no_correct_known,
		    no_correct_unknown );
	outfile << tagged_sentence << endl;
	// increase the counter of processed words
	no_words += mySentence.size();
      }
      else {
	  // probably empty sentence??
      }
    } // end of while looping over sentences

    cerr << endl << endl << "Done: " << no_words
	 << " words processed." << endl << endl;
    if ( no_words > 0 ){
      if ( input_kind != UNTAGGED ){
	cerr << "Classification Statistics:" << endl;
	cerr << endl << "  Known Words:" << endl;
	KnownTree->ShowStatistics(cerr);
	cerr << endl << "  UnKnown Words:" << endl;
	unKnownTree->ShowStatistics(cerr);
	cerr << endl
	     << "  Total        : " << no_correct_known+no_correct_unknown
	     << "\tcorrect from " << no_known+no_unknown << " ("
	     << ((float)(no_correct_known+no_correct_unknown) /
		 (float)(no_known+no_unknown))*100
	     << " %)" << endl;
      }
      else {
	cerr << "  Known   words: " << no_known << endl;
	cerr << "  Unknown words: " << no_unknown;
	if ( no_unknown > 0 )
	  cerr << " ("
	       << ((float)(no_unknown)/(float)(no_unknown+no_known))*100
	       << " %)";
	cerr << endl;
	cerr << "  Total        : " << no_known+no_unknown << endl;
      }
    }
    return no_words;
  }


  bool TaggerClass::readsettings( string& fname ){
    ifstream setfile( fname, ios::in);
    if( !setfile ){
      return false;
    }
    char SetBuffer[512];
    char value[512];
    while(setfile.getline(SetBuffer,511,'\n')){
      switch (SetBuffer[0]) {
      case 'B':
	if ( sscanf(SetBuffer,"B %d", &Beam_Size ) != 1 )
	  Beam_Size = 1;
	break;
      case 'd':
	dumpflag=true;
	cerr << "  Dumpflag ON" << endl;
	break;
      case 'e': {
	sscanf( SetBuffer, "e %s", value );
	EosMark = value;
	break;
      }
      case 'k':
	sscanf(SetBuffer,"k %s", value );
	KnownTreeName = value;
	prefixWithAbsolutePath( KnownTreeName, SettingsFilePath );
	knowntreeflag = true; // there is a knowntreefile specified
	break;
      case 'l':
	sscanf(SetBuffer,"l %s", value );
	l_option_name = value;
	prefixWithAbsolutePath(l_option_name, SettingsFilePath );
	lexflag = true; // there is a lexicon specified
	break;
      case 'L':
	sscanf(SetBuffer,"L %s", value );
	L_option_name = value;
	prefixWithAbsolutePath(L_option_name, SettingsFilePath );
	klistflag = true;
	break;
      case 'o':
	sscanf(SetBuffer,"t %s", value );
	OutputFileName = value;
	prefixWithAbsolutePath(OutputFileName, SettingsFilePath );
	break;
      case 'O':  // Option string for Timbl
	TimblOptStr = string(SetBuffer+1);
	break;
      case 'p':  // windowing pattern for known words
	KtmplStr = string( SetBuffer+2 );
	break;
      case 'P':  // windowing pattern for unknown words
	UtmplStr = string( SetBuffer+2 );
	break;
      case 'r':
	sscanf(SetBuffer,"r %s", value );
	r_option_name = value;
	prefixWithAbsolutePath(r_option_name, SettingsFilePath );
	reverseflag = true;
	break;
      case 'S':
	cerr << "Server mode NOT longer supported in this version!\n"
	     << "use mbtserver instead\n"
	     << "sorry..." << endl;
	exit(EXIT_FAILURE);
	break;
      case 't':
	sscanf(SetBuffer,"t %s", value );
	TestFileName = value;
	prefixWithAbsolutePath(TestFileName, SettingsFilePath );
	piped_input = false; // there is a test file specified
	break;
      case 'E':
	if ( SetBuffer[1] == ' ' && sscanf(SetBuffer,"E %s", value ) > 0 ){
	  TestFileName = value;
	  prefixWithAbsolutePath(TestFileName, SettingsFilePath );
	  piped_input = false;
	  input_kind = ENRICHED; // an enriched tagged test file specified
	}
	else if ( !strncmp( SetBuffer, "ENRICHED", 8 ) )
	  input_kind = ENRICHED; // an enriched tagged test file specified
	else {
	  cerr << "Unknown option in settingsfile, ("
	       << SetBuffer << "), ignored." <<endl;
	  break;
	}
	break;
      case 'T':
	sscanf(SetBuffer,"T %s", value );
	TestFileName = value;
	prefixWithAbsolutePath(TestFileName, SettingsFilePath );
	piped_input = false;
	input_kind = TAGGED; // there is a tagged test file specified
	break;
      case 'u':
	sscanf(SetBuffer,"u %s", value );
	UnknownTreeName = value;
	prefixWithAbsolutePath(UnknownTreeName, SettingsFilePath );
	unknowntreeflag = true; // there is a unknowntreefile file specified
	break;
      default:
	cerr << "Unknown option in settingsfile, ("
	     << SetBuffer << "), ignored." <<endl;
	break;
      }
    }
    return true;
  }

  bool TaggerClass::parse_run_args( TimblOpts& Opts, bool as_server ){
    string value;
    bool mood;
    if ( Opts.Find( 'V', value, mood ) ||
	 Opts.Find( "version", value, mood ) ){
      manifest();
      return false;
    }
    if ( Opts.Find( 's', value, mood ) ){
      // if a settingsfile option has been given, read that first
      // and then override with commandline options
      //
      SettingsFileName = value;
      // extract the absolute path to the settingsfile
      string::size_type lastSlash = SettingsFileName.rfind('/');
      if ( lastSlash != string::npos )
	SettingsFilePath = SettingsFileName.substr( 0, lastSlash+1 );
      else
	SettingsFilePath = "";
      if( !readsettings( SettingsFileName ) ){
	cerr << "Cannot read settingsfile " << SettingsFileName << endl;
	return false;
      }
      Opts.Delete( 's' );
    };
    if ( Opts.Find( 'B', value, mood ) ){
      int dum_beam = stringTo<int>(value);
      if (dum_beam>1)
	Beam_Size = dum_beam;
      else
	Beam_Size = 1;
      Opts.Delete( 'B' );
    };
    if ( Opts.Find( 'd', value, mood ) ){
      dumpflag=true;
      cerr << "  Dumpflag ON" << endl;
      Opts.Delete( 'd' );
    }
    if ( Opts.Find( 'D', value, mood ) ){
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
      Opts.Delete( 'D' );
    }
    if ( Opts.Find( 'e', value, mood ) ){
      EosMark = value;
      Opts.Delete( 'e' );
    }
    if ( Opts.Find( 'k', value, mood ) ){
      KnownTreeName = value;
      knowntreeflag = true; // there is a knowntreefile specified
      Opts.Delete( 'k' );
    };
    if ( Opts.Find( 'l', value, mood ) ){
      l_option_name = value;
      lexflag = true; // there is a lexicon specified
      Opts.Delete( 'l' );
    };
    if ( Opts.Find( 'L', value, mood ) ){
      L_option_name = value;
      klistflag = true;
      Opts.Delete( 'L' );
    };
    if ( Opts.Find( 'o', value, mood ) ){
      OutputFileName = value;
      Opts.Delete( 'o' );
    };
    if ( Opts.Find( 'O', value, mood ) ){  // Option string for Timbl
      TimblOptStr = value;
      Opts.Delete( 'O' );
    };
    if ( Opts.Find( 'r', value, mood ) ){
      r_option_name = value;
      reverseflag = true;
      Opts.Delete( 'r' );
    }
    if ( Opts.Find( 'S', value, mood ) ){
      cerr << "Server mode NOT longer supported in this version!\n"
	   << "You must use mbtserver instead\n"
	   << "sorry..." << endl;
      return false;
    };
    if ( Opts.Find( 't', value, mood ) ){
      TestFileName = value;
      piped_input = false; // there is a test file specified
      Opts.Delete( 't' );
    };
    if ( Opts.Find( 'E', value, mood ) ){
      TestFileName = value;
      piped_input = false;
      input_kind = ENRICHED; // enriched tagged test file specified
      Opts.Delete( 'E' );
    };
    if ( Opts.Find( 'T', value, mood ) ){
      TestFileName = value;
      piped_input = false;
      if ( input_kind == ENRICHED ){
	cerr << "Option -T conflicts with ENRICHED format from settingsfile "
	     << "unable to continue" << endl;
	return false;
      }
      input_kind = TAGGED; // there is a tagged test file specified
      Opts.Delete( 'T' );
    };
    if ( Opts.Find( 'u', value, mood ) ){
      UnknownTreeName = value;
      unknowntreeflag = true; // there is a unknowntreefile file specified
      Opts.Delete( 'u' );
    }
    if ( Opts.Find( 'v', value, mood ) ){
      vector<string> opts;
      size_t num = split_at( value, opts, "+" );
      for ( size_t i = 0; i < num; ++i ){
	if ( opts[i] == "di" )
	  distance_flag = true;
	if ( opts[i] == "db" )
	  distrib_flag = true;
	if ( opts[i] == "cf" )
	  confidence_flag = true;
      }
      Opts.Delete( 'v' );
    };
    if ( cloned && input_kind == ENRICHED ){
      cerr << "Servermode doesn't support enriched inputformat!" << endl
	   << "bailing out, sorry " << endl;
      return false;
    }
    if ( !as_server &&
	 (!knowntreeflag || !unknowntreeflag) ){
      cerr << "missing required options. See 'mbt -h' " << endl;
      return false;
    }
    return true;
  }

  void TaggerClass::manifest( ){
    // present yourself to the user
    //
    LOG << "mbt " << VERSION << " (c) ILK and CLiPS 1998 - 2015." << endl
	<< "Memory Based Tagger " << endl
	<< "Tilburg University" << endl
	<< "CLiPS Computational Linguistics Group, University of Antwerp"
	<< endl
	<< "Based on " << Timbl::VersionName()
	<< endl << endl;
  }

  void run_usage( const string& progname ){
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

  TaggerClass *TaggerClass::StartTagger( int argc, char*argv[], LogStream* os ){
    TimblOpts Opts( argc, argv );
    string value;
    bool mood;
    if ( Opts.Find( 'h', value, mood ) ||
	 Opts.Find( "help", value, mood ) ){
      run_usage( "mbt" );
      return 0;
    }
    TaggerClass *tagger = new TaggerClass;
    if ( !tagger->parse_run_args( Opts ) ){
      delete tagger;
      return 0;
    }
    if ( os )
      tagger->setLog( *os );
    else // only manifest() when running 'standalone'
      tagger->manifest();
    tagger->set_default_filenames();
    tagger->InitTagging();
    return tagger;
  }

  TaggerClass *TaggerClass::StartTagger( const string& opts, LogStream* os ){
    TimblOpts Opts( opts );
    string value;
    bool mood;
    if ( Opts.Find( 'h', value, mood ) ||
	 Opts.Find( "help", value, mood ) ){
      run_usage( "mbt" );
      return 0;
    }
    TaggerClass *tagger = new TaggerClass;
    if ( !tagger->parse_run_args( Opts ) ){
      delete tagger;
      return 0;
    }
    if ( os )
      tagger->setLog( *os );
    else // only manifest() when running 'standalone'
      tagger->manifest();
    tagger->set_default_filenames();
    tagger->InitTagging();
    return tagger;
  }

  void RemoveTagger( TaggerClass* tagger ){
    delete tagger;
  }

  bool TaggerClass::InitLearning( ){
    // if not supplied on command line, make a default
    // name for both output files (concatenation of datafile
    // and pattern string)
    //
    create_lexicons();
    if ( TimblOptStr.empty() )
      Timbl_Options = "-a IB1";
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
    sentence mySentence;
    while ( mySentence.read( infile, input_kind, EosMark ) ){
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
      if ( mySentence.init_windowing( &Ktemplate,&Utemplate,
				      *MT_lexicon, TheLex ) ) {
	// we initialize the windowing procedure, this entails lexical lookup
	// of the words in the dictionary and the values
	// of the features are stored in the testpattern
	int swcn = 0;
	int thisTagCode;
	while( mySentence.nextpat( &Action, TestPat,
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
	    for (vector<string>::iterator it = mySentence.getWord(swcn)->extraFeatures.begin(); it != mySentence.getWord(swcn)->extraFeatures.end(); it++)
	      outfile << *it << " ";
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

  bool TaggerClass::parse_create_args( TimblOpts& Opts ){
    string value;
    bool mood;
    if ( Opts.Find( 'V', value, mood ) ||
	 Opts.Find( "version", value, mood ) ){
      // we already identified ourself. just bail out.
      return false;
    }
    if ( Opts.Find( '%', value, mood ) ){
      FilterThreshold = stringTo<int>( value );
    }
    if ( Opts.Find( 'd', value, mood ) ){
      dumpflag=true;
      cout << "  Dumpflag ON" << endl;
    }
    if ( Opts.Find( 'e', value, mood ) ){
      EosMark = value;
      cout << "  Sentence delimiter set to '" << EosMark << "'" << endl;
    }
    if ( Opts.Find( 'K', value, mood ) ){
      K_option_name = value;
      knownoutfileflag = true; // there is a knownoutfile specified
    }
    if ( Opts.Find( 'k', value, mood ) ){
      KnownTreeName = value;
      knowntreeflag = true; // there is a knowntreefile specified
    }
    if ( Opts.Find( 'l', value, mood ) ){
      l_option_name = value;
      lexflag = true; // there is a lexicon specified
    }
    if ( Opts.Find( 'L', value, mood ) ){
      L_option_name = value;
      klistflag = true;
    }
    if ( Opts.Find( 'M', value, mood ) ){
      TopNumber = stringTo<int>(value);
    }
    if ( Opts.Find( 'n', value, mood ) ){
      Npax = stringTo<int>(value);
      if ( Npax == 0 )
	DoNpax = false;
    }
    if ( Opts.Find( 'O', value, mood ) ){
      TimblOptStr = value; // Option string for Timbl
    }
    if ( Opts.Find( 'p', value, mood ) ){
      KtmplStr = value;  // windowing pattern for known words
    }
    if ( Opts.Find( 'P', value, mood ) ){
      UtmplStr = value;  // windowing pattern for unknown words
    }
    if ( Opts.Find( 'r', value, mood ) ){
      r_option_name = value;
      reverseflag = true;
    }
    if ( Opts.Find( 's', value, mood ) ){
      // if a settingsfile option has been specified, use that name
      SettingsFileName = value;
      // extract the absolute path to the settingsfile
      string::size_type lastSlash = SettingsFileName.rfind('/');
      if ( lastSlash != string::npos )
	SettingsFilePath = SettingsFileName.substr( 0, lastSlash+1 );
      else
	SettingsFilePath = "";
    }
    if ( Opts.Find( 'E', value, mood ) ){
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
    if ( Opts.Find( 'T', value, mood ) ){
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
    if ( Opts.Find( 'u', value, mood ) ){
      UnknownTreeName = value;
      unknowntreeflag = true; // there is a unknowntreefile file specified
    }
    if ( Opts.Find( 'U', value, mood ) ){
      U_option_name = value;
      unknownoutfileflag = true; // there is a unknownoutfile specified
    }
    if ( Opts.Find( 'X', value, mood ) ){
      KeepIntermediateFiles = true;
    }
    if ( Opts.Find( 'D', value, mood ) ){
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
      Opts.Delete( 'D' );
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

  int TaggerClass::CreateTagger( TimblOpts& Opts ){
    string value;
    bool mood;
    if ( Opts.Find( 'h', value, mood ) ||
	 Opts.Find( "help", value, mood ) ){
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
    if ( !tagger.parse_create_args( Opts ) )
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
    TimblOpts opts( opt_string );
    return CreateTagger( opts );
  }

  int TaggerClass::CreateTagger( int argc, char* argv[] ){
    TimblOpts opts( argc, argv );
    return CreateTagger( opts );
  }

}
