/*
  Copyright (c) 1998 - 2026
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

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <cstring>
#include <cctype>
#include <cassert>
#include <string>

#include "mbt/Pattern.h"

using std::string;
using std::cerr;
using std::endl;

PatTemplate::PatTemplate()
{
  numslots = 0;
  wordslots = 0;
  focuspos =-1;
  word_focuspos = 0;
  numsuffix= 0;
  numprefix= 0;
  skipfocus= 0;
  hyphen   = 0;
  capital  = 0;
  numeric  = 0;
  compensation = 0;
  wordfocus = 0;
}

bool PatTemplate::set( const string& tempstr ){
  /// reads a format string and figures out a template for the patterns
  /*!
    \param tempstr the template string to parse

    this function uses a template string to fill the PatTemplate. It determines
    the focus postion
  */
  int j = 0;
  int k = 0;
  bool focus = false;
  compensation = 0;
  for ( const auto& c : tempstr ){
    switch( c ){
    case 'f':
      if ( focus ){
	cerr << "more than 1 focus position in Pattern! " << tempstr << endl;
	return false;
      }
      focuspos = j;
      skipfocus=0;
      templatestring += c;
      word_templatestring += c;
      word_focuspos = k;
      ++numslots;
      ++j;
      ++wordslots;
      ++compensation;
      ++k;
      focus = true;
      break;
    case 'F':
      if ( focus ){
	cerr << "more than 1 focus position in Pattern! " << tempstr << endl;
	return false;
      }
      focuspos = j;
      skipfocus=1;
      templatestring += c;
      word_templatestring += c;
      word_focuspos = k;
      ++numslots;
      ++j;
      ++wordslots;
      ++compensation;
      ++k;
      focus = true;
      break;
    case 'd':
      templatestring += c;
      ++numslots;
      ++j;
      break;
    case 'a':
      templatestring += c;
      ++numslots;
      ++j;
      break;
    case 'p':
      ++numprefix;
      break;
    case 's':
      ++numsuffix;
      break;
    case 'h':
      hyphen = 1;
      break;
    case 'c':
      capital = 1;
      break;
    case 'n':
      numeric = 1;
      break;
    case 'w':
      word_templatestring += c;
      ++wordslots;
      ++k;
      break;
    case 'W':
      //
      // the W is a modifier which changes the 'f' to a
      // w.
      //
      if ( focus ){
	if ( word_templatestring[word_focuspos] == 'f' ||
	     word_templatestring[word_focuspos] == 'F' ) {
	  word_templatestring[word_focuspos] = 'w';
	  --compensation;
	  wordfocus = 1;
	}
	else {
	  cerr << "W modifier not directly after 'f' or 'F' in "
	       << tempstr << endl;
	}
      }
      else {
	// asume W means FW
	focuspos = j;
	skipfocus = 1;
	templatestring += 'F';
	word_templatestring += 'w';
	word_focuspos = k;
	++numslots;
	++j;
	++wordslots;
	++k;
	wordfocus = 1;
	focus = true;
      }
      break;
    default:
      cerr << "ERROR: illegal symbol '" << c
	   << "' in context string'" << endl;
      return false;
    }
  }

  if (focuspos == -1) {
    cerr << "ERROR: no focus in context string." << endl;
    return false;
  }
  return true;
}

int PatTemplate::totalslots() const {
  /// return the total number of slots in the pattern
  return ( numslots + numprefix + numsuffix +
	   hyphen + capital + numeric + word_totalslots());
}


int PatTemplate::word_totalslots() const {
  /// return the number of word slots in the pattern
  // Compensate for the 'f'
  // (hm, what if 'F' or 'W'?).
  //
  return (wordslots-compensation);
}
