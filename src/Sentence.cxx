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

#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
#include <cctype>
#include <cctype>
#include <cassert>

#include "ticcutils/StringOps.h"
#include "mbt/Pattern.h"
#include "mbt/Sentence.h"

namespace Tagger {
  using namespace Hash;
  using namespace std;

  const string Separators = "\t \n";
  const int MAXTCPBUF   = 65536;

  // New enriched word.
  //
  word::word( const string& some_word, const vector<string>& extra_features, const string& some_tag){
    the_word =  some_word;
    word_tag = some_tag;
    the_word_index = -1;
    extraFeatures = extra_features;
  }

  // Delete a word
  //
  word::~word(){
  }

  // New sentence.
  //
  sentence::sentence( const PatTemplate& k, const PatTemplate& u ):
    UTAG(-1), Ktemplate(k), Utemplate(u), no_words(0)
  {}

  // Delete it.
  //
  sentence::~sentence(){
    clear();
  }

  void sentence::clear(){
    for ( unsigned int i=0; i < no_words; i++ )
      delete Words[i];
    Words.clear();
    no_words = 0;
  }

  ostream& operator<<( ostream& os, const sentence& s ){
    s.print( os );
    return os;
  }

  string sentence::getenr( unsigned int index ){
    string result;
    if ( index < no_words ){
      const std::vector<std::string> enr = getEnrichments( index );
      auto it = Words[index]->extraFeatures.cbegin();
      while( it != Words[index]->extraFeatures.cend() ){
	result += *it;
	++it;
	if (  it != Words[index]->extraFeatures.end() )
	  result += " ";
      }
    }
    return result;
  }

  // Print it.
  //
  void sentence::print( ostream &os ) const{
    os << "Sentence :'";
    if ( no_words != 0 ){
      for ( unsigned int i = 0; i < no_words-1; i++ )
	os << Words[i]->the_word << ", ";
      os << Words[no_words-1]->the_word;
    }
    os << "'";
  }

  bool sentence::Utt_Terminator( const string& z_something ){
    if ( InternalEosMark == "EL" )
      return z_something.empty();
    return ( z_something == InternalEosMark );
  }

  // Add an enriched word to a sentence.
  //
  void sentence::add( const string& a_word,
		      const vector<string>& extraFeatures,
		      const string& a_tag ){
    Words.push_back( new word( a_word, extraFeatures, a_tag ) );
    ++no_words;
  }

  // Add a word to a sentence.
  //
  void sentence::add(const string& a_word, const string& a_tag)
  {
    vector<string> tmp;
    add(a_word, tmp, a_tag);
  }

  bool sentence::init_windowing( Lexicon &lex,
				 StringHash& TheLex ) {
    if ( UTAG == -1 )
      UTAG = TheLex.Hash( UNKNOWN );
    if ( no_words == 0 ) {
      //    cerr << "ERROR: empty sentence?!" << endl;
      return false;
    }
    else {
      LexInfo * foundInfo;
      word *cur_word;
      for ( unsigned int wpos = 0; wpos < no_words; ++wpos ){
	cur_word = Words[wpos];
	cur_word->the_word_index = TheLex.Hash( cur_word->the_word );
	// look up ambiguous tag in the dictionary
	//
	foundInfo = lex.Lookup( cur_word->the_word );
	if( foundInfo != NULL ){
	  //	  cerr << "MT Lookup(" << cur_word->the_word << ") gave " << *foundInfo << endl;
	  cur_word->word_amb_tag = TheLex.Hash( foundInfo->Trans() );
	}
	else  {
	  // cerr << "MT Lookup(" << cur_word->the_word << ") gave NILL" << endl;
	  // not found, so give the appropriate unknown word code
	  cur_word->word_amb_tag = UTAG;
	}
      };
      return true;
    }
  }

  int sentence::classify_hapax( const string& word, StringHash& TheLex ) const{
    string hap = "HAPAX-";
    if ( word.find( "-" ) != string::npos ) // hyphen anywere
      hap += 'H';
    if ( isupper( word[0] ) ){ // Capitalized first letter?
      hap += 'C';
    }
    if ( word.find_first_of( "0123456789" ) != string::npos ) // digit anywhere
      hap += 'N';
    if ( hap.length() == 6 )
      hap += '0';
    return TheLex.Hash( hap );
  }

  bool sentence::nextpat( MatchAction& Action, vector<int>& Pat,
			  StringHash& wordlist, StringHash& TheLex,
			  unsigned int position, int *old_pat ) const {
    // safety check:
    //
    if( no_words == 0 || position >= no_words)
      return false;
    word *current_word = Words[position];
    size_t CurWLen = current_word->the_word.length();
    int i_feature=0;
    const PatTemplate *aTemplate;
    word* wPtr;
    unsigned int tok;
    // is the present pattern for a known or unknown word?
    //
    if( Action == MakeKnown )
      aTemplate = &Ktemplate;
    else if ( Action == MakeUnknown )
      aTemplate = &Utemplate;
    else if( current_word->word_amb_tag == UTAG ){
      Action = Unknown;
      //      cerr << "Next pat, Unknown word = "
      //  	 << current_word->the_word << endl;
      aTemplate = &Utemplate;
    }
    else {
      Action = Known;
      //      cerr << "Next pat, Known word = "
      //  	 << current_word->the_word << endl;
      aTemplate = &Ktemplate;
    }

    // Prefix?
    //
    if (aTemplate->numprefix > 0) {
      for ( size_t j = 0;
	    j < (size_t)aTemplate->numprefix; j++) {
	string addChars = "_";
	if ( j < CurWLen )
	  addChars += current_word->the_word[j];
	else
	  addChars += '=';  // "_=" denotes "no value"
	Pat[i_feature] = TheLex.Hash( addChars );
	i_feature++;
      }
    }

    for ( unsigned int i = 0, c_pos = position - aTemplate->word_focuspos;
	  i < aTemplate->wordslots;
	  i++, c_pos++) {
      // Now loop.
      //
      // depending on the slot type, transfer the appropriate
      // information to the next feature
      //
      if ( c_pos < no_words ) {
	wPtr = Words[c_pos];
	//
	// If a list is specified, check if wPtr->the_word is
	// allowed.
	//
	switch(aTemplate->word_templatestring[i]) {
	case 'w':
	  if ( wordlist.NumOfEntries() == 0 ){
	    Pat[i_feature] = wPtr->the_word_index;
	  }
	  else {
	    tok = wordlist.Lookup( wPtr->the_word );
	    //cerr << "known word Lookup(" << wPtr->the_word << ") gave " << tok << endl;
	    if ( tok ){
	      Pat[i_feature] = wPtr->the_word_index;
	    }
	    else
	      Pat[i_feature] = classify_hapax(  wPtr->the_word, TheLex );
	  }
	  i_feature++;
	  break;
	}
      }
      else {   // Out of context.
	Pat[i_feature] = TheLex.Hash( DOT );
	i_feature++;
      }
    } // i

    // Lexical and Context features ?
    //
    for ( unsigned int ii = 0, cc_pos = position - aTemplate->focuspos;
	  ii < aTemplate->numslots;
	  ii++, cc_pos++ ) {

      // move a pointer to the position of the word that
      // should occupy the present template slot
      //
      if ( cc_pos < no_words ) {
	wPtr = Words[cc_pos];
	// depending on the slot type, transfer the appropriate
	// information to the next feature
	//
	switch(aTemplate->templatestring[ii]){
	case 'd':
	  if ( old_pat == 0 )
	    Pat[i_feature] = wPtr->word_ass_tag;
	  else {
	    // cerr << "bekijk old pat = " << position+ii-aTemplate->focuspos
	    //  << " - " << old_pat[position+ii-aTemplate->focuspos] << endl;
	    Pat[i_feature] = old_pat[position+ii-aTemplate->focuspos];
	  }
	  i_feature++;
	  break;
	case 'f':
	  Pat[i_feature] = wPtr->word_amb_tag;
	  i_feature++;
	  break;
	case 'F':
	  break;
	case 'a':
	  Pat[i_feature] = wPtr->word_amb_tag;
	  i_feature++;
	  break;
      }
      }
      else{   // Out of context.
	Pat[i_feature] = TheLex.Hash( DOT );
	i_feature++;
      }
    } // i

    // Suffix?
    //
    if (aTemplate->numsuffix > 0) {
      for ( size_t j = aTemplate->numsuffix; j > 0; j--) {
	string addChars = "_";
	if ( j <= CurWLen )
	  addChars  += current_word->the_word[CurWLen - j];
	else
	  addChars += '=';
	Pat[i_feature] = TheLex.Hash( addChars );
	i_feature++;
      }
    }

    // Hyphen?
    //
    if (aTemplate->hyphen) {
      string addChars;
      if ( current_word->the_word.find('-') != string::npos )
	addChars = "_H";
      else
	addChars = "_0";
      Pat[i_feature] = TheLex.Hash( addChars );
      i_feature++;
    }

    // Capital (First Letter)?
    //
    if (aTemplate->capital) {
      string addChars = "_";
      if ( isupper(current_word->the_word[0]) )
	addChars += 'C';
      else
	addChars += '0';
      Pat[i_feature] = TheLex.Hash( addChars );
      i_feature++;
    }

    // Numeric (somewhere in word)?
    //
    if (aTemplate->numeric) {
      string addChars = "_0";
      for ( unsigned int j = 0; j < CurWLen; j++) {
	if( isdigit(current_word->the_word[j]) ){
	  addChars[1] = 'N';
	  break;
	}
      }
      Pat[i_feature] = TheLex.Hash( addChars );
      i_feature++;
    }
    //    cerr << "next_pat: i_feature = " << i_feature << endl;
    //    for ( int bla = 0; bla < i_feature; bla++ )
    //      cerr << bla << " - " << Pat[bla] << endl;
    return true;
  }

  void sentence::assign_tag( int cat, unsigned int pos ){
    // safety check:
    //
    if( no_words > 0 && pos < no_words )
      Words[pos]->word_ass_tag = cat;
  }

  bool sentence::known( unsigned int i ) const {
    if( no_words > 0 && i < no_words )
      return Words[i]->word_amb_tag != UTAG;
    else
      return false;
  }

  bool sentence::read( istream &infile, input_kind_type kind,
		       const string& eom, size_t& line ){
    if ( !infile )
      return false;
    InternalEosMark = eom;
    //    cerr << "READ zet InternalEosMark = " << eom << endl;
    if ( kind == TAGGED ){
      return read_tagged( infile, line );
    }
    else if ( kind == UNTAGGED ){
      return read_untagged( infile, line );
    }
    else
      return read_enriched( infile, line );
  }

  bool sentence::read_tagged( istream &infile, size_t& line_no ){
    // read a whole sentence from a stream
    // A sentence can be delimited either by an Eos marker or EOF.
    clear();
    string line;
    while ( getline( infile, line ) ){
      ++line_no;
      //      cerr << "read line: " << line << endl;
      line = TiCC::trim( line );
      if ( line.empty() ){
	if ( InternalEosMark == "EL" ){
	  return true;
	}
	continue;
      }
      else if ( Utt_Terminator( line ) ){
	return true;
      }
      vector<string> parts;
      int num = TiCC::split_at_first_of( line, parts, Separators );
      if ( num != 2 ){
#pragma omp critical
	{
	  cerr << endl << "error in line " << line_no << " : '"
	       << line << "' (skipping it)" << endl;
	  if ( num == 1 ){
	    cerr << "missing a tag ? " << endl;
	  }
	  else {
	    cerr << "extra values found " << endl;
	  }
	}
	continue;
      }
      add( parts[0], parts[1] );
    }
    //    cerr << "read a sentence: " << *this << endl;
    return true;
  }

  bool sentence::read_untagged( istream &infile, size_t& line_no ){
    // read a whole sentence from a stream
    // A sentence can be delimited either by an Eos marker or EOF.
    clear();
    //    cerr << "untagged-read remainder='" << remainder << "'" << endl;
    string line = remainder;
    remainder.clear();
    while ( !line.empty() || getline( infile, line ) ){
      ++line_no;
      //      cerr << "untagged-read line: " << line << endl;
      line = TiCC::trim( line );
      if ( line.empty() ){
	if ( InternalEosMark == "EL" ){
	  return true;
	}
	continue;
      }
      vector<string> parts;
      TiCC::split_at_first_of( line, parts, " \t" );
      line = "";
      bool terminated = false;
      for ( auto p : parts ){
	//	cerr << "bekijk " << p << endl;
	if ( Utt_Terminator( p ) ){
	  terminated = true;
	}
	else {
	  if ( terminated ){
	    remainder += p + " ";
	  }
	  else {
	    add( p, "" );
	  }
	}
      }
      if ( terminated )
	return true;
    }
    return true;
  }

  bool sentence::read_enriched( istream &infile, size_t& line_no ){
    // read a sequence of enriched and tagged words from infile
    // every word must be a one_liner
    // cleanup the sentence for re-use...
    clear();
    string line;
    string Word;
    string Tag;
    vector<string> extras;
    while( getline( infile, line ) ){
      ++line_no;
      line = TiCC::trim( line );
      if ( line.empty() ){
	if ( InternalEosMark == "EL" ){
	  return true;
	}
	continue;
      }
      else if ( Utt_Terminator( line ) ){
	return true;
      }
      size_t size = TiCC::split_at_first_of( line, extras, Separators );
      if ( size >= 2 ){
	Word = extras.front();
	extras.erase(extras.begin()); // expensive, but allas. extras is small
	Tag  = extras.back();
	extras.pop_back();
	if ( !Word.empty() && !Tag.empty() ){
	  add( Word, extras, Tag );
	}
      }
    };
    return no_words > 0;
  }

} // namespace
