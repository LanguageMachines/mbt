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
#ifndef MBT_PATTERN_H
#define MBT_PATTERN_H

class PatTemplate {
 public:
  std::string templatestring;
  std::string word_templatestring;
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

  PatTemplate();
  ~PatTemplate(){};

  int totalslots() const;
  int word_totalslots() const;
  bool set( const std::string& );
};

#endif
