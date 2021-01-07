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

#include "ticcutils/TreeHash.h"

namespace Tagger {
  using Hash::StringHash;

  const std::string DOT = "==";
  const std::string UNKNOWN = "__";
  enum MatchAction { Unknown, Known, MakeKnown, MakeUnknown };

  // A word in a sentence.
  //
  class word {
  public:

    std::string the_word;
    int the_word_index;

    std::string word_tag;
    int word_amb_tag;
    int word_ass_tag;
    std::vector<std::string> extraFeatures;
    word( const std::string&, const std::string& );
    word( const std::string&, const std::vector<std::string>&, const std::string& );
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
    bool init_windowing( std::map<std::string,std::string>&, StringHash& );
    bool nextpat( MatchAction&, std::vector<int>&, StringHash& , StringHash&,
		  unsigned int, int * = 0 ) const;
    int classify_hapax( const std::string&, StringHash& ) const;
    void assign_tag( int, unsigned int );
    std::string getword( unsigned int i ) { return Words[i]->the_word; };
    const std::string& gettag( int i ) const { return Words[i]->word_tag; };
    const std::vector<std::string>& getEnrichments( unsigned int i )
      const { return Words[i]->extraFeatures; };
    std::string getenr( unsigned int i );
    unsigned int size() const { return no_words; };
    bool known( unsigned int ) const;
    bool read( std::istream &,
	       input_kind_type,
	       const std::string&,
	       const std::string&,
	       size_t& );
  private:
    int UTAG;
    std::vector<word *> Words;
    std::string remainder;
    const PatTemplate& Ktemplate;
    const PatTemplate& Utemplate;
    unsigned int no_words;
    std::string InternalEosMark;
    bool Utt_Terminator( const std::string& );
    void add( const std::string&, const std::vector<std::string>&,
	      const std::string& );
    void add( const std::string&, const std::string& );
    bool read_tagged( std::istream&, const std::string&, size_t& );
    bool read_untagged( std::istream&, const std::string&, size_t& );
    bool read_enriched( std::istream&, const std::string&, size_t& );
    void print( std::ostream & ) const;
  };

  std::ostream& operator<<( std::ostream& os, const sentence& s );
}
#endif
