#ifndef CondIter_CondBasicIter_h
#define CondIter_CondBasicIter_h

#include "CondCore/IOVService/interface/IOVIterator.h"
#include "CondCore/DBCommon/interface/RelationalStorageManager.h"
#include "CondCore/DBCommon/interface/PoolStorageManager.h"
#include "CondCore/DBCommon/interface/AuthenticationMethod.h"
#include "CondCore/DBCommon/interface/SessionConfiguration.h"
#include "CondCore/DBCommon/interface/ConnectionConfiguration.h"
#include "CondCore/DBCommon/interface/MessageLevel.h"
#include "CondCore/DBCommon/interface/DBSession.h"
#include "CondCore/DBCommon/interface/Exception.h"
#include "CondCore/MetaDataService/interface/MetaData.h"
#include "CondCore/IOVService/interface/IOVService.h"
#include "CondCore/DBCommon/interface/ConnectMode.h"



class CondBasicIter{

    private:
        
        cond::IOVIterator* ioviterator;
        std::string payloadContainer;
        cond::PoolStorageManager *pooldb;
        /*minimum and maximum of the interval where search IOVs*/     
        unsigned int iter_Min;
        unsigned int iter_Max;
        /*start time of each IOV*/
        unsigned int m_startTime;
        /*stop time of each IOV*/
        unsigned int m_stopTime;
        unsigned int m_time;

        
    public:
       
        CondBasicIter();
        ~CondBasicIter();    
            
        template <class A> friend class CondIter;
    
        void create(const std::string & NameDB,
                    const std::string & FileXml,
                    const std::string & File,
                    const std::string & User = "",
                    const std::string & Pass = ""
                   );
       
        void setRange(unsigned int min,unsigned int max);
        void setRange(int min,int max); 

        void setMin(unsigned int min);
        void setMin(int min);

        void setMax(unsigned int max);
        void setMax(int max);
 
        unsigned int getTime();
  
        unsigned int getStartTime();
 
        unsigned int getStopTime();
       
        unsigned int getMin();
       
        unsigned int getMax();
       
        void getRange(unsigned int*,unsigned int*);
 
        void setStartTime(unsigned int start);
        
        void setStopTime(unsigned int stop); 

        void setTime(unsigned int time); 
  
};


#endif
