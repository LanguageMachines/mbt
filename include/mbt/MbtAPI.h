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

#ifndef MBT_API_H
#define MBT_API_H

#include "timbl/TimblAPI.h"
#include "mbt/Tagger.h"

using namespace Timbl;
using namespace Tagger;

class MbtAPI {
 public:
  static bool GenerateTagger( int, char** );
  static bool RunTagger( int, char** );
  MbtAPI( const std::string& );
  ~MbtAPI();
  std::string Tag( const std::string& );
 private:
  TaggerClass *tagger;
};

#endif
