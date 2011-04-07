/*
  $Id$
  $URL$

  Copyright (c) 1998 - 2011
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
#ifndef TAGGER_H
#define TAGGER_H

#include "mbt/Pattern.h"
#include "mbt/Sentence.h"
#include "mbt/TagLex.h"
#include "timbl/TimblAPI.h"

namespace Tagger {

  std::string Version();
  std::string VersionName();

  const int EMPTY_PATH = -1000000;

  class n_best_tuple {
  public:
    n_best_tuple(){ path = EMPTY_PATH; tag = EMPTY_PATH; prob = 0.0; }
    void clean(){ path = EMPTY_PATH; tag = EMPTY_PATH; prob = 0.0; };
    int path;
    int tag;
    double prob;
  };

  class BeamData {
  public:
    BeamData();
    ~BeamData();
    bool Init( int, unsigned int );
    void InitPaths( StringHash&, 
		    const Timbl::TargetValue *,
		    const Timbl::ValueDistribution * );
    void NextPath( StringHash&,
		   const Timbl::TargetValue *,
		   const Timbl::ValueDistribution*, 
		   int ); 
    void ClearBest();
    void Shift( int, int );
    void Print( std::ostream& os, int i_word, StringHash& TheLex );
    void PrintBest( std::ostream& os, StringHash& TheLex );
    int size;
    int **paths;
    int **temppaths;
    double *path_prob;
    n_best_tuple **n_best_array;
  };
  
  class TaggerClass{
  public:
    TaggerClass( );
    TaggerClass( const TaggerClass& );
    ~TaggerClass();
    bool InitTagging();
    bool InitLearning();
    bool InitBeaming( unsigned int );
    TaggerClass *clone() const;
    int Run( );
    int TagLine( const std::string&, std::string& );
    std::string Tag( const std::string& );
    int CreateKnown();
    int CreateUnknown();
    void CreateSettingsFile();
    bool set_default_filenames();
    void parse_create_args( Timbl::TimblOpts& Opts );
    void parse_run_args( Timbl::TimblOpts& Opts );
    bool isClone() const { return cloned; };
    void ShowCats( std::ostream& os, std::vector<int>& Pat, int slots );
    bool setLog( LogStream& );
    int ProcessLines( std::istream&, std::ostream& );
    void read_lexicon( const std::string& );
    void read_listfile( const std::string&, StringHash * );
    static TaggerClass *StartTagger( Timbl::TimblOpts&, LogStream* = 0 );
    static int CreateTagger( Timbl::TimblOpts& );
  private:
    LogStream *cur_log;
    sentence mySentence;
    Timbl::TimblAPI *KnownTree;
    Timbl::TimblAPI *unKnownTree;
    std::string Timbl_Options;
    std::string knownstr;
    std::string unknownstr;
    std::string uwf;
    std::string kwf;
    int nwords;
    bool initialized;
    StringHash TheLex;
    StringHash *kwordlist;
    StringHash *uwordlist;
    BeamData *Beam;
    input_kind_type input_kind;
    bool piped_input;
    bool lexflag;
    bool knowntreeflag;
    bool unknowntreeflag;
    bool knowntemplateflag;
    bool unknowntemplateflag;
    bool knownoutfileflag;
    bool unknownoutfileflag;
    bool reverseflag;
    bool dumpflag;
    bool distance_flag;
    bool distrib_flag;
    bool klistflag;
    int Beam_Size;
    std::vector<double> distance_array;
    std::vector<std::string> distribution_array;

    int makedataset( std::istream& infile, bool do_known );
    bool readsettings( std::string& fname );
    void create_lexicons( const std::string& filename );
    int ProcessFile( std::istream&, std::ostream& );
    void ProcessTags( TagInfo * );
    std::string Tag();
    void InitTest( MatchAction );
    bool NextBest( int, int );
    const Timbl::TargetValue *Classify( MatchAction, const std::string&, 
					const Timbl::ValueDistribution *distribution, 
					double& );
    std::string get_result();
    void statistics( int& no_known,
		     int& no_unknown,
		     int& no_correct_known, 
		     int& no_correct_unknown );
    std::string pat_to_string( MatchAction, int );

    std::string TimblOptStr;
    int FilterTreshold;
    int Npax;
    int TopNumber;
    bool DoSort;
    bool DoTop;
    bool DoNpax;
    bool KeepIntermediateFiles;

    std::string KtmplStr;
    std::string UtmplStr;
    std::string l_option_name;
    std::string K_option_name;
    std::string U_option_name;
    std::string r_option_name;
    std::string L_option_name;
    std::string EosMark;
    
    PatTemplate Ktemplate;
    PatTemplate Utemplate;
    Lexicon *MT_lexicon;
  
    std::string UnknownTreeName;
    std::string KnownTreeName;
    std::string LexFileName;
    std::string MTLexFileName;
    std::string TopNFileName;
    std::string NpaxFileName;
    std::string TestFileName;
    std::string OutputFileName;
    std::string SettingsFileName;
    std::string SettingsFilePath;
    
    bool cloned;
    std::vector<int> TestPat; 
  };

  void RemoveTagger( TaggerClass * );
}

#endif
  
