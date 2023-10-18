/*
  Copyright (c) 1998 - 2023
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

#include <fstream>
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <cstdlib>
#include <cctype>
#include <cctype>
#include <cassert>

#include "ticcutils/Unicode.h"
#include "ticcutils/StringOps.h"
#include "mbt/Pattern.h"
#include "mbt/Sentence.h"

namespace Tagger {
  using namespace Hash;
  using namespace std;
  using namespace icu;

  // New enriched word.
  //
  word::word( const UnicodeString& some_word,
	      const vector<UnicodeString>& extra_features,
	      const UnicodeString& some_tag ):
    /*!
      construct a word structure
      \param some_word the string value of the word
      \param extra_features list of optional enrichment values
      \param some_tag the tag to assign to the word
     */
    the_word( some_word ),
    word_tag( some_tag ),
    word_amb_tag( -1 ),
    word_ass_tag( -1 ),
    extraFeatures( extra_features )
  {
    the_word_index = -1;
  }

  word::~word(){
    /// destruct a word structure
  }

  sentence::sentence( const PatTemplate& k, const PatTemplate& u ):
    /*!
      construct a sentence using the given Pattern Templates

      \param k the Pattern for \e known words
      \param u the Pattern for \e inknown words
    */
    UTAG(-1), Ktemplate(k), Utemplate(u), no_words(0)
  {
  }

  sentence::~sentence(){
    /// destruct a sentence
    clear();
  }

  void sentence::clear(){
    /// reset the sentence by removing al the words in it.
    for ( const auto& w : Words ){
      delete w;
    }
    Words.clear();
    no_words = 0;
  }

  ostream& operator<<( ostream& os, const sentence& s ){
    /// output a \e sentence to a stream \e os
    s.print( os );
    return os;
  }

  UnicodeString sentence::getenr( unsigned int index ){
    UnicodeString result;
    if ( index < no_words ){
      for ( const auto& it : Words[index]->extraFeatures ){
	result += it;
	if (  &it != &Words[index]->extraFeatures.back() ){
	  result += " ";
	}
      }
    }
    return result;
  }

  // Print it.
  //
  void sentence::print( ostream &os ) const {
    /// Print a sentence (debugging only)
    //
    os << "Sentence :'";
    for ( const auto& w : Words ){
      os << w->the_word;
      if ( &w != &Words.back() ){
	os << ", ";
      }
    }
    os << "'";
  }

  bool sentence::Utt_Terminator( const UnicodeString& test ){
    /// check if the parameter equals the current EOS marker
    /*!
      /param test the value to check
      /return true if the strings are equal, false otherwise

      When the current EOS marker is set to the value "EL" an empty 'test'
      value is a match too.
    */
    if ( InternalEosMark == "EL" ){
      return test.isEmpty();
    }
    return ( test == InternalEosMark );
  }

  // Add an enriched word to a sentence.
  //
  void sentence::add( const UnicodeString& u_word,
		      const vector<UnicodeString>& extraFeatures,
		      const UnicodeString& u_tag ){
    Words.push_back( new word( u_word,
			       extraFeatures,
			       u_tag ) );
    ++no_words;
  }

  // Add a word to a sentence.
  //
  void sentence::add( const UnicodeString& a_word,
		      const UnicodeString& a_tag)
  {
    vector<UnicodeString> tmp;
    add(a_word, tmp, a_tag);
  }

  bool sentence::init_windowing( map<UnicodeString,UnicodeString>& lex,
				 UnicodeHash& TheLex ) {
    if ( UTAG == -1 ){
#pragma omp critical (hasher)
      {
	UTAG = TheLex.hash( UNKNOWN );
      }
    }
    if ( no_words == 0 ) {
      //    cerr << "ERROR: empty sentence?!" << endl;
      return false;
    }
    else {
      for ( const auto& cur_word : Words ){
#pragma omp critical (hasher)
	{
	  cur_word->the_word_index = TheLex.hash(cur_word->the_word );
	}
	// look up ambiguous tag in the dictionary
	//
	const auto it = lex.find( cur_word->the_word );
	if ( it != lex.end() ){
#pragma omp critical (hasher)
	  {
	    cur_word->word_amb_tag = TheLex.hash( it->second );
	  }
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

  int sentence::classify_hapax( const UnicodeString& word,
				UnicodeHash& TheLex ) const{
    UnicodeString hap = "HAPAX-";
    if ( word.indexOf( "-" ) != -1 ){
      // hyphen anywere
      hap += 'H';
    }
    if ( u_isupper( word[0] ) ){
      // Capitalized first letter?
      hap += 'C';
    }
    for ( int i=0; i < word.length(); ++i ){
      if ( u_isdigit( word[i] ) ) {
	// digit anywhere
	hap += 'N';
	break;
      }
    }
    if ( hap.length() == 6 ){
      hap += "0";
      //      cerr << "classified HAPAX-0 for: '" << word << "'" << endl;
    }
    int result = -1;
#pragma omp critical (hasher)
    {
      result = TheLex.hash( hap );
    }
    return result;
  }

  bool sentence::nextpat( MatchAction& Action, vector<int>& Pat,
			  UnicodeHash& wordlist, UnicodeHash& TheLex,
			  unsigned int position, int *old_pat ) const {
    Pat.clear();
    // safety check:
    //
    if ( no_words == 0 || position >= no_words ){
      return false;
    }
    word *current_word = Words[position];
    size_t CurWLen = current_word->the_word.length();
    const PatTemplate *aTemplate;
    word* wPtr;
    unsigned int tok;
    // is the present pattern for a known or unknown word?
    //
    if ( Action == MakeKnown ){
      aTemplate = &Ktemplate;
    }
    else if ( Action == MakeUnknown ){
      aTemplate = &Utemplate;
    }
    else if ( current_word->word_amb_tag == UTAG ){
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
      for ( size_t j = 0; j < (size_t)aTemplate->numprefix; ++j ) {
	UnicodeString addChars = "_";
	if ( j < CurWLen ) {
	  addChars += current_word->the_word[j];
	}
	else {
	  addChars += '=';  // "_=" denotes "no value"
	}
#pragma omp critical (hasher)
	{
	  Pat.push_back( TheLex.hash( addChars ) );
	}
      }
    }

    for ( unsigned int i = 0, c_pos = position - aTemplate->word_focuspos;
	  i < aTemplate->wordslots;
	  ++i, ++c_pos ) {
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
	  if ( wordlist.num_of_entries() == 0 ){
	    Pat.push_back( wPtr->the_word_index );
	  }
	  else {
	    tok = wordlist.lookup(  wPtr->the_word );
	    //cerr << "known word Lookup(" << wPtr->the_word << ") gave " << tok << endl;
	    if ( tok ){
	      Pat.push_back( wPtr->the_word_index );
	    }
	    else {
	      Pat.push_back( classify_hapax( wPtr->the_word, TheLex ) );
	    }
	  }
	  break;
	}
      }
      else {   // Out of context.
#pragma omp critical (hasher)
	{
	  Pat.push_back( TheLex.hash( DOT ) );
	}
      }
    } // i

    // Lexical and Context features ?
    //
    for ( unsigned int ii = 0, cc_pos = position - aTemplate->focuspos;
	  ii < aTemplate->numslots;
	  ++ii, ++cc_pos ) {

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
	  if ( old_pat == 0 ){
	    Pat.push_back( wPtr->word_ass_tag );
	  }
	  else {
	    // cerr << "bekijk old pat = " << position+ii-aTemplate->focuspos
	    //  << " - " << old_pat[position+ii-aTemplate->focuspos] << endl;
	    Pat.push_back( old_pat[position+ii-aTemplate->focuspos] );
	  }
	  break;
	case 'f':
	  Pat.push_back( wPtr->word_amb_tag );
	  break;
	case 'F':
	  break;
	case 'a':
	  Pat.push_back( wPtr->word_amb_tag );
	  break;
	}
      }
      else {   // Out of context.
#pragma omp critical (hasher)
	{
	  Pat.push_back( TheLex.hash( DOT ) );
	}
      }
    } // i

    // Suffix?
    //
    if (aTemplate->numsuffix > 0) {
      for ( size_t j = aTemplate->numsuffix; j > 0; --j ) {
	UnicodeString addChars = "_";
	if ( j <= CurWLen ){
	  addChars  += current_word->the_word[CurWLen - j];
	}
	else {
	  addChars += '=';
	}
#pragma omp critical (hasher)
	{
	  Pat.push_back( TheLex.hash( addChars ) );
	}
      }
    }

    // Hyphen?
    //
    if (aTemplate->hyphen) {
      UnicodeString addChars;
      if ( current_word->the_word.indexOf('-') != -1 ){
	addChars = "_H";
      }
      else {
	addChars = "_0";
      }
#pragma omp critical (hasher)
      {
	Pat.push_back( TheLex.hash( addChars ) );
      }
    }

    // Capital (First Letter)?
    //
    if ( aTemplate->capital ) {
      UnicodeString addChars = "_";
      if ( u_isupper(current_word->the_word[0]) ){
	addChars += 'C';
      }
      else {
	addChars += '0';
      }
#pragma omp critical (hasher)
      {
	Pat.push_back( TheLex.hash( addChars ) );
      }
    }

    // Numeric (somewhere in word)?
    //
    if ( aTemplate->numeric ) {
      UnicodeString addChars = "_0";
      for ( unsigned int j = 0; j < CurWLen; ++j ) {
	if ( u_isdigit(current_word->the_word[j]) ){
	  addChars = "_N";
	  break;
	}
      }
#pragma omp critical (hasher)
      {
	Pat.push_back( TheLex.hash( addChars ) );
      }
    }
    return true;
  }

  void sentence::assign_tag( int cat, unsigned int pos ){
    // safety check:
    //
    if ( no_words > 0 && pos < no_words ){
      Words[pos]->word_ass_tag = cat;
    }
  }

  bool sentence::known( unsigned int i ) const {
    if ( no_words > 0 && i < no_words ){
      return Words[i]->word_amb_tag != UTAG;
    }
    else {
      return false;
    }
  }

  bool sentence::read( istream &infile,
		       input_kind_type kind,
		       const UnicodeString& eom,
		       const UnicodeString& seps,
		       size_t& line_no ){
    if ( !infile ) {
      return false;
    }
    InternalEosMark = eom;
    //    cerr << "READ zet InternalEosMark = " << eom << endl;
    if ( kind == TAGGED ){
      return read_tagged( infile, seps, line_no );
    }
    else if ( kind == UNTAGGED ){
      return read_untagged( infile, seps, line_no );
    }
    else {
      return read_enriched( infile, seps, line_no );
    }
  }

  bool sentence::read_tagged( istream &infile,
			      const UnicodeString& seps,
			      size_t& line_no ){
    // read a whole sentence from a stream
    // A sentence can be delimited either by an Eos marker or EOF.
    clear();
    UnicodeString line;
    while ( TiCC::getline( infile, line ) ){
      ++line_no;
      //cerr << "read line: " << line << endl;
      line.trim();
      if ( line.isEmpty() ){
	if ( InternalEosMark == "EL" ){
	  return true;
	}
	continue;
      }
      else if ( Utt_Terminator( line ) ){
	return true;
      }
      vector<UnicodeString> parts = TiCC::split_at_first_of( line, seps );
      if ( parts.size() != 2 ){
#pragma omp critical (errors)
	{
	  cerr << endl << "error in line " << line_no << " : '"
	       << line << "' (skipping it)" << endl;
	  if ( parts.size() == 1 ){
	    cerr << "missing a tag ? " << endl;
	  }
	  else {
	    cerr << "extra values found " << endl;
	  }
	}
      }
      else {
	add( parts[0].trim(), parts[1].trim() );
      }
    }
    //    cerr << "read a sentence: " << *this << endl;
    return true;
  }

  bool sentence::read_untagged( istream &infile,
				const UnicodeString& seps,
				size_t& line_no ){
    // read a whole sentence from a stream
    // A sentence can be delimited either by an Eos marker or EOF.
    clear();
    //    cerr << "untagged-read remainder='" << remainder << "'" << endl;
    UnicodeString line = remainder;
    remainder.remove();
    while ( !line.isEmpty() || TiCC::getline( infile, line ) ){
      ++line_no;
      //      cerr << "untagged-read line: " << line << endl;
      line.trim();
      if ( line.isEmpty() ){
	if ( InternalEosMark == "EL" ){
	  return true;
	}
	continue;
      }
      vector<UnicodeString> parts = TiCC::split_at_first_of( line, seps );
      line = "";
      bool terminated = false;
      for ( const auto& p : parts ){
	//	cerr << "bekijk " << p << endl;
	if ( Utt_Terminator( p ) ){
	  terminated = true;
	}
	else if ( terminated ){
	  remainder += p + " ";
	}
	else {
	  add( p, "" );
	}
      }
      if ( terminated || InternalEosMark == "NL" ){
	return true;
      }
    }
    return true;
  }

  bool sentence::read_enriched( istream &infile,
				const UnicodeString& seps,
				size_t& line_no ){
    // read a sequence of enriched and tagged words from infile
    // every word must be a one_liner
    // cleanup the sentence for re-use...
    clear();
    UnicodeString line;
    while( TiCC::getline( infile, line ) ){
      ++line_no;
      line.trim();
      if ( line.isEmpty() ){
	if ( InternalEosMark == "EL" ){
	  return true;
	}
	continue;
      }
      else if ( Utt_Terminator( line ) ){
	return true;
      }
      vector<UnicodeString> extras = TiCC::split_at_first_of( line, seps );
      if ( extras.size() >= 2 ){
	UnicodeString Word = extras.front();
	extras.erase(extras.begin()); // expensive, but allas. extras is small
	UnicodeString Tag = extras.back();
	extras.pop_back();
	if ( !Word.isEmpty() && !Tag.isEmpty() ){
	  add( Word, extras, Tag );
	}
      }
    };
    return no_words > 0;
  }

} // namespace
