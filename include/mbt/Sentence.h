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
#ifndef MBT_SENTENCE_H
#define MBT_SENTENCE_H

#include "ticcutils/Unicode.h"
#include "ticcutils/UniHash.h"

namespace Tagger {
  using Hash::UnicodeHash;

  const icu::UnicodeString DOT = "==";
  const icu::UnicodeString UNKNOWN = "__";
  enum MatchAction { Unknown, Known, MakeKnown, MakeUnknown };

  // A word in a sentence.
  //
  class word {
  public:

    icu::UnicodeString the_word;
    int the_word_index;

    icu::UnicodeString word_tag;
    int word_amb_tag;
    int word_ass_tag;
    std::vector<icu::UnicodeString> extraFeatures;
    word( const icu::UnicodeString&,
	  const icu::UnicodeString& );
    word( const icu::UnicodeString&,
	  const std::vector<icu::UnicodeString>&,
	  const icu::UnicodeString& );
    ~word();

  };

  enum word_stat { NO_MORE_WORDS, LAST_WORD, EOS_FOUND, READ_MORE };
  enum input_kind_type { UNTAGGED, TAGGED, ENRICHED };

  // A sentence (used when windowing).
  //
  class sentence {
    friend std::ostream& operator<< ( std::ostream& , const sentence& );
  public:
    sentence( const PatTemplate&, const PatTemplate& );
    ~sentence();
    void clear();
    bool init_windowing( std::map<icu::UnicodeString,icu::UnicodeString>&, UnicodeHash& );
    bool nextpat( MatchAction&, std::vector<int>&, UnicodeHash& , UnicodeHash&,
		  unsigned int, int * = 0 ) const;
    int classify_hapax( const icu::UnicodeString&, UnicodeHash& ) const;
    void assign_tag( int, unsigned int );
    icu::UnicodeString getword( unsigned int i ) const {
      return Words[i]->the_word;
    };
    icu::UnicodeString& gettag( int i ) const {
      return Words[i]->word_tag;
    };
    const std::vector<icu::UnicodeString>& getEnrichments( unsigned int i ) const {
      return Words[i]->extraFeatures;
    };
    icu::UnicodeString getenr( unsigned int i );
    unsigned int size() const { return no_words; };
    bool known( unsigned int ) const;
    bool read( std::istream &,
	       input_kind_type,
	       const icu::UnicodeString&,
	       const icu::UnicodeString&,
	       size_t& );
  private:
    int UTAG;
    std::vector<word *> Words;
    icu::UnicodeString remainder;
    const PatTemplate& Ktemplate;
    const PatTemplate& Utemplate;
    unsigned int no_words;
    icu::UnicodeString InternalEosMark;
    bool Utt_Terminator( const icu::UnicodeString& );
    void add( const icu::UnicodeString&,
	      const std::vector<icu::UnicodeString>&,
	      const icu::UnicodeString& );
    void add( const icu::UnicodeString&,
	      const icu::UnicodeString& );
    bool read_tagged( std::istream&, const icu::UnicodeString&, size_t& );
    bool read_untagged( std::istream&, const icu::UnicodeString&, size_t& );
    bool read_enriched( std::istream&, const icu::UnicodeString&, size_t& );
    void print( std::ostream & ) const;
  };

  std::ostream& operator<<( std::ostream& os, const sentence& s );
}
#endif
