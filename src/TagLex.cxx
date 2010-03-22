/*
  Copyright (c) 1998 - 2010
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

#include <algorithm>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <string>

#include "timbl/StringOps.h"
#include "mbt/TagLex.h"

namespace Tagger {
  using namespace Timbl;
  using namespace std;
  
  TagInfo::TagInfo( const string& name, const string& tag ){
    Word = name;
    WordFreq = 1;
    TF = new TagFreq();
    TF->Update( tag );
    StringRepr = "";
  }
  
  TagInfo::~TagInfo(){
    delete TF;
  }
  
  void TagInfo::Update( const string& tag ){
    WordFreq++;
    TF->Update( tag );
  }
  
  TagFreqList::~TagFreqList(){
    if ( this->next )
      delete this->next;
  }
  
  void TagFreq::Update( const string& tag ){
    TagFreqList *tmp, **pnt = &Tags;
    while( *pnt ){
      if ( strcmp( (*pnt)->tag.c_str(), tag.c_str() ) > 0 ){
	tmp = *pnt;
	*pnt = new TagFreqList( tag );
	(*pnt)->next = tmp;
	return;
      }
      else if ( strcmp( (*pnt)->tag.c_str(), tag.c_str() ) == 0 ){
	(*pnt)->freq++;
	return;
      }
      else {
	pnt = &((*pnt)->next);
      }
    }
    *pnt = new TagFreqList( tag );
  }
  
  void TagFreq::Prune( int Treshold, int TotalWords ){
    double perc;
    TagFreqList *tmp, **pnt = &Tags, *aside = NULL;
    bool first = true;
    while( *pnt ){
      perc = ( (double)(*pnt)->freq * 100 ) / (double)TotalWords;
      if ( perc < Treshold ){
	tmp = *pnt;
	*pnt = (*pnt)->next;
	tmp->next = NULL;
	if ( first ){
	  first = false;
	  aside = tmp;
	}
	else {
	  delete tmp;
	}
      }
      else
	pnt = &((*pnt)->next);
    }
    if ( Tags == NULL )
      Tags = aside;
    else
      delete aside;
  }
  
  int cmp_freq( const TagFreqList *p1, const TagFreqList *p2 ){
    return ( p2->freq < p1->freq );
  }
  
  void TagFreq::FreqSort( ){
    vector<TagFreqList *> TagsArray;
    TagFreqList *pnt = Tags;
    while ( pnt ){
      TagsArray.push_back( pnt );
      pnt = pnt->next;
    }
    if ( TagsArray.size() > 1 ) {
      sort( TagsArray.begin(), TagsArray.end(), cmp_freq );
      for ( unsigned int i = 0; i < TagsArray.size()-1; ++i ){
	TagsArray[i]->next = TagsArray[i+1];
      }
      TagsArray[TagsArray.size()-1]->next = 0;
    }
    Tags = TagsArray[0];
  }
  
  ostream& operator<<( ostream& os, TagInfo *LI ){
    if ( LI ){
      os << " " << LI->Word << ":" << LI->WordFreq
	 << " " << LI->TF << " " << LI->StringRepr;
    }
    return os;
  }
  
  ostream& operator<<( ostream& os, TagFreqList *TFL ){
    TagFreqList *pnt = TFL;
    while ( pnt ) {
      os << pnt->tag << ":" << pnt->freq << " ";
      pnt = pnt->next;
    }
    return os;
  }
  
  ostream& operator<<( ostream& os, TagFreq *TF ){ 
    os << " { " << TF->Tags << "}";
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
  
  void StoreInArray( TagInfo *TI, void *arg ){
    TagInfo **TA = (TagInfo **)arg;
    static int Pos = 0;
    TA[Pos++] = TI;
  }
  
  int cmp_info( const void *p1, const void *p2 ){
    TagInfo * const *t1, * const *t2;
    t1 = (TagInfo * const *)p1;
    t2 = (TagInfo * const *)p2;
    int diff = (*t2)->Freq() - (*t1)->Freq();
    if ( diff == 0 ){
      if ( compare_nocase( (*t2)->Word, (*t1)->Word ) )
	diff = strcmp( (*t1)->Word.c_str(), (*t2)->Word.c_str() );
      else
	diff = strcmp( (*t2)->Word.c_str(), (*t1)->Word.c_str() );
    }
    return diff;
  }
  
  TagInfo **TagLex::CreateSortedArray(){
    TagInfo **TagArray = new TagInfo *[NumOfEntries];
    TagTree->ForEachDo( StoreInArray, (void *)TagArray );
    qsort( TagArray, NumOfEntries, sizeof(TagInfo *), cmp_info );
    return TagArray;
  }
  
  ostream& operator<<( ostream& os, TagLex *L ){
    return os << L->TagTree; }
  
}
