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
#ifndef TAGLEX_H
#define TAGLEX_H

#include "timbl/Trie.h"
#include "timbl/Common.h"

namespace Tagger {
  using Tries::Trie;

const int MAX_TAGS = 200;

class TagFreqList {
  friend std::ostream& operator<<( std::ostream&, TagFreqList * );
 public:
  TagFreqList( const std::string& s ) { tag = s; freq=1; next = NULL; };
  ~TagFreqList();
  std::string tag;
  int freq;
  TagFreqList *next;
};

class TagFreq {
  friend std::ostream& operator<<( std::ostream&, TagFreq * );
 public:
  TagFreq( ) { Tags = NULL; };
  ~TagFreq() { delete Tags; };
  void Update( const std::string&  );
  void Prune( int, int );
  void FreqSort( );
  TagFreqList *Tags;
};

// a Tagged Lexion. Stores strings , frequencies and assigned tags
class TagInfo {
  friend std::ostream& operator<<( std::ostream&, TagInfo * );
 public:
  TagInfo( const std::string& , const std::string&  );
  ~TagInfo();
  void Update( const std::string&  );
  int Freq() const { return WordFreq; };
  void Prune( int perc ) { TF->Prune( perc, WordFreq ); };
  std::string Word;
  int WordFreq;
  TagFreq *TF;
  std::string StringRepr;
};

class TagLex {
  friend std::ostream& operator<< ( std::ostream&, TagLex * );
 public:
  TagLex();
  ~TagLex();
  TagInfo *Lookup( const std::string& s );
  TagInfo *Store( const std::string&  , const std::string&  );
  TagInfo **CreateSortedArray();
  Trie<TagInfo> *TagTree;
  int NumOfEntries;
};

}
#endif
