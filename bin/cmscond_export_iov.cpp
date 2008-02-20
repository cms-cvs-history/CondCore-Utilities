#include "CondCore/DBCommon/interface/ConnectionHandler.h"
#include "CondCore/DBCommon/interface/CoralTransaction.h"
#include "CondCore/DBCommon/interface/PoolTransaction.h"
#include "CondCore/DBCommon/interface/AuthenticationMethod.h"
#include "CondCore/DBCommon/interface/Connection.h"
#include "CondCore/DBCommon/interface/SessionConfiguration.h"
#include "CondCore/DBCommon/interface/FipProtocolParser.h"
#include "CondCore/DBCommon/interface/MessageLevel.h"
#include "CondCore/DBCommon/interface/DBSession.h"
#include "CondCore/DBCommon/interface/Exception.h"
#include "CondCore/MetaDataService/interface/MetaData.h"
#include "CondCore/IOVService/interface/IOVService.h"
#include "CondCore/IOVService/interface/IOVEditor.h"
#include "SealBase/SharedLibrary.h"
#include "SealBase/SharedLibraryError.h"
#include <boost/program_options.hpp>
#include <iterator>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <cstdlib>
int main( int argc, char** argv ){
  boost::program_options::options_description desc("options");
  boost::program_options::options_description visible("Usage: cmscond_export_iov [options] \n");
  visible.add_options()
    ("sourceConnect,s",boost::program_options::value<std::string>(),"source connection string(required)")
    ("destConnect,d",boost::program_options::value<std::string>(),"destionation connection string(required)")
    ("dictionary,D",boost::program_options::value<std::string>(),"data dictionary(required)")
    ("tag,t",boost::program_options::value<std::string>(),"tag to export(required)")
    ("payloadName,n",boost::program_options::value<std::string>(),"payload object name(required)")
    ("authPath,p",boost::program_options::value<std::string>(),"path to authentication xml(default .)")
    ("configFile,f",boost::program_options::value<std::string>(),"configuration file(optional)")
    ("withBlob","with blob streaming capability")
    ("debug","switch on debug mode")
    ("help,h", "help message")
    ;
  desc.add(visible);
  std::string sourceConnect, destConnect;
  std::string dictionary;
  std::string tag;
  std::string payloadName;
  std::string authPath(".");
  std::string configuration_filename;
  bool debug=false;
  bool withBlob=false;
  boost::program_options::variables_map vm;
  try{
    boost::program_options::store(boost::program_options::command_line_parser(argc, argv).options(desc).run(), vm);
    if (vm.count("help")) {
      std::cout << visible <<std::endl;;
      return 0;
    }
    if( vm.count("configFile") ){
      configuration_filename=vm["configFile"].as<std::string>();
      if (! configuration_filename.empty()){
	std::fstream configuration_file;
	configuration_file.open(configuration_filename.c_str(), std::fstream::in);
	boost::program_options::store(boost::program_options::parse_config_file(configuration_file,desc), vm);
	configuration_file.close();
      }
    }
    if(!vm.count("sourceConnect")){
      std::cerr <<"[Error] no sourceConnect[s] option given \n";
      std::cerr<<" please do "<<argv[0]<<" --help \n";
      return 1;
    }else{
      sourceConnect=vm["sourceConnect"].as<std::string>();
    }
    if(!vm.count("destConnect")){
      std::cerr <<"[Error] no destConnect[s] option given \n";
      std::cerr<<" please do "<<argv[0]<<" --help \n";
      return 1;
    }else{
      destConnect=vm["destConnect"].as<std::string>();
    }
    if(!vm.count("dictionary")){
      std::cerr <<"[Error] no dictionary[D] option given \n";
      std::cerr<<" please do "<<argv[0]<<" --help \n";
      return 1;
    }else{
      dictionary=vm["dictionary"].as<std::string>();
    }
    if(!vm.count("tag")){
      std::cerr <<"[Error] no tag[t] option given \n";
      std::cerr<<" please do "<<argv[0]<<" --help \n";
      return 1;
    }else{
      tag=vm["tag"].as<std::string>();
    }
    if(!vm.count("payloadName")){
      std::cerr <<"[Error] no payloadName[n] option given \n";
      std::cerr<<" please do "<<argv[0]<<" --help \n";
      return 1;
    }else{
      payloadName=vm["payloadName"].as<std::string>();
    }
    if( vm.count("authPath") ){
      authPath=vm["authPath"].as<std::string>();
    }
    if(vm.count("debug")){
      debug=true;
    }
    if(vm.count("withBlob")){
      withBlob=true;
    }
    boost::program_options::notify(vm);
  }catch(const boost::program_options::error& er) {
    std::cerr << er.what()<<std::endl;
    return 1;
  }
  std::string dictlibrary=seal::SharedLibrary::libname( dictionary );
  if(debug){
    std::cout<<"sourceConnect:\t"<<sourceConnect<<'\n';
    std::cout<<"destConnect:\t"<<destConnect<<'\n';
    std::cout<<"dictionary:\t"<<dictlibrary<<'\n';
    std::cout<<"payloadName:\t"<<payloadName<<'\n';
    std::cout<<"tag:\t"<<tag<<'\n';
    std::cout<<"authPath:\t"<<authPath<<'\n';
    if(withBlob) std::cout<<"with Blob streamer"<<authPath<<'\n';
    std::cout<<"configFile:\t"<<configuration_filename<<std::endl;
  }
  //
  try {
    seal::SharedLibrary::load( dictlibrary );
  }catch ( seal::SharedLibraryError *error) {
    throw std::runtime_error( error->explainSelf().c_str() );
  }

  cond::DBSession session;

  if(!debug){
    session.configuration().setMessageLevel(cond::Error);
  }else{
    session.configuration().setMessageLevel(cond::Debug);
  }

  session.configuration().setAuthenticationMethod(cond::XML);
  session.configuration().setBlobStreamer("");

  std::string pathval("CORAL_AUTH_PATH=");
  pathval+=authPath;
  ::putenv(const_cast<char*>(pathval.c_str()));

  cond::ConnectionHandler& conHandler=cond::ConnectionHandler::Instance();
  conHandler.registerConnection("mysourcedb",sourceConnect,-1);
  conHandler.registerConnection("mydestdb",destConnect,-1);

  try{
    session.open();

    conHandler.connect(session);
    std::string sourceiovtoken;
    std::string destiovtoken;
    cond::TimeType sourceiovtype;
    if( sourceConnect.find("sqlite_fip:") != std::string::npos ){
      cond::FipProtocolParser p;
      sourceConnect=p.getRealConnect(sourceConnect);
    }


    // find tag in source
    {
      cond::CoralTransaction& sourceCoralDB=conHandler.getConnection("mysourcedb")->coralTransaction();
      sourceCoralDB.start(true);
      cond::MetaData  sourceMetadata(sourceCoralDB);
      if( !sourceMetadata.hasTag(tag) ){
	throw std::runtime_error(std::string("tag ")+tag+std::string(" not found") );
      }
      //sourceiovtoken=sourceMetadata->getToken(tag);
      cond::MetaDataEntry entry;
      sourceMetadata.getEntryByTag(tag,entry);
      sourceiovtoken=entry.iovtoken;
      sourceiovtype=entry.timetype;
      
      sourceCoralDB.commit();
      if(debug){
	std::cout<<"source iov token "<<sourceiovtoken<<std::endl;
	std::cout<<"source iov type "<<sourceiovtype<<std::endl;
      }
    }

    // find tag in destination
    {
      cond::CoralTransaction& coralDB=conHandler.getConnection("mydestdb")->coralTransaction();
      coralDB.start(true);
      cond::MetaData  metadata(coralDB);
      if( metadata.hasTag(tag) ){
	cond::MetaDataEntry entry;
	metadata.getEntryByTag(tag,entry);
	destiovtoken=entry.iovtoken;
	if (sourceiovtype!=entry.timetype) {
	  // throw...
	}
      }
      coralDB.commit();
      if(debug){
	std::cout<<"destintion iov token "<< destiovtoken<<std::endl;
      }
    }


    cond::PoolTransaction& sourcedb=conHandler.getConnection("mysourcedb")->poolTransaction();
    cond::PoolTransaction& destdb=conHandler.getConnection("mydestdb")->poolTransaction();
    cond::IOVService iovmanager(sourcedb);
    
    cond::Time_time since = iovmanager.globalSince();
    cond::Time_time till = iovmanager.globalTill();


    cond::IOVEditor* editor=iovmanager.newIOVEditor();
    sourcedb.start(true);
    destdb.start(false);
    bool newIOV = destiovtoken.empty();
    destiovtoken=iovmanager.exportIOVWithPayload( destdb,
						  sourceiovtoken,
						  destiovtoken,
						  since, till,
						  payloadName );
    sourcedb.commit();
    destdb.commit();
    if (newIOV) {
      cond::CoralTransaction& destCoralDB=conHandler.getConnection("mydestdb")->coralTransaction();
      cond::MetaData destMetadata(destCoralDB);
      destCoralDB.start(false);
      destMetadata.addMapping(tag,destiovtoken,sourceiovtype);
      if(debug){
	std::cout<<"dest iov token "<<destiovtoken<<std::endl;
	std::cout<<"dest iov type "<<sourceiovtype<<std::endl;
      }
      destCoralDB.commit();
    }
    delete editor;
    session.close();
  }catch(const cond::Exception& er){
    std::cout<<"error "<<er.what()<<std::endl;
  }catch(const std::exception& er){
    std::cout<<"std error "<<er.what()<<std::endl;
  }
  return 0;
}
