#include "tividy/memento/ProcessDataStore.h"

ProcessDataStore::ProcessDataStore(size_t maxThreshold) : threshold(maxThreshold) {}

void ProcessDataStore::append(InstanceMemento* mem) {
    if(mem){
    log.push_back(mem);
    }

}                  
InstanceMemento* ProcessDataStore::undo(){ 
     if (log.empty()){
      return nullptr;
     }

        InstanceMemento* latest = log.back();
        log.pop_back();
        
        return latest;
    
}  

    ProcessDataStore::~ProcessDataStore(){
    for (InstanceMemento* mem : log) {
            delete mem;
        }
        for (InstanceMemento* mem : backupStorage) {
        delete mem;
       }
         log.clear();
         backupStorage.clear();
    }

    bool ProcessDataStore::IspastThreshold() const{
        return log.size()>threshold ; 
    }
    
void ProcessDataStore::backupstorage(){
    while (log.size() > threshold) {
        backupStorage.push_back(log.front());
        log.erase(log.begin());
    }
}

const std::vector<InstanceMemento*>& ProcessDataStore::getLog() const { 
    return log; 
}
const std::vector<InstanceMemento*>& ProcessDataStore::getBackupStorage() const {
     return backupStorage; 
    }

void ProcessDataStore::accept(WorkflowVisitor& visitor) {
         visitor.visitProcessDataStore(*this); 
    }