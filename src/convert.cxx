/*
  Copyright (c) 1998 - 2024
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

#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <cstdlib>

using std::ifstream;
using std::ofstream;
using std::istream;
using std::ostream;
using std::cerr;
using std::endl;
using std::ws;
using std::map;
using std::string;

typedef  map<string, string> string_map;

void fill_map( istream& in, string_map& map ){
  string tag, word;
  int cnt=0;
  while ( in ){
    in >> tag;
    in >> word;
    in >> ws;
    string::size_type pos = word.find("-");
    while ( pos != string::npos ){
      word.replace( pos, 1, ";" );
      pos = word.find( "-", pos );
    }
    map[tag] = word;
    ++cnt;
  }
  cerr << cnt << " tags read " << endl;
}

void backup( const string& in_name, const string& out_name ){
  ifstream in( in_name );
  if ( !in ){
    cerr << "problems with inputfile: " << in_name << endl;
    exit(1);
  }
  ofstream out( out_name );
  if ( !out ){
    cerr << "problems with opening backup file: " << out_name << endl;
    exit(1);
  }
  string line;
  while( getline( in, line ) ){
    out << line << endl;
  }
  cerr << "made a backup copy of " << in_name << " (" << out_name
       << ")" << endl;
}

void convert( const string& in_name, const string& out_name,
	      string_map& tmap ){
  ifstream in( in_name );
  if ( !in ){
    cerr << "problems with inputfile: " << in_name << endl;
    exit(1);
  }
  ofstream out( out_name );
  if ( !out ){
    cerr << "problems with opening output file: " << out_name << endl;
    exit(1);
  }
  int ch;
  string line;
  while ( (ch=in.peek(), ch=='#' ) ){
    getline( in, line );
    out << line << endl;
  }
  while ( getline( in, line ) ){
    string::size_type pos = line.find_first_not_of("(){}, []");
    while ( pos != string::npos ){
      string::size_type epos = line.find_first_of( "(){}, []", pos );
      string tag = line.substr( pos, epos-pos );
      cerr << "zoek tag = " << tag << endl;
      string_map::const_iterator it = tmap.find( tag );
      if ( it != tmap.end() ){
	line.replace( pos, epos-pos, it->second );
	pos = line.find_first_not_of("(){}, []", pos+it->second.length());
      }
      else
	pos = line.find_first_not_of("(){}, []", epos);
    }
    out << line << endl;
  }
  cerr << "converted " << out_name << endl;
}

void usage(){
  cerr << "usage: convert tag_file tree_file" << endl;
  cerr << "the tagfile is the reversed index as used in old Mbt" << endl;
  cerr << "    (given by the -r option)" << endl;
  cerr << "this file contains a series of tag word combinations like"
       << " AF Prep" << endl;
  cerr << "treefile is the known, resp. unknow InstanceBase file" << endl;
  cerr << "     (as given by the -k and -u options) " << endl;
}

int main( int argc, char *argv[] ){
  if ( argc < 3 ){
    usage();
    exit(1);
  }
  ifstream tag_file( argv[1] );
  if ( !tag_file  ){
    cerr << "problems with " << argv[1] << endl;
    exit(1);
  }
  std::map<string, string> tag_map;
  fill_map( tag_file, tag_map );
  string tree_file_name = argv[2];
  string back_name = tree_file_name + ".bck";
  backup( tree_file_name, back_name );
  convert( back_name, tree_file_name, tag_map );
}
