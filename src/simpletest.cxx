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

#include <cstdlib>
#include "mbt/MbtAPI.h"
using namespace std;
using namespace Tagger;

int main(){
  string path;
  const char *ev = getenv( "topsrcdir" );
  if ( ev ){
    path = ev;
  }
  else {
    path = ".";
  }
  string command = "-T " + path + "/example/eindh.data -s ./simple.setting";
  MbtAPI::GenerateTagger( command );
  MbtAPI demo( "-s ./simple.setting" );
  cerr << demo.Tag( "dit is een test" ) << endl;
  vector<TagResult> v = demo.TagLine( "Test regel 2 ." );
  assert( v[0].assignedTag() == "N" );
  assert( v[2].confidence() == -1 );
}
