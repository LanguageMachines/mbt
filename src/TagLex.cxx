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

#include <algorithm>
#include <map>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <string>

#include "timbl/StringOps.h"
#include "timbl/Types.h"
#include "mbt/TagLex.h"

namespace Tagger {
  using namespace Timbl;
  using namespace std;
  
  TagInfo::TagInfo( const string& name, const string& tag ){
    Word = name;
    WordFreq = 0;
    Update( tag );
  }
  
  TagInfo::~TagInfo(){
  }
  
  void TagInfo::Update( const string& tag ){
    ++WordFreq;
    map<string,int>::iterator it = TagFreqs.find( tag );
    if ( it != TagFreqs.end() ){
      ++it->second;
    }
    else {
      TagFreqs[tag] = 1;
    }
  }
  
  void TagInfo::Prune( int Threshold ){
    map<string,int>::iterator it = TagFreqs.begin();
    while ( it != TagFreqs.end() ){
      double perc = ( (double)it->second * 100 ) / ( double)WordFreq;
      if ( perc < Threshold )
	TagFreqs.erase( it++ );
      else
	++it;
    }
  }
  
  string TagInfo::DisplayTagFreqs( )const {
    string result;
    map<string, int>::const_iterator it = TagFreqs.begin();
    while ( it != TagFreqs.end() ){
      result += it->first + ":" + toString(it->second) + " ";
      ++it;
    }
    return result;
  }

  struct FS {
    FS( int f, string s ):freq(f), str(s) {};
    int freq;
    string str;
  };

  int cmpFreq( const FS& p1, const FS& p2 ){
    return ( p2.freq < p1.freq );
  }

  void TagInfo::CreateStringRepr(){
    vector<FS> FreqTags;
    map<string,int>::const_iterator it = TagFreqs.begin();
    while ( it != TagFreqs.end() ){
      FreqTags.push_back( FS( it->second, it->first) );
      ++it;
    }
    sort( FreqTags.begin(), FreqTags.end(), cmpFreq );
    string tmpstr;
    vector<FS>::const_iterator it2 = FreqTags.begin();
    while ( it2 != FreqTags.end() ){
      tmpstr += it2->str;
      ++it2;
      if ( it2 != FreqTags.end() )
	tmpstr += ";";
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
  
  TagInfo *TagLex::Lookup( const string& name ){
    return (TagInfo *)TagTree->Retrieve( name ); 
  }
  
  TagInfo *TagLex::Store( const string& name, const string& tag ){
    TagInfo *info = TagTree->Retrieve( name );
    if ( !info ){
      NumOfEntries++;
      info = new TagInfo( name, tag );
      return TagTree->Store( name, info );
    }
    else 
      info->Update( tag );
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
      if ( compare_nocase( t2->Word, t1->Word ) )
	return strcmp( t2->Word.c_str(), t1->Word.c_str() ) < 0;
      else
	return strcmp( t1->Word.c_str(), t2->Word.c_str() ) < 0;
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
