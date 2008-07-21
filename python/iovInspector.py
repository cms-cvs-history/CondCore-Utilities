#
# iov info server backend
#
#  it assumes that all magic and incantations are done...
#

import pluginCondDBPyInterface as CondDB

class Iov :
       def __init__(self, db, tag) :
           self.__db = db
           self.__tag = tag
           try : 
               self.__modName = db.moduleName(tag)
               exec('import '+self.__modName+' as Plug')
           except RuntimeError :
               self.__modName = 0
           self.__me = db.iov(tag)

       def list(self) :
           ret = []
           for elem in self.__me.elements :
               ret.append( (elem.payloadToken(), elem.since(), elem.till(),0))
           return ret

       def summaries(self) :
           if (self.__modName==0) : return ["no plugin for "  + self.__tag+" no summaries"]
           exec('import '+self.__modName+' as Plug')
           ret = []
           for elem in self.__me.elements :
               p = Plug.Object(elem)
               ret.append( (elem.payloadToken(), elem.since(), elem.till(), p.summary()))
           return ret
           
       def trend(self, s, l) :
           if (self.__modName==0) : return ["no plugin for "  + self.__tag+" no trend"]
           exec('import '+self.__modName+' as Plug')
           ret = []
           vi = CondDB.VInt()
           for i in l:
               vi.append(int(i))
           ex = Plug.Extractor(s,vi)
           for elem in self.__me.elements :
               p = Plug.Object(elem)
               p.extract(ex)
               v = []
               for i in ex.values() :
                   v.append(i)
               ret.append((elem.since(),elem.till(),v))
           return ret  


class PayLoad :
    def __init__(self, db, token) :
        self.__db = db
        self.__token = token
        exec('import '+db.moduleName(token)+' as Plug')
        self.__me = Plug.Object(db.payLoad(token))

    def __str__(self) :
        return self.__me.dump()

    def object(self) :
        return self.__me

    def summary(self) :
        return self.__me.summary()

    def plot(fname, s, is, fs) :
        vi = CondDB.VInt()
        vf = CondDB.VFloat()
        for i in is:
            vi.append(int(i))
        for i in fs:
            vf.append(float(i))
        return self.__me.plot(fname,s,vi,vf)

