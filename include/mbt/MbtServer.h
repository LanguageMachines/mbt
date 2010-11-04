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
#ifndef MBTSERVER_H
#define MBTSERVER_H

namespace Tagger {
  class MbtServer {
    friend class TaggerClass;
  public:
    LogStream myLog;
    bool doDebug() { return debug; };
    TaggerClass *theExp(){ return exp; };
    virtual ~MbtServer();
    static std::string VersionInfo( bool );
    MbtServer( Timbl::TimblOpts& );
    void RunClassicServer();
  protected:
    bool getConfig( const std::string& );
    bool startClassicServer( int, int=0 );
    //    bool startMultiServer( const std::string& );
    //    void RunHttpServer();
    TaggerClass *splitChild() const;
    void setDebug( bool d ){ debug = d; };
    Sockets::ServerSocket *TcpSocket() const { return tcp_socket; };
    TaggerClass *exp;
    std::string logFile;
    std::string pidFile;
    bool doDaemon;
  private:
    bool debug;
    int maxConn;
    int serverPort;
    Sockets::ServerSocket *tcp_socket;
    //    std::string serverProtocol;
    //    std::string serverConfigFile;
    //    std::map<std::string, std::string> serverConfig;
  };

}
#endif
