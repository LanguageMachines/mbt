/*
  Copyright (c) 1998 - 2022
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
#ifndef MBT_TAGLEX_H
#define MBT_TAGLEX_H

#include "ticcutils/UniTrie.h"
#include "ticcutils/Unicode.h"
#include "timbl/Common.h"

namespace Tagger {

  // a Tagged Lexion. Stores strings , frequencies and assigned tags
  class TagInfo {
    friend std::ostream& operator<<( std::ostream&, TagInfo * );
  public:
    TagInfo( const icu::UnicodeString& ,
	     const icu::UnicodeString& );
    ~TagInfo();
    void Update( const icu::UnicodeString& s );
    void Prune( int perc );
    int Freq() const { return WordFreq; };
    icu::UnicodeString stringRep() { return StringRepr; };
    void CreateStringRepr();
    icu::UnicodeString DisplayTagFreqs() const;
    icu::UnicodeString Word;
  private:
    int WordFreq;
    icu::UnicodeString StringRepr;
    std::map<icu::UnicodeString, int> TagFreqs;
  };

  class TagLex {
    friend std::ostream& operator<< ( std::ostream&, TagLex * );
  public:
    TagLex();
    ~TagLex();
    TagInfo *Lookup( const icu::UnicodeString& );
    TagInfo *Store( const icu::UnicodeString&,
		    const icu::UnicodeString&  );
    std::vector<TagInfo *> CreateSortedVector();
    int numOfLexiconEntries() const { return NumOfEntries; };
  private:
    TagLex( const TagLex& ); // inhibit copies
    TagLex& operator=( const TagLex& ); // inhibit copies
    Tries::UniTrie<TagInfo> *TagTree;
    int NumOfEntries;
  };

}
#endif
