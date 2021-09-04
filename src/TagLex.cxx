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
#include <map>
#include <vector>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <string>

#include "ticcutils/StringOps.h"
#include "ticcutils/Unicode.h"
#include "mbt/TagLex.h"

namespace Tagger {
  using namespace std;
  using namespace icu;

  TagInfo::TagInfo( const UnicodeString& word,
		    const UnicodeString& tag ):
    Word(word), WordFreq(0) {
    Update( tag );
  }

  TagInfo::~TagInfo(){
  }

  void TagInfo::Update( const UnicodeString& tag ){
    ++WordFreq;
    ++TagFreqs[tag];
  }

  void TagInfo::Prune( int Threshold ){
    auto it = TagFreqs.begin();
    while ( it != TagFreqs.end() ){
      double perc = ( (double)it->second * 100 ) / ( double)WordFreq;
      if ( perc < Threshold ){
	TagFreqs.erase( it++ );
      }
      else {
	++it;
      }
    }
  }

  UnicodeString TagInfo::DisplayTagFreqs( )const {
    UnicodeString result;
    for( const auto& it : TagFreqs ){
      result += it.first;
      result += ":";
      result += TiCC::UnicodeFromUTF8(TiCC::toString(it.second));
      result += " ";
    }
    return result;
  }

  struct FS {
    FS( int f, const UnicodeString& s ):freq(f), str(s) {};
    int freq;
    UnicodeString str;
  };

  int cmpFreq( const FS& p1, const FS& p2 ){
    return ( p2.freq < p1.freq );
  }

  void TagInfo::CreateStringRepr(){
    vector<FS> FreqTags;
    for ( const auto& it : TagFreqs ){
      FreqTags.push_back( FS( it.second, it.first) );
    }
    sort( FreqTags.begin(), FreqTags.end(), cmpFreq );
    UnicodeString tmpstr;
    for ( auto const& it2 : FreqTags ){
      tmpstr += it2.str;
      if ( &it2 != &FreqTags.back() ){
	tmpstr += ";";
      }
    }
    StringRepr = tmpstr;
  }

  ostream& operator<<( ostream& os, TagInfo *LI ){
    if ( LI ){
      os << " " << LI->Word << ":" << LI->WordFreq
	 << " {" << LI->DisplayTagFreqs() << "} " << LI->StringRepr;
    }
    return os;
  }

  TagLex::TagLex(){
    TagTree = new Trie<TagInfo>;
    NumOfEntries = 0;
  }

  TagLex::~TagLex(){
    delete TagTree;
  }

  TagInfo *TagLex::Lookup( const UnicodeString& name ){
    return reinterpret_cast<TagInfo *>(TagTree->Retrieve( TiCC::UnicodeToUTF8(name) ));
  }

  TagInfo *TagLex::Store( const UnicodeString& name,
			  const UnicodeString& tag ){
    TagInfo *info = TagTree->Retrieve( TiCC::UnicodeToUTF8(name) );
    if ( !info ){
      NumOfEntries++;
      info = new TagInfo( name, tag );
      return TagTree->Store( TiCC::UnicodeToUTF8(name), info );
    }
    else {
      info->Update( tag );
    }
    return info;
  }

  void StoreInVector( TagInfo *TI, void *arg ){
    vector<TagInfo*> *vec = (vector<TagInfo*> *)arg;
    vec->push_back( TI );
  }

  bool ascendingInfo( const TagInfo* t1, const TagInfo* t2 ){
    //
    // sort on decending frequency
    // when same frequency, sort alphabetical
    // but: sort Uppercase words before lowercase when equal (e.g Land/land)
    //
    int diff = t2->Freq() - t1->Freq();
    if ( diff == 0 ){
      UnicodeString u1 = t1->Word;
      u1.toLower();
      UnicodeString u2 = t2->Word;
      u2.toLower();
      if ( u2 == u1 ){
	return t2->Word < t1->Word;
      }
      else {
	return t1->Word < t2->Word;
      }
    }
    return diff < 0;
  }

  vector<TagInfo *> TagLex::CreateSortedVector(){
    vector<TagInfo*> TagVec;
    TagTree->ForEachDo( StoreInVector, (void *)&TagVec );
    sort( TagVec.begin(), TagVec.end() , ascendingInfo );
    return TagVec;
  }

  ostream& operator<<( ostream& os, TagLex *L ){
    return os << L->TagTree; }

}
