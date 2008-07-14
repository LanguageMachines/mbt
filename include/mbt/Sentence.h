/*
  Copyright (c) 1998 - 2008
  ILK  -  Tilburg University
  CNTS -  University of Antwerp
 
  This file is part of Mbt3

  Mbt3 is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mbt3 is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, see <http://www.gnu.org/licenses/>.

  For questions and suggestions, see:
      http://ilk.uvt.nl/software.html
  or send mail to:
      Timbl@uvt.nl
*/
#ifndef SENTENCE_H
#define SENTENCE_H

#include "timbl/Tree.h"

namespace Tagger {
  using Hash::Lexicon;
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
  public:
    sentence();
    ~sentence();

    void reset( const std::string& );    
    bool Fill( const std::string&, bool );
    void print( std::ostream & ) const;
    bool init_windowing( PatTemplate *, PatTemplate *, Lexicon&, StringHash& );
    bool nextpat( MatchAction *, std::vector<int>&, StringHash& , StringHash&,
		  unsigned int, int * = 0 );
    int classify_hapax( const std::string&, StringHash& );
    void assign_tag( int, unsigned int );
    std::string getword( unsigned int i ) { return Words[i]->the_word; };
    word *getWord( unsigned int i ) const { return Words[i]; };
    const std::string& gettag( int i ) const { return Words[i]->word_tag; };
    std::string getenr( unsigned int i );
    int No_Words() const { return no_words; };
    bool known( unsigned int );
    std::string Eos() const;
    bool read( std::istream &, input_kind_type );
    bool read( int, input_kind_type );
  private:
    int UTAG;
    std::vector<word *> Words;
    PatTemplate * Ktemplate;
    PatTemplate * Utemplate;
    unsigned int no_words;
    std::string InternalEosMark;
    bool Utt_Terminator( const std::string& );
    word_stat get_word( std::istream& is, std::string& Word );
    void add( const std::string&, const std::vector<std::string>&,
	      const std::string& );
    void add( const std::string&, const std::string& );
    bool read( std::istream &infile, bool );
    bool read( std::istream &infile );
    bool read( int, bool tagged );
  };

}
#endif
