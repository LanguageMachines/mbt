/*
  Copyright (c) 1998 - 2023
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
#ifndef MBT_TAGGER_H
#define MBT_TAGGER_H

#include "mbt/Pattern.h"
#include "mbt/Sentence.h"
#include "ticcutils/UniHash.h"
#include "ticcutils/Timer.h"
#include "timbl/TimblAPI.h"

namespace TiCC {
  class LogStream;
  class Timer;
}

namespace Tagger {

  class TagInfo;

  std::string Version();
  std::string VersionName();
  double DataVersion();

  extern const std::string mbt_short_opts;
  extern const std::string mbt_long_opts;

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
    void InitPaths( Hash::UnicodeHash&,
		    const Timbl::TargetValue *,
		    const Timbl::ValueDistribution * );
    void NextPath( Hash::UnicodeHash&,
		   const Timbl::TargetValue *,
		   const Timbl::ValueDistribution *,
		   int );
    void ClearBest();
    void Shift( int, int );
    void Print( std::ostream& os, int i_word, Hash::UnicodeHash& TheLex );
    void PrintBest( std::ostream& os, Hash::UnicodeHash& TheLex );
    int size;
    int **paths;
    int **temppaths;
    double *path_prob;
    n_best_tuple **n_best_array;
  private:
    BeamData( const BeamData& ); // inhibit copies
    BeamData& operator=( const BeamData& ); // inhibit copies
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
    std::vector<TagResult> tagLine( const icu::UnicodeString& );
    nlohmann::json tag_line_to_JSON( const std::string& );
    nlohmann::json tag_JSON_to_JSON( const nlohmann::json& );
    std::vector<TagResult> tagSentence( sentence& );
    icu::UnicodeString Tag( const icu::UnicodeString& inp ){
      return TRtoString( tagLine(inp) );
    };
    icu::UnicodeString TRtoString( const std::vector<TagResult>& ) const;
    int TagLine( const icu::UnicodeString&, icu::UnicodeString& );
    // only for backward compatability
    int CreateKnown();
    int CreateUnknown();
    bool CreateSettingsFile();
    icu::UnicodeString set_eos_mark( const icu::UnicodeString& );
    bool set_default_filenames();
    bool parse_create_args( TiCC::CL_Options& );
    bool parse_run_args( TiCC::CL_Options&, bool = false );
    bool isClone() const { return cloned; };
    void ShowCats( std::ostream& os, const std::vector<int>& Pat, int slots );
    bool setLog( TiCC::LogStream& );
    int ProcessLines( std::istream&, std::ostream& );
    void read_lexicon( const std::string& );
    void read_listfile( const std::string&, Hash::UnicodeHash * );
    bool enriched() const { return input_kind == ENRICHED; };
    bool distance_is_set() const { return distance_flag; };
    bool distrib_is_set()const { return distrib_flag; };
    bool confidence_is_set() const { return confidence_flag; };
    static TaggerClass *StartTagger( TiCC::CL_Options&, TiCC::LogStream* = 0 );
    static int CreateTagger( TiCC::CL_Options& );
    static int CreateTagger( const std::string& );
    static int CreateTagger( int, char*[] );
    bool isInit() const { return initialized; };
    static void manifest( const std::string& );
    static void run_usage( const std::string& );
    static void gen_usage( const std::string& );
  private:
    TaggerClass& operator=( const TaggerClass& ); // inhibit copy-assignment
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
    Hash::UnicodeHash TheLex;
    Hash::UnicodeHash *kwordlist;
    Hash::UnicodeHash *uwordlist;
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
    TiCC::Timer timer1;
    TiCC::Timer timer2;
    TiCC::Timer timer3;

    int makedataset( std::istream& infile, bool do_known );
    bool readsettings( std::string& fname );
    bool create_lexicons();
    int ProcessFile( std::istream&, std::ostream& );
    void ProcessTags( TagInfo * );
    void InitTest( const sentence&, const std::vector<int>&, MatchAction );
    bool NextBest( const sentence&, std::vector<int>&, int, int );
    const Timbl::TargetValue *Classify( MatchAction,
					const icu::UnicodeString&,
					const Timbl::ValueDistribution **distribution,
					double& );
    void statistics( const sentence&,
		     int& no_known,
		     int& no_unknown,
		     int& no_correct_known,
		     int& no_correct_unknown );
    icu::UnicodeString pat_to_string( const sentence&,
				      const std::vector<int>&,
				      MatchAction,
				      int );

    std::string TimblOptStr;
    int FilterThreshold;
    int Npax;
    int TopNumber;
    bool DoTop;
    bool DoNpax;
    bool DoTagList;
    bool KeepIntermediateFiles;

    std::string KtmplStr;
    std::string UtmplStr;
    std::string l_option_name;
    std::string K_option_name;
    std::string U_option_name;
    std::string r_option_name;
    std::string L_option_name;
    icu::UnicodeString EosMark;
    icu::UnicodeString Separators;

    PatTemplate Ktemplate;
    PatTemplate Utemplate;
    std::map<icu::UnicodeString,icu::UnicodeString> *MT_lexicon;
    std::string UnknownTreeBaseName;
    std::string KnownTreeBaseName;
    std::string LexFileBaseName;
    std::string MTLexFileBaseName;
    std::string TopNFileBaseName;
    std::string NpaxFileBaseName;
    std::string UnknownTreeName;
    std::string KnownTreeName;
    std::string LexFileName;
    std::string MTLexFileName;
    std::string TopNFileName;
    std::string NpaxFileName;
    std::string TestFileName;
    std::string TestFilePath;
    std::string OutputFileName;
    std::string TagListName;
    std::string SettingsFileName;
    std::string SettingsFilePath;

    bool cloned;
  };

  class TagResult {
    friend std::vector<TagResult> TaggerClass::tagSentence( sentence& );
    friend std::vector<TagResult> StringToTR( const std::string&, bool );
  public:
  TagResult(): _distance(-1), _confidence(-1), _known(false){};
    bool is_known() const { return _known; };
    void set_known( bool b ) { _known = b; };

    icu::UnicodeString word() const { return _word; };
    void set_word( const icu::UnicodeString& w ) { _word = w; };

    icu::UnicodeString assigned_tag() const { return _tag; };
    void set_tag( const icu::UnicodeString& t ) { _tag = t; };

    icu::UnicodeString input_tag() const { return _input_tag; };
    void set_input_tag( const icu::UnicodeString& t ) { _input_tag = t; };

    icu::UnicodeString enrichment() const { return _enrichment; };
    void set_enrichment( const icu::UnicodeString& e ){ _enrichment = e; };

    icu::UnicodeString distribution() const { return _distribution; };
    void set_distribution( const icu::UnicodeString& d ){ _distribution = d; };

    double confidence() const { return _confidence; };
    void set_confidence( double c ){ _confidence = c; };

    double distance() const { return _distance; };
    void set_distance( double c ){ _distance = c; };
  private:
    icu::UnicodeString _word;
    icu::UnicodeString _input_tag;
    icu::UnicodeString _tag;
    icu::UnicodeString _enrichment;
    icu::UnicodeString _distribution;
    double _distance;
    double _confidence;
    bool _known;
  };

  inline void RemoveTagger( TaggerClass* tagger ){
    delete tagger;
  }

  std::vector<TagResult> StringToTR( const std::string&, bool=false );

  const icu::UnicodeString& indexlex( const unsigned int, Hash::UnicodeHash& );
  void get_weightsfile_name( std::string& opts, std::string& );
  void splits( const std::string& , std::string& common,
	       std::string& known, std::string& unknown );
  std::string prefixWithAbsolutePath( const std::string& , const std::string& );

}

#endif
