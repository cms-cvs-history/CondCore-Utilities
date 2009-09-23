#include "CondCore/DBCommon/interface/DbSession.h"
#include "CondCore/DBCommon/interface/DbTransaction.h"
#include "CondCore/DBCommon/interface/Exception.h"
#include "CondCore/MetaDataService/interface/MetaData.h"
#include "CondCore/IOVService/interface/IOVService.h"
#include "CondCore/IOVService/interface/IOVEditor.h"
#include "CondCore/Utilities/interface/CSVHeaderLineParser.h"
#include "CondCore/Utilities/interface/CSVBlankLineParser.h"
#include "CondCore/Utilities/interface/Utilities.h"
#include "DataFormats/Provenance/interface/EventID.h"
#include "DataFormats/Provenance/interface/Timestamp.h"

#include <boost/spirit/core.hpp>
#include <boost/spirit/utility/lists.hpp>
#include <iterator>
#include <iostream>
#include <fstream>
#include <sstream>

void parseInputFile(std::fstream& inputFile,
		    std::vector< std::pair<cond::Time_t, std::string> >& newValues){
  unsigned int counter=0;
  std::vector<std::string> fieldNames;
  CSVHeaderLineParser headerParser;
  unsigned int tillidx=0; //default
  unsigned int tokenidx=1;//default
  while (! inputFile.eof() ){
    std::string line;
    std::getline (inputFile,line);
     CSVBlankLineParser blank;
    if(blank.isBlank(line)){
      continue;
    }
    if(counter==0) {
      if(!headerParser.parse(line)) {
        throw cms::Exception("unable to parse header: ")<<line;
      }
      fieldNames=headerParser.result();
      unsigned int idx=0;
      for(std::vector<std::string>::iterator it=fieldNames.begin();
          it!=fieldNames.end(); ++it, ++idx){
        if( *it==std::string("TILL") || *it==std::string("till") ){
          tillidx=idx;
        }
        if( *it==std::string("TOKEN") || *it==std::string("token") ){
          tokenidx=idx;
        }
      }
    }else{
      using namespace boost::spirit;
      std::vector<std::string> result;
      boost::spirit::rule<> strlist_parser;
      strlist_parser=list_p((*anychar_p)[push_back_a(result)],',');
      parse_info<> status=boost::spirit::parse(line.c_str(),strlist_parser);
      if(!status.full) throw cms::Exception("unable to parse data: ")<<line;
      unsigned int idx=0;
      cond::Time_t till=0;
      std::string payloadToken;
      for(std::vector<std::string>::iterator it=result.begin();
          it!=result.end(); ++it, ++idx){
        //std::cout<<idx<<std::endl;
        if( idx==tillidx ){
          //std::cout<<"is till "<<*it<<std::endl;
          std::istringstream iss(*it);
          if((iss>>std::dec>>till).fail()){
            throw cms::Exception("string conversion failed");
          }
        }
        if( idx==tokenidx ){
          //std::cout<<"is token"<<std::endl;
          payloadToken=*it;
        }
      }
      //std::cout<<"till "<<till<<std::endl;
      //std::cout<<"token "<<payloadToken<<std::endl;
      newValues.push_back(std::make_pair<cond::Time_t, std::string>(till,payloadToken));
    }
    ++counter;
    continue;
  }
}

namespace cond {
  class ShuffleIOVUtilities : public Utilities {
    public:
      ShuffleIOVUtilities();
      ~ShuffleIOVUtilities();
      int execute();
  };
}

cond::ShuffleIOVUtilities::ShuffleIOVUtilities():Utilities("cmscond_shuffle_iov","inputFile"){
  addConnectOption();
  addAuthenticationOptions();
  addOption<std::string>("tag","t","tag (required)");
  addOption<std::string>("timetype","T","timetype of the tag (optional)");
  addOption<cond::Time_t>("endTime","e","end time (last till) (optional)");
}

cond::ShuffleIOVUtilities::~ShuffleIOVUtilities(){
}

int cond::ShuffleIOVUtilities::execute(){

  std::string tag = getOptionValue("tag");
  std::string timetype("runnumber");
  if(hasOptionValue("timetype")) timetype = getOptionValue("timetype");
  cond::Time_t  till=0;
  cond::Time_t  since=0;
  if(hasOptionValue("endTime")) till = getOptionValue("endTime");
  bool debug= hasDebug();
  std::string inputFileName = getOptionValue("inputFile");
  std::vector< std::pair<cond::Time_t, std::string> > newValues;
  std::fstream inputFile;
  inputFile.open(inputFileName.c_str(), std::fstream::in);
  parseInputFile(inputFile,newValues);
  inputFile.close();

  std::string iovtoken;
  cond::DbSession session= openDbSession("connect");
  cond::IOVService iovmanager(session);
  cond::IOVEditor* editor=iovmanager.newIOVEditor("");
  cond::DbScopedTransaction transaction(session);
  transaction.start(false);
  if (timetype=="runnumber"){
    since=(cond::Time_t)edm::EventID::firstValidEvent().run();
    editor->create(since,cond::runnumber);
  }else if(timetype=="timestamp"){
    since==(cond::Time_t)edm::Timestamp::beginOfTime().value();
    editor->create(since,cond::timestamp);
  }else{
    throw cond::Exception("unsupported timetype");
  }
  editor->bulkInsert(newValues);
  iovtoken=editor->token();
  transaction.commit();
  cond::MetaData metadata(session);
  transaction.start(false);
  metadata.addMapping(tag,iovtoken);
  transaction.commit();
  if(debug){
    std::cout<<"source iov token "<<iovtoken<<std::endl;
  }
  delete editor;
  return 0;
}

int main( int argc, char** argv ){
  cond::ShuffleIOVUtilities utilities;
  return utilities.run(argc,argv);
}


