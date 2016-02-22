/*
  Copyright (c) 1998 - 2016
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

#include "ticcutils/Trie.h"
#include "timbl/Common.h"

namespace Tagger {
  using Tries::Trie;

  // a Tagged Lexion. Stores strings , frequencies and assigned tags
  class TagInfo {
    friend std::ostream& operator<<( std::ostream&, TagInfo * );
  public:
    TagInfo( const std::string& , const std::string& );
    ~TagInfo();
    void Update( const std::string& s );
    void Prune( int perc );
    int Freq() const { return WordFreq; };
    std::string stringRep() { return StringRepr; };
    void CreateStringRepr();
    std::string DisplayTagFreqs() const;
    std::string Word;
  private:
    int WordFreq;
    std::string StringRepr;
    std::map<std::string, int> TagFreqs;
  };

  class TagLex {
    friend std::ostream& operator<< ( std::ostream&, TagLex * );
  public:
    TagLex();
    ~TagLex();
    TagInfo *Lookup( const std::string& s );
    TagInfo *Store( const std::string&  , const std::string&  );
    std::vector<TagInfo *> CreateSortedVector();
    int numOfLexiconEntries() const { return NumOfEntries; };
  private:
    TagLex( const TagLex& ); // inhibit copies
    TagLex& operator=( const TagLex& ); // inhibit copies
    Trie<TagInfo> *TagTree;
    int NumOfEntries;
  };

}
#endif
