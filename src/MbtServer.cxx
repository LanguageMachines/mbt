// #include <fstream> 
// #include <iostream>
//#include <cstdio>
//#include <cstdlib>
//#include <cctype>
//#include <ctime>
#include <csignal>
#include <cerrno>
#include <string>
#include <unistd.h> // for unlink()
#include "config.h"
#include "timbl/TimblAPI.h"
#include "timblserver/TimblServerAPI.h"
#include "mbt/Logging.h"
#include "mbt/Tagger.h"
#include "mbt/MbtServer.h"

#include <pthread.h>

using namespace std;
using namespace Timbl;

namespace Tagger {

  TaggerClass *createServerPimpl( Timbl::TimblOpts& opts ){
    TaggerClass *exp = new TaggerClass();
    exp->parse_run_args( opts );
    exp->set_default_filenames();
    exp->InitTagging();
    return exp;
  }

  MbtServer::MbtServer( Timbl::TimblOpts& opts ): myLog("MbtServer"){
    debug = false;
    maxConn = 25;
    serverPort = -1;
    tcp_socket = 0;
    doDaemon = true;
    string val;
    bool mood;
    if ( opts.Find( "S", val, mood ) ) {
      serverPort = Timbl::stringTo<int>( val );
      if ( serverPort < 1 || serverPort > 32767 ){
	cerr << "-S option, portnumber invalid: " << serverPort << endl;
	exit(1);
      }
      //      opts.delete( "S" );
    }
    else {
      cerr << "missing -S<port> option" << endl;
      exit(1);
    }
    if ( opts.Find( "pidfile", val ) ) {
      pidFile = val;
      //      opts.delete( "pidfile" );
    }
    if ( opts.Find( "logfile", val ) ) {
      logFile = val;
      //      opts.delete( "logfile" );
    }
    if ( opts.Find( "daemonize", val ) ) {
      doDaemon = ( val != "no" && val != "NO" && val != "false" && val != "FALSE" );
      //      opts.delete( "daemonize" );
    }
    
    exp = createServerPimpl( opts );
  } 
 
  MbtServer::~MbtServer(){
    delete exp;
  }

  struct childArgs{
    MbtServer *Mother;
    Sockets::ServerSocket *socket;
    int maxC;
    TaggerClass *experiment;
  };
  
  // ***** This is the routine that is executed from a new thread **********
  void *tagChild( void *arg ){
    childArgs *args = (childArgs *)arg;
    MbtServer *theServer = args->Mother;
    Sockets::Socket *Sock = args->socket;
    static int service_count = 0;
    static pthread_mutex_t my_lock = PTHREAD_MUTEX_INITIALIZER;
    //
    // use a mutex to update the global service counter
    //
    pthread_mutex_lock( &my_lock );
    service_count++;
    int nw = 0;
    if ( service_count > args->maxC ){
      Sock->write( "Maximum connections exceeded\n" );
      Sock->write( "try again later...\n" );
      pthread_mutex_unlock( &my_lock );
      cerr << "Thread " << pthread_self() << " refused " << endl;
    }
    else {
      // Greeting message for the client
      //
      pthread_mutex_unlock( &my_lock );
      time_t timebefore, timeafter;
      time( &timebefore );
      // report connection to the server terminal
      //
      LOG << "Thread " << pthread_self() << ", Socket number = "
	  << Sock->getSockId() << ", started at: " 
	  << asctime( localtime( &timebefore) );
      Sock->write( "Welcome to the Mbt server.\n" );
      nw = args->experiment->ProcessSocket();
      time( &timeafter );
      LOG << "Thread " << pthread_self() << ", terminated at: " 
	  << asctime( localtime( &timeafter ) );
      LOG << "Total time used in this thread: " << timeafter - timebefore 
	  << " sec, " << nw << " words processed " << endl;
    }
    // exit this thread
    //
    pthread_mutex_lock( &my_lock );
    service_count--;
    LOG << "Socket Total = " << service_count << endl;
    pthread_mutex_unlock( &my_lock );
    delete Sock;
  }
  
  void MbtServer::RunClassicServer(){
    cerr << "Trying to Start a Server on port: " << serverPort << endl
	 << "maximum # of simultanious connections: " << maxConn
	 << endl;
    if ( !logFile.empty() ){
      ostream *tmp = new ofstream( logFile.c_str() );
      if ( tmp && tmp->good() ){
	cerr << "switching logging to file " << logFile << endl;
	cur_log->associate( *tmp );
	cur_log->message( "MbtServer:" );
	LOG << "Started logging " << endl;	
      }
      else {
	cerr << "unable to create logfile: " << logFile << endl;
	cerr << "not started" << endl;
	exit(EXIT_FAILURE);
      }
    }
    int start;
    if ( logFile.empty() )
      start = daemonize( 1, 1 );
    else
      start = daemonize( 0, 0 );
    if ( start < 0 ){
      LOG << "failed to daemonize error= " << strerror(errno) << endl;
      exit(EXIT_FAILURE);
    };
    if ( !pidFile.empty() ){
      // we have a liftoff!
      // signal it to the world
      unlink( pidFile.c_str() ) ;
      ofstream pid_file( pidFile.c_str() ) ;
      if ( !pid_file ){
	LOG << "unable to create pidfile:"<< pidFile << endl;
	LOG << "TimblServer NOT Started" << endl;
	exit(EXIT_FAILURE);
      }
      else {
	pid_t pid = getpid();
	pid_file << pid << endl;
      }
    }
    
      // set the attributes
    pthread_attr_t attr;
    if ( pthread_attr_init(&attr) ||
	 pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED ) ){
      LOG << "Threads: couldn't set attributes" << endl;
      exit(EXIT_FAILURE);
    }
    //
    // setup Signal handling to abort the server.
    signal( SIGINT, StopServerFun );
    
    pthread_t chld_thr;
    
    // start up server
    // 
    LOG << "Started Server on port: " << serverPort << endl
	<< "maximum # of simultanious connections: " << maxConn
	<< endl;
    
    Sockets::ServerSocket server;
    string portString = toString<int>(serverPort);
    if ( !server.connect( portString ) ){
      LOG << "failed to start Server: " << server.getMessage() << endl;
      exit(EXIT_FAILURE);
    }
    if ( !server.listen( maxConn ) < 0 ){
      LOG << "server: listen failed " << strerror( errno ) << endl;
      exit(EXIT_FAILURE);
    };
      
    while(true){ // waiting for connections loop
      Sockets::ServerSocket *newSock = new Sockets::ServerSocket();
      if ( !server.accept( *newSock ) ){
	if( errno == EINTR )
	  continue;
	else {
	  LOG << "Server: Accept Error: " << server.getMessage() << endl;
	  exit(EXIT_FAILURE);
	}
      }
      LOG << "Accepting Connection " << newSock->getSockId()
	  << " from remote host: " << newSock->getClientName() << endl;
      
      // create a new thread to process the incoming request 
      // (The thread will terminate itself when done processing
      // and release its socket handle)
      //
      childArgs *args = new childArgs();
      args->experiment = exp->clone( newSock );
      args->Mother = this;
      args->maxC = maxConn;
      args->socket = newSock;
      pthread_create( &chld_thr, &attr, tagChild, (void *)args );
      // the server is now free to accept another socket request 
    }
  }
  
  void RunServer( TimblOpts& Opts ){
    MbtServer server( Opts );
    server.RunClassicServer();
  }

}
