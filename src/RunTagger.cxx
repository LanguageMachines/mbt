/*
  Copyright (c) 1998 - 2021
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
#include "ticcutils/PrettyPrint.h"
#include "ticcutils/json.hpp"
#include "mbt/Pattern.h"
#include "mbt/Sentence.h"
#include "mbt/Logging.h"
#include "mbt/Tagger.h"

#if defined(HAVE_PTHREAD)
#include <pthread.h>
#endif

using namespace TiCC;
using namespace nlohmann;

namespace Tagger {
  using namespace std;
  using namespace Hash;
  using namespace Timbl;
  using TiCC::operator<<;

  const string UNKSTR   = "UNKNOWN";

  class BeamData;

  BeamData::BeamData(){
    size = 0;
    paths = 0;
    temppaths = 0;
    path_prob = 0;
    n_best_array = 0;
  }

  BeamData::~BeamData(){
    if ( paths ){
      for ( int q=0; q < size; ++q ){
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
	for ( int q=0; q < Size; ++q ){
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
    for ( int q=0; q < Size; ++q ){
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
    for ( int i=0; i < size; ++i ){
      n_best_array[i]->clean();
    }
  }

  void BeamData::Shift(	int no_words, int i_word ){
    for ( int q1 = 0; q1 < no_words; ++q1 ){
      for ( int jb = 0; jb < size; ++jb ){
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
	  else {
	    temppaths[jb][q1] = EMPTY_PATH;
	  }
	}
	else {
	  temppaths[jb][q1] = EMPTY_PATH;
	}
      }
    }
    for ( int jb = 0; jb < size; ++jb ){
      for ( int q1=0; q1 < no_words; ++q1 ){
	paths[jb][q1] = temppaths[jb][q1];
      }
    }
  }

  void BeamData::Print( ostream& os, int i_word, StringHash& TheLex ){
    for ( int i=0; i < size; ++i ){
      os << "path_prob[" << i << "] = " << path_prob[i] << endl;
    }
    for ( int j=0; j <= i_word; ++j ){
      for ( int i=0; i < size; ++i ){
	if (  paths[i][j] != EMPTY_PATH ){
	  DBG << "    paths[" << i << "," << j << "] = "
	      << indexlex( paths[i][j], TheLex ) << endl;
	}
	else {
	  DBG << "    paths[" << i << "," << j << "] = EMPTY" << endl;
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
    name_prob_pair( const string& n, double p ): name(n),prob(p){
      next = 0;
    }
    ~name_prob_pair(){};
    string name;
    double prob;
    name_prob_pair *next;
  private:
    name_prob_pair( const name_prob_pair& ); // inhibit copies
    name_prob_pair operator=( const name_prob_pair& ); // inhibit copies
  };

  name_prob_pair *add_descending( name_prob_pair *n, name_prob_pair *lst ){
    name_prob_pair *result;
    if ( lst == 0 ){
      result = n;
    }
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
    if ( !Dist ){
      return 0;
    }
    double sum_freq = 0.0;
    for ( const auto& it : *Dist ){
      string name = it.second->Value()->Name();
      double freq = it.second->Weight();
      sum_freq += freq;
      tmp = new name_prob_pair( name, freq );
      if ( name == PrefClass->Name() ){
	assert( Pref == 0 );
	Pref = tmp;
      }
      else {
	result = add_descending( tmp, result );
      }
    }
    if ( Pref ){
      Pref->next = result;
      result = Pref;
    }
    //
    // Now we must Normalize te get real Probabilities
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
      name_prob_pair *d_pnt, *Distr;
      Distr = break_down( distrib, answer );
      d_pnt = Distr;
      int jb = 0;
      while ( d_pnt ){
	if ( jb < size ){
	  paths[jb][0] =  TheLex.Hash( d_pnt->name );
	  path_prob[jb] = d_pnt->prob;
	}
	name_prob_pair *tmp_d_pnt = d_pnt;
	d_pnt = d_pnt->next;
	delete tmp_d_pnt;
	jb++;
      }
      for ( ; jb < size; ++jb ){
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
      name_prob_pair *d_pnt, *Distr;
      Distr = break_down( distrib, answer );
      d_pnt = Distr;
      int ab = 0;
      while ( d_pnt ){
	if ( ab < size ){
	  double thisWProb = d_pnt->prob;
	  double thisPProb = thisWProb * path_prob[beam_cnt];
	  int dtag = TheLex.Hash( d_pnt->name );
	  for ( int ane = size-1; ane >=0; --ane ){
	    if ( thisPProb <= n_best_array[ane]->prob )
	      break;
	    if ( ane == 0 ||
		 thisPProb <= n_best_array[ane-1]->prob ){
 	      if ( ane == 0 ){
 		DBG << "Insert, n=0" << endl;
	      }
 	      else {
 		DBG << "Insert, n=" << ane << " Prob = " << thisPProb
		    << " after prob = " << n_best_array[ane-1]->prob
		    << endl;
	      }
	      // shift
	      n_best_tuple *keep = n_best_array[size-1];
	      for ( int ash = size-1; ash > ane; --ash ){
		n_best_array[ash] = n_best_array[ash-1];
	      }
	      n_best_array[ane] = keep;
	      n_best_array[ane]->prob = thisPProb;
	      n_best_array[ane]->path = beam_cnt;
	      n_best_array[ane]->tag = dtag;
	    }
	  }
	}
	name_prob_pair *tmp_d_pnt = d_pnt;
	d_pnt = d_pnt->next;
	delete tmp_d_pnt;
	++ab;
      }
    }
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

  void TaggerClass::ShowCats( ostream& os, const vector<int>& Pat, int slots ){
    os << "Pattern : ";
    for ( int slot=0; slot < slots; ++slot ){
      os << indexlex( Pat[slot], TheLex )<< " ";
    }
    os << endl;
  }

  string TaggerClass::pat_to_string( const sentence& mySentence,
				     const vector<int>& pat,
				     MatchAction action,
				     int word ){
    int slots;
    if ( action == Unknown ){
      slots = Utemplate.totalslots() - Utemplate.skipfocus;
    }
    else {
      slots = Ktemplate.totalslots() - Ktemplate.skipfocus;
    }
    string line;
    for ( int f=0; f < slots; ++f ){
      line += indexlex( pat[f], TheLex );
      line += " ";
    }
    const vector<string> enr = mySentence.getEnrichments(word);
    for ( const auto& er: enr ){
      line += er + " ";
    }
    if ( input_kind != UNTAGGED ){
      line += mySentence.gettag(word);
    }
    else {
      line += "??";
    }
    if ( IsActive(DBG) ){
      ShowCats( LOG, pat, slots );
    }
    // dump if desired
    //
    if ( dumpflag ){
      for ( int slot=0; slot < slots; ++slot ){
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
      MT_lexicon->insert(make_pair(wordbuf,valbuf));
      no_words++;
      lexfile >> ws;
    }
    LOG << "  Reading the lexicon from: " << FileName << " ("
	<< no_words << " words)." << endl;
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

    if ( TimblOptStr.empty() ){
      Timbl_Options = "-FColumns ";
    }
    else {
      Timbl_Options = TimblOptStr;
    }

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
    if ( !knowntreeflag ){
      cerr << "<knowntreefile> not specified" << endl;
      return false;
    }
    else if ( !unknowntreeflag ){
      cerr << "<unknowntreefile> not specified" << endl;
      return false;
    }
    KnownTree = new TimblAPI( knownstr + commonstr );
    if ( !KnownTree->Valid() ){
      return false;
    }
    unKnownTree = new TimblAPI( unknownstr + commonstr );
    if ( !unKnownTree->Valid() ){
      return false;
    }
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
	else {
	  cerr << "\n  Read known weights from " << kwf << endl;
	}
      }
      LOG << "  case-base for known words read." << endl;
      // read  a previously stored InstanceBase for unknown words
      //
      LOG << "  Reading case-base for unknown words from: "
	  << UnknownTreeName << "... " << endl;
      if ( !unKnownTree->GetInstanceBase( UnknownTreeName) ){
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
	  else {
	    LOG << "\n  Read unknown weights from " << uwf << endl;
	  }
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

  int TaggerClass::Run(){
    int result = -1;
    if ( initialized ){
      bool out_to_file = OutputFileName != "";
      ostream *os;
      if ( out_to_file ){
	os = new ofstream( OutputFileName );
      }
      else {
	os = &default_cout;
      }
      ifstream infile;
      if ( !piped_input ){
	string inname = TestFilePath + TestFileName;
	infile.open(inname, ios::in);
	if ( infile.bad( ) ){
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
	else {
	  result = ProcessFile( cin, *os );
	}
      }
      if ( out_to_file ){
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
    if ( cloned ){
      pthread_mutex_lock( &timbl_lock );
    }
#endif
    timer1.start();
    if ( Action == Known ){
      timer2.start();
      answer = KnownTree->Classify( teststring, *distribution, distance );
      timer2.stop();
    }
    else {
      timer3.start();
      answer = unKnownTree->Classify( teststring, *distribution, distance );
      timer3.stop();
    }
    timer1.stop();
#if defined(HAVE_PTHREAD)
    if ( cloned ){
      pthread_mutex_unlock( &timbl_lock );
    }
#endif
    if ( !answer ){
      throw runtime_error( "Tagger: A classifying problem prevented continuing. Sorry!" );
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
    if ( distance_flag ){
      distance_array[0] = distance;
    }
    if ( distribution ){
      if ( distrib_flag ){
	distribution_array[0] = distribution->DistToString();
      }
      if ( confidence_flag ){
	confidence_array[0] = distribution->Confidence( answer );
      }
      if ( IsActive( DBG ) ){
	LOG << "BeamData::InitPaths( " << mySentence << endl;
	LOG << " , " << answer << " , " << distribution << " )" << endl;
      }
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
    else if ( mySentence.nextpat( Action, TestPat,
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
	if ( distance_flag ){
	  distance_array[i_word] = distance;
	}
	if ( distribution ){
	  if ( distrib_flag ){
	    distribution_array[i_word] = distribution->DistToString();
	  }
	  if ( confidence_flag ){
	    confidence_array[i_word] = distribution->Confidence( answer );
	  }
	}
      }
      if ( IsActive( DBG ) ){
	LOG << "BeamData::NextPaths( " << mySentence << endl;
	LOG << " , " << answer << " , " << distribution << " )" << endl;
      }
      Beam->NextPath( TheLex, answer, distribution, beam_cnt );
      if ( IsActive( DBG ) ){
	Beam->PrintBest( LOG, TheLex );
      }
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
    sentence mySentence( Ktemplate, Utemplate );
    stringstream ss(line);
    size_t dummy = 0;
    mySentence.read( ss, input_kind, EosMark, Separators, dummy );
    return tagSentence( mySentence );
  }

  json TaggerClass::tag_line_to_JSON( const string& line ){
    sentence mySentence( Ktemplate, Utemplate );
    stringstream ss(line);
    size_t dummy = 0;
    mySentence.read( ss, input_kind, EosMark, Separators, dummy );
    vector<TagResult> tag_results = tagSentence( mySentence );
    json result = json::array();
    for ( const auto& tr : tag_results ){
      // lookup the assigned category
      json one_entry;
      one_entry["word"] = tr.word();
      one_entry["known"] = tr.is_known();
      if ( enriched() ){
	one_entry["enrichment"] = tr.enrichment();
      }
      one_entry["tag"] = tr.assigned_tag();
      if ( confidence_is_set() ){
	one_entry["confidence"] = tr.confidence();
      }
      if ( distrib_is_set() ){
	one_entry["distribution"] = tr.distribution();
      }
      if ( distance_is_set() ){
	one_entry["distance"] = tr.distance();
      }
      result.push_back( one_entry );
    }
    return result;
  }

  string extract_text( const json& my_json ){
    string result;
    if ( my_json.is_array() ){
      for ( const auto& it : my_json ){
	string tmp = it["word"];
	result += tmp + " ";
	if ( it.find("enrichment") != it.end() ){
	  tmp = it["enrichment"];
	  result += tmp + " ";
	  if ( it.find("tag") != it.end() ){
	    tmp = it["tag"];
	    result += tmp;
	  }
	  else {
	    result += "??";
	  }
	  result += "\n";
	}
      }
    }
    else {
      string tmp = my_json["word"];
      result += tmp +  " ";
      if ( my_json.find("enrichment") != my_json.end() ){
	tmp = my_json["enrichment"];
	result += tmp + " ";
	if ( my_json.find("tag") != my_json.end() ){
	  tmp = my_json["tag"];
	  result += tmp;
	}
	else {
	  result += "??";
	}
	result += "\n";
      }
    }
    return result;
  }

  json TaggerClass::tag_JSON_to_JSON( const json& in ){
    json result;
    string line = extract_text( in );
    return tag_line_to_JSON( line );
  }

  vector<TagResult> TaggerClass::tagSentence( sentence& mySentence ){
    vector<TagResult> result;
    if ( mySentence.size() != 0 ){
      if ( !initialized ||
	   !InitBeaming( mySentence.size() ) ){
	throw runtime_error( "Tagger not initialized" );
      }
      DBG << mySentence << endl;
      if ( mySentence.init_windowing( *MT_lexicon, TheLex ) ) {
	// here the word window is looked up in the dictionary and the values
	// of the features are stored in the testpattern
	MatchAction Action = Unknown;
	vector<int> TestPat;
	TestPat.reserve(Utemplate.totalslots());
	if ( mySentence.nextpat( Action, TestPat, *kwordlist, TheLex, 0 )){
	  DBG << "Start: " << mySentence.getword( 0 ) << endl;
	  InitTest( mySentence, TestPat, Action );
	  for ( unsigned int iword=1; iword < mySentence.size(); ++iword ){
	    // clear best_array
	    DBG << endl << "Next: " << mySentence.getword( iword ) << endl;
	    Beam->ClearBest();
	    for ( int beam_count=0; beam_count < Beam_Size; ++beam_count ){
	      if ( !NextBest( mySentence, TestPat, iword, beam_count ) ){
		break;
	      }
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
	res._word= TiCC::UnicodeToUTF8(mySentence.getword(Wcnt));
	// get the original tag
	res._input_tag = mySentence.gettag(Wcnt);
	// lookup the assigned tag
	res._tag = indexlex( Beam->paths[0][Wcnt], TheLex );
	// is it known/unknown
	res._known = mySentence.known(Wcnt);
	if ( input_kind == ENRICHED ){
	  res._enrichment = mySentence.getenr(Wcnt);
	}
	if ( confidence_flag ){
	  res._confidence = confidence_array[Wcnt];
	}
	if ( distrib_flag ){
	  res._distribution = distribution_array[Wcnt];
	}
	if ( distance_flag ){
	  res._distance = distance_array[Wcnt];
	}
	result.push_back( res );
      }
    } // end of output loop through one sentence
    return result;
  }

  string decode( const string& eom ){
    if ( eom  == "EL" ){
      return "";
    }
    else {
      return eom;
    }
  }

  string TaggerClass::TRtoString( const vector<TagResult>& trs ) const {
    string result;
    for ( const auto& tr : trs ){
      // lookup the assigned category
      result += tr.word();
      if ( tr.is_known() ){
	if ( input_kind == UNTAGGED ){
	  result += "/";
	}
	else {
	  result += "\t/\t";
	}
      }
      else {
	if ( input_kind == UNTAGGED ){
	  result += "//";
	}
	else {
	  result += "\t//\t";
	}
      }
      // output the correct tag if possible
      //
      if ( input_kind == ENRICHED ){
	result = result + tr.enrichment() + "\t";
      }
      if ( input_kind == TAGGED ||
	   input_kind == ENRICHED ){
	result += tr.input_tag() + "\t" + tr.assigned_tag();
	if ( confidence_flag ){
	  result += " [" + toString( tr.confidence() ) + "]";
	}
	if ( distrib_flag ){
	  result += " " + tr.distribution();
	}
	if ( distance_flag ){
	  result += " " + toString( tr.distance() );
	}
	result += "\n";
      }
      else {
	result += tr.assigned_tag();
	if ( confidence_flag ){
	  result += "/" + toString( tr.confidence() );
	}
	result += " ";
      }
    } // end of output loop through one sentence
    if ( input_kind != ENRICHED ){
      result = result + decode( EosMark );
    }
    return result;
  }

  void TaggerClass::statistics( const sentence& mySentence,
				int& no_known, int& no_unknown,
				int& no_correct_known,
				int& no_correct_unknown ){
    string tagstring;
    //now some output
    for ( unsigned int Wcnt=0; Wcnt < mySentence.size(); ++Wcnt ){
      tagstring = indexlex( Beam->paths[0][Wcnt], TheLex );
      if ( mySentence.known(Wcnt) ){
	no_known++;
	if ( input_kind != UNTAGGED ){
	  if ( mySentence.gettag(Wcnt) == tagstring ){
	    ++no_correct_known;
	  }
	}
      }
      else {
	no_unknown++;
	if ( input_kind != UNTAGGED ){
	  if ( mySentence.gettag(Wcnt) == tagstring ){
	    ++no_correct_unknown;
	  }
	}
      }
    } // end of output loop through one sentence
  }

  int TaggerClass::ProcessFile( istream& infile, ostream& outfile ){
    int no_words=0;
    int no_correct_known=0;
    int no_correct_unknown=0;
    int no_known=0;
    int no_unknown=0;
    static UnicodeString UniEos = TiCC::UnicodeFromUTF8(EosMark);
    // loop as long as you get sentences
    //
    int HartBeat = 0;
    size_t line_cnt = 0;
    sentence mySentence( Ktemplate, Utemplate );
    while ( mySentence.read(infile, input_kind, EosMark, Separators, line_cnt ) ){
      if ( mySentence.size() == 0 )
	continue;
      if ( ++HartBeat % 100 == 0 ) {
	cerr << "."; cerr.flush();
      }
      if ( mySentence.getword(0) == UniEos ){
	// only possible for ENRICHED!
	outfile << EosMark << endl;
	continue;
      }
      vector<TagResult> res = tagSentence( mySentence );
      string tagged_sentence = TRtoString( res );
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
	if ( no_unknown > 0 ){
	  cerr << " ("
	       << ((float)(no_unknown)/(float)(no_unknown+no_known))*100
	       << " %)";
	}
	cerr << endl;
	cerr << "  Total        : " << no_known+no_unknown << endl;
      }
    }
    return no_words;
  }


  bool TaggerClass::readsettings( string& fname ){
    ifstream setfile( fname, ios::in);
    if ( !setfile ){
      return false;
    }
    char SetBuffer[512];
    char value[512];
    while(setfile.getline(SetBuffer,511,'\n')){
      switch (SetBuffer[0]) {
      case 'B':
	if ( sscanf(SetBuffer,"B %40d", &Beam_Size ) != 1 ){
	  Beam_Size = 1;
	}
	break;
      case 'd':
	dumpflag=true;
	cerr << "  Dumpflag ON" << endl;
	break;
      case 'e': {
	sscanf( SetBuffer, "e %40s", value );
	EosMark = value;
	break;
      }
      case 'k':
	sscanf(SetBuffer,"k %300s", value );
	KnownTreeBaseName = value;
	KnownTreeName = prefixWithAbsolutePath( KnownTreeBaseName,
						SettingsFilePath );
	knowntreeflag = true; // there is a knowntreefile specified
	break;
      case 'l':
	sscanf(SetBuffer,"l %300s", value );
	l_option_name = value;
	l_option_name = prefixWithAbsolutePath( l_option_name,
						SettingsFilePath );
	lexflag = true; // there is a lexicon specified
	break;
      case 'L':
	sscanf(SetBuffer,"L %300s", value );
	L_option_name = value;
	L_option_name = prefixWithAbsolutePath( L_option_name,
						SettingsFilePath );
	klistflag = true;
	break;
      case 'o':
	sscanf(SetBuffer,"t %300s", value );
	OutputFileName = value;
	OutputFileName = prefixWithAbsolutePath( OutputFileName,
						 SettingsFilePath );
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
	sscanf(SetBuffer,"r %300s", value );
	r_option_name = value;
	r_option_name = prefixWithAbsolutePath( r_option_name,
						SettingsFilePath );
	reverseflag = true;
	break;
      case 't':
	sscanf(SetBuffer,"t %300s", value );
	TestFileName = value;
	TestFileName = prefixWithAbsolutePath( TestFileName,
					       SettingsFilePath );
	piped_input = false; // there is a test file specified
	break;
      case 'E':
	if ( SetBuffer[1] == ' ' && sscanf(SetBuffer,"E %300s", value ) > 0 ){
	  TestFileName = value;
	  TestFileName = prefixWithAbsolutePath( TestFileName,
						 SettingsFilePath );
	  piped_input = false;
	  input_kind = ENRICHED; // an enriched tagged test file specified
	}
	else if ( !strncmp( SetBuffer, "ENRICHED", 8 ) ){
	  input_kind = ENRICHED; // an enriched tagged test file specified
	}
	else {
	  cerr << "Unknown option in settingsfile, ("
	       << SetBuffer << "), ignored." <<endl;
	  break;
	}
	break;
      case 'T':
	sscanf(SetBuffer,"T %300s", value );
	TestFileName = value;
	TestFileName = prefixWithAbsolutePath( TestFileName,
					       SettingsFilePath );
	piped_input = false;
	input_kind = TAGGED; // there is a tagged test file specified
	break;
      case 'u':
	sscanf(SetBuffer,"u %300s", value );
	UnknownTreeBaseName = value;
	UnknownTreeName = prefixWithAbsolutePath( UnknownTreeBaseName,
						  SettingsFilePath );
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

  bool TaggerClass::parse_run_args( TiCC::CL_Options& Opts, bool as_server ){
    string value;
    if ( Opts.extract( 's', value ) ){
      // if a settingsfile option has been given, read that first
      // and then override with commandline options
      //
      SettingsFileName = value;
      // extract the absolute path to the settingsfile
      string::size_type lastSlash = SettingsFileName.rfind('/');
      if ( lastSlash != string::npos ){
	SettingsFilePath = SettingsFileName.substr( 0, lastSlash+1 );
      }
      else {
	SettingsFilePath = "";
      }
      if ( !readsettings( SettingsFileName ) ){
	cerr << "Cannot read settingsfile " << SettingsFileName << endl;
	return false;
      }
    };
    if ( Opts.extract( 'B', value ) ){
      int dum_beam = stringTo<int>(value);
      if ( dum_beam > 1 ){
	Beam_Size = dum_beam;
      }
      else {
	Beam_Size = 1;
      }
    };
    if ( Opts.extract( 'd', value ) ){
      dumpflag=true;
      cerr << "  Dumpflag ON" << endl;
    }
    if ( Opts.extract( 'D', value ) ){
      if ( value == "LogSilent" ) {
	cur_log->setlevel( LogSilent );
      }
      else if ( value == "LogNormal" ){
	cur_log->setlevel( LogNormal );
      }
      else if ( value == "LogDebug" ){
	cur_log->setlevel( LogDebug );
      }
      else if ( value == "LogHeavy" ){
	cur_log->setlevel( LogHeavy );
      }
      else if ( value == "LogExtreme" ){
	cur_log->setlevel( LogExtreme );
      }
      else {
	cerr << "Unknown Debug mode! (-D " << value << ")" << endl;
      }
    }
    if ( Opts.extract( 'e', value ) ){
      EosMark = value;
    }
    if ( Opts.extract( "tabbed" ) ){
      Separators = "\t";
    }
    if ( Opts.extract( 'k', value ) ){
      KnownTreeName = value;
      knowntreeflag = true; // there is a knowntreefile specified
    };
    if ( Opts.extract( 'l', value ) ){
      l_option_name = value;
      lexflag = true; // there is a lexicon specified
    };
    if ( Opts.extract( 'L', value ) ){
      L_option_name = value;
      klistflag = true;
    };
    if ( Opts.extract( 'o', value ) ){
      OutputFileName = value;
    };
    if ( Opts.extract( 'O', value ) ){  // Option string for Timbl
      TimblOptStr = value;
    };
    if ( Opts.extract( 'r', value ) ){
      r_option_name = value;
      reverseflag = true;
    }
    if ( Opts.extract( 't', value ) ){
      TestFileName = value;
      piped_input = false; // there is a test file specified
    };
    if ( Opts.extract( 'E', value ) ){
      TestFileName = value;
      piped_input = false;
      if ( input_kind == TAGGED ){
	cerr << "Option -E conflicts with TAGGED format from settingsfile "
	     << "unable to continue" << endl;
	return false;
      }
      input_kind = ENRICHED; // enriched tagged test file specified
    };
    if ( Opts.extract( 'T', value ) ){
      TestFileName = value;
      piped_input = false;
      if ( input_kind == ENRICHED ){
	cerr << "Option -T conflicts with ENRICHED format from settingsfile "
	     << "unable to continue" << endl;
	return false;
      }
      input_kind = TAGGED; // there is a tagged test file specified
    };
    if ( Opts.extract( 'u', value ) ){
      UnknownTreeName = value;
      unknowntreeflag = true; // there is a unknowntreefile file specified
    }
    if ( Opts.extract( 'v', value ) ){
      vector<string> opts = split_at( value, "+" );
      for ( const auto& o : opts ){
	if ( o == "di" ){
	  distance_flag = true;
	}
	else if ( o == "db" ){
	  distrib_flag = true;
	}
	else if ( o == "cf" ){
	  confidence_flag = true;
	}
      }
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
    if ( !Opts.empty() ){
      cerr << "unsupported options found: " << Opts.toString() << endl;
    }
    return true;
  }

  void TaggerClass::manifest( const string& prog ){
    // present yourself to the user
    //
    cerr << prog << " " << VERSION << " (c) CLST, ILK and CLiPS 1998 - 2021." << endl
	 << "Memory Based Tagger " << endl
	 << "CLST  - Centre for Language and Speech Technology,"
	 << "Radboud University" << endl
	 << "ILK   - Induction of Linguistic Knowledge Research Group,"
	 << "Tilburg University" << endl
	 << "CLiPS - Computational Linguistics Group, University of Antwerp"
	 << endl
	 << "Based on " << Timbl::VersionName()
	 << endl << endl;
  }

  const std::string mbt_short_opts = "hv:VB:dD:e:k:l:L:o:O:r:s:t:E:T:u:";
  const std::string mbt_long_opts  = "help,version,settings:,tabbed";

  void TaggerClass::run_usage( const string& progname ){
    cerr << "Usage is : " << progname << " option option ... \n"
	 << "\t-s settingsfile  ...or:\n\n"
	 << "\t-l <lexiconfile>\n"
	 << "\t-r <ambitagfile>\n"
	 << "\t-k <known words case base>\n"
	 << "\t-u <unknown words case base>\n"
	 << "\t-D <loglevel> (possible values are 'LogSilent', 'LogNormal', 'LogDebug', 'LogHeavy' and 'LogExtreme')\n"
	 << "\t-e <sentence delimiter> (default '<utt>')\n"
	 << "\t-E <enriched tagged testfile>\n "
	 << "\t-t <testfile> | -T <tagged testfile> "
	 << "(default is untagged stdin)\n"
	 << "\t--tabbed use tabs as separator in TAGGED input. (default is all whitespace)\n"
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

  TaggerClass *TaggerClass::StartTagger( TiCC::CL_Options& opts,
					 LogStream* os ){
    TaggerClass *tagger = new TaggerClass;
    if ( !tagger->parse_run_args( opts ) ){
      delete tagger;
      return 0;
    }
    if ( os ){
      tagger->setLog( *os );
    }
    else {
      // only manifest() when running 'standalone'
      manifest( "mbt" );
    }
    tagger->set_default_filenames();
    tagger->InitTagging();
    return tagger;
  }

}
