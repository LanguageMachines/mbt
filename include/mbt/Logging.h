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

#ifndef MBT_LOGGING_H
#define MBT_LOGGING_H

#include "ticcutils/LogStream.h"

extern TiCC::LogStream default_log;
extern TiCC::LogStream *cur_log;
extern TiCC::LogStream default_cout;
extern LogLevel Tagger_Log_Level;

#define LOG (*TiCC::Log(cur_log))
#define DBG (*TiCC::Dbg(cur_log))
#define xDBG (*TiCC::xDbg(cur_log))
#define xxDBG (*TiCC::xxDbg(cur_log))
#define COUT *TiCC::Log(default_cout)

extern LogLevel internal_default_level;
extern LogLevel Tagger_Log_Level;

#endif // LOGGING_H
