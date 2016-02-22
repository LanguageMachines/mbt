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

#ifndef MBT_API_H
#define MBT_API_H

#include "timbl/TimblAPI.h"
#include "ticcutils/LogStream.h"
#include "mbt/Tagger.h"

class MbtAPI {
 public:
  static bool GenerateTagger( int, char** );
  static bool GenerateTagger( const std::string& );
  static bool RunTagger( int, char** );
  MbtAPI( const std::string& );
  MbtAPI( const std::string&, TiCC::LogStream& );
  ~MbtAPI();
  bool isInit() const;
  std::string Tag( const std::string& );
  std::vector<Tagger::TagResult> TagLine( const std::string& );
  std::string getResult( const std::vector<Tagger::TagResult>& ) const;
  std::string set_eos_mark( const std::string& );
 private:
  MbtAPI( const MbtAPI& ); // inhibit copies
  MbtAPI& operator=( const MbtAPI& ); // inhibit copies
  Tagger::TaggerClass *tagger;
};

#endif
