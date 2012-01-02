/*
  $Id$
  $URL$

  Copyright (c) 1998 - 2012
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
#ifndef TAGLEX_H
#define TAGLEX_H

#include "timbl/Trie.h"
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
    Trie<TagInfo> *TagTree;
    int NumOfEntries;
  };
  
}
#endif
