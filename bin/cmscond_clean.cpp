#include "CondCore/DBCommon/interface/Exception.h"
#include "CondCore/DBCommon/interface/RelationalStorageManager.h"
#include "CondCore/DBCommon/interface/DBSession.h"
#include "CondCore/DBCommon/interface/MessageLevel.h"
#include "CondCore/DBCommon/interface/AuthenticationMethod.h"
#include "CondCore/DBCommon/interface/ConnectionConfiguration.h"
#include "CondCore/DBCommon/interface/SessionConfiguration.h"

#include "RelationalAccess/ISessionProxy.h"
#include "RelationalAccess/ISchema.h"
#include "RelationalAccess/ITable.h"
#include "RelationalAccess/ITableDataEditor.h"
#include "CoralBase/AttributeList.h"
#include "CoralBase/Attribute.h"

#include <boost/program_options.hpp>
#include <stdexcept>
#include <string>
#include <iostream>

int main(int argc, char** argv) {
  boost::program_options::options_description desc("Allowed options");
  boost::program_options::options_description visible("Usage: cmscond_clean contactstring [-h]\n Options");
  visible.add_options()
    ("connect,c", boost::program_options::value<std::string>(), "contact string to db(required)")
    ("user,u",boost::program_options::value<std::string>(),"user name (default \"\")")
    ("pass,p",boost::program_options::value<std::string>(),"password (default \"\")")
    ("debug,d","switch on debug mode")
    ("help,h", "help message")
    ;
  
  desc.add(visible);
  boost::program_options::variables_map vm;
  try{
    boost::program_options::store(boost::program_options::command_line_parser(argc, argv).options(desc).run(), vm);    
    boost::program_options::notify(vm);
  }catch(const boost::program_options::error& er) {
    std::cerr << er.what()<<std::endl;
    return 1;
  }
  if (vm.count("help")) {
    std::cout << visible <<std::endl;;
    return 0;
  }
  if(!vm.count("connect")){
    std::cerr <<"[Error] no contactString given \n";
    std::cerr<<" please do "<<argv[0]<<" --help \n";
    return 1;
  }
  
  std::string connect=vm["connect"].as<std::string>();
  std::string user("");
  std::string pass("");
  bool debug=false;
  if(vm.count("user")){
    user=vm["user"].as<std::string>();
  }
  if(vm.count("pass")){
    pass=vm["pass"].as<std::string>();
  }
  if(vm.count("debug")){
    debug=true;
  }
  try{
    cond::DBSession* session=new cond::DBSession(true);
    session->sessionConfiguration().setAuthenticationMethod( cond::Env );
    if(debug){
      session->sessionConfiguration().setMessageLevel( cond::Debug );
    }else{
      session->sessionConfiguration().setMessageLevel( cond::Error );
    }
    session->connectionConfiguration().setConnectionRetrialTimeOut( 600 );
    session->connectionConfiguration().enableConnectionSharing();
    session->connectionConfiguration().enableReadOnlySessionOnUpdateConnections();  
    std::string userenv(std::string("CORAL_AUTH_USER=")+user);
    std::string passenv(std::string("CORAL_AUTH_PASSWORD=")+pass);
    ::putenv(const_cast<char*>(userenv.c_str()));
    ::putenv(const_cast<char*>(passenv.c_str()));
    session->open();
    cond::RelationalStorageManager coraldb(connect);
    coraldb.connect(cond::ReadWriteCreate);
    coraldb.startTransaction(false);
    coral::AttributeList emptybinddata;
    std::set<std::string> tables=coraldb.sessionProxy().nominalSchema().listTables();
    std::set<std::string>::iterator it;
    std::set<std::string>::iterator itEnd;
    for( it=tables.begin(); it!=tables.end(); ++it){
      coraldb.sessionProxy().nominalSchema().dropTable(*it);
      std::cout<<*it<<" droped"<<std::endl;
    }
    coraldb.commit();
    coraldb.disconnect();
    session->close();
    delete session;
  }catch( std::exception& e ) {
    std::cerr << e.what() << std::endl;
    exit(-1);
  }catch( ... ) {
    std::cerr << "Funny error" << std::endl;
    exit(-1);
  }
  return 0;
}
  
