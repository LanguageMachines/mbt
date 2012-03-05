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

#ifndef MBT_API_H
#define MBT_API_H

#include "timbl/TimblAPI.h"
#include "timbl/LogStream.h"
#include "mbt/Tagger.h"

using namespace Timbl;
using namespace Tagger;

class MbtAPI {
 public:
  static bool GenerateTagger( TimblOpts& );
  static bool GenerateTagger( int, char** );
  static bool GenerateTagger( const std::string& );
  static bool RunTagger( int, char** );
  MbtAPI( const std::string& );
  MbtAPI( const std::string&, LogStream& );
  ~MbtAPI();
  bool isInit() const;
  std::string Tag( const std::string& );
  std::vector<TagResult> TagLine( const std::string& );
  std::string getResult( const std::vector<TagResult>& ) const;
 private:
  TaggerClass *tagger;
};

#endif
