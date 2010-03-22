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
#ifndef TAGGER_H
#define TAGGER_H

namespace Tagger {
  class TaggerClass;
  int MakeTagger( Timbl::TimblOpts& );
  int RunTagger( Timbl::TimblOpts& );
  TaggerClass *CreateTagger( Timbl::TimblOpts& );
  bool setLog( LogStream& );
  void RemoveTagger( TaggerClass * );
  std::string Tag( TaggerClass*, const std::string& );
}

#endif
  
