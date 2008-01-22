#include "CondCore/DBCommon/interface/CoralTransaction.h"
#include "CondCore/DBCommon/interface/DBSession.h"
#include "CondCore/DBCommon/interface/MessageLevel.h"
#include "CondCore/DBCommon/interface/AuthenticationMethod.h"
#include "CondCore/DBCommon/interface/SessionConfiguration.h"
#include "CondCore/DBCommon/interface/Connection.h"
#include "RelationalAccess/ISchema.h"
#include "CoralBase/AttributeList.h"

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
    cond::DBSession* session=new cond::DBSession;
    session->configuration().setAuthenticationMethod( cond::Env );
    if(debug){
      session->configuration().setMessageLevel( cond::Debug );
    }else{
      session->configuration().setMessageLevel( cond::Error );
    }
    // session->configuration().connectionConfiguration()->setConnectionRetrialTimeOut( 600 );
    //session->configuration().connectionConfiguration()->enableConnectionSharing();
    //session->configuration().connectionConfiguration()->enableReadOnlySessionOnUpdateConnections();  
    std::string userenv(std::string("CORAL_AUTH_USER=")+user);
    std::string passenv(std::string("CORAL_AUTH_PASSWORD=")+pass);
    ::putenv(const_cast<char*>(userenv.c_str()));
    ::putenv(const_cast<char*>(passenv.c_str()));
    session->open();
    cond::Connection myconnection(connect);
    myconnection.connect(session);
    cond::CoralTransaction& coraldb=myconnection.coralTransaction();
    coraldb.start(false);
    coral::AttributeList emptybinddata;
    std::set<std::string> tables=coraldb.nominalSchema().listTables();
    std::set<std::string>::iterator it;
    std::set<std::string>::iterator itEnd;
    for( it=tables.begin(); it!=tables.end(); ++it){
      coraldb.nominalSchema().dropTable(*it);
      std::cout<<*it<<" droped"<<std::endl;
    }
    coraldb.commit();
    myconnection.disconnect();
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
  

