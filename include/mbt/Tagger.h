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
#ifndef MBT_TAGGER_H
#define MBT_TAGGER_H

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

  class TagResult;

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
    std::vector<TagResult> tagLine( const std::string& );
    std::vector<TagResult> tagSentence( sentence& );
    std::string Tag( const std::string& inp ){
      return TRtoString( tagLine(inp) );
    };
    std::string TRtoString( const std::vector<TagResult>& ) const;
    int TagLine( const std::string&, std::string& ); 
    // only for backward compatability
    int CreateKnown();
    int CreateUnknown();
    void CreateSettingsFile();
    bool set_default_filenames();
    bool parse_create_args( Timbl::TimblOpts& Opts );
    bool parse_run_args( Timbl::TimblOpts& Opts, bool = false );
    bool isClone() const { return cloned; };
    void ShowCats( std::ostream& os, const std::vector<int>& Pat, int slots );
    bool setLog( TiCC::LogStream& );
    int ProcessLines( std::istream&, std::ostream& );
    void read_lexicon( const std::string& );
    void read_listfile( const std::string&, StringHash * );
    static TaggerClass *StartTagger( Timbl::TimblOpts&, TiCC::LogStream* = 0 );
    static int CreateTagger( Timbl::TimblOpts& );
    bool isInit() const { return initialized; };
  private:
    TiCC::LogStream *cur_log;
    Timbl::TimblAPI *KnownTree;
    Timbl::TimblAPI *unKnownTree;
    std::string Timbl_Options;
    std::string commonstr;
    std::string knownstr;
    std::string unknownstr;
    std::string uwf;
    std::string kwf;
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
    bool confidence_flag;
    bool klistflag;
    int Beam_Size;
    std::vector<double> distance_array;
    std::vector<std::string> distribution_array;
    std::vector<double> confidence_array;

    void manifest();
    int makedataset( std::istream& infile, bool do_known );
    bool readsettings( std::string& fname );
    void create_lexicons();
    int ProcessFile( std::istream&, std::ostream& );
    void ProcessTags( TagInfo * );
    void InitTest( const sentence&, std::vector<int>&, MatchAction );
    bool NextBest( const sentence&, std::vector<int>&, int, int );
    const Timbl::TargetValue *Classify( MatchAction, const std::string&, 
					const Timbl::ValueDistribution **distribution, 
					double& );
    void statistics( const sentence&,
		     int& no_known,
		     int& no_unknown,
		     int& no_correct_known, 
		     int& no_correct_unknown );
    std::string pat_to_string( const sentence&, 
			       const std::vector<int>&,
			       MatchAction,
			       int );

    std::string TimblOptStr;
    int FilterThreshold;
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
    std::string TestFilePath;
    std::string OutputFileName;
    std::string SettingsFileName;
    std::string SettingsFilePath;
    
    bool cloned;
  };

  class TagResult {
    friend std::vector<TagResult> TaggerClass::tagSentence( sentence& );
  public:
  TagResult(): _distance(-1), _confidence(-1), _known(false){};
    bool isKnown() const { return _known; };
    std::string word() const { return _word; };
    std::string assignedTag() const { return _tag; };
    std::string inputTag() const { return _inputTag; };
    std::string enrichment() const { return _enrichment; };
    std::string distribution() const { return _distribution; };
    double confidence() const { return _confidence; };
    double distance() const { return _distance; };
  private:
    std::string _word;
    std::string _inputTag;
    std::string _tag;
    std::string _enrichment;
    std::string _distribution;
    double _distance;
    double _confidence;
    bool _known;
  };
  

  void RemoveTagger( TaggerClass * );
}

#endif
  
