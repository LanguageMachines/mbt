/*
  Copyright (c) 1998 - 2009
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
#ifndef PATTERN_H
#define PATTERN_H

class PatTemplate {
 public:
  std::string templatestring;
  std::string word_templatestring;
  size_t tlen;
  size_t numslots;
  size_t wordslots;
  int focuspos;
  int word_focuspos;
  int skipfocus;
  int numsuffix;
  int numprefix;
  int hyphen;
  int capital;
  int numeric;
  int compensation;
  int wordfocus;
  
  // todo:
  // int Cap1 : Cap1, CAPH, CAP1H
  // int non_alfanum
  // int num

  // int wordlength
  // 

  PatTemplate();
  ~PatTemplate(){;}

  int totalslots();
  int word_totalslots();
  bool set( const std::string& );
  size_t sprint( std::string& );
};

#endif
