#include "ProcessDataStore.h"

ProcessDataStore::ProcessDataStore(size_t maxThreshold) : threshold(maxThreshold) {}

void ProcessDataStore::append(WorkflowMemento* mem) {
    if(mem){
    log.push_back(mem);
    }

}                  
WorkflowMemento*  ProcessDataStore::undo(){ 
     if (log.empty()){
      return nullptr;
     }

        WorkflowMemento* latest = log.back();
        log.pop_back();
        
        return latest;
    
}  

    ProcessDataStore::~ProcessDataStore(){
      for (WorkflowMemento* mem : log) {
            delete mem;
        }
        for (WorkflowMemento* mem : backupStorage) {
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

const std::vector<WorkflowMemento*>& ProcessDataStore::getLog() const { 
    return log; 
}
const std::vector<WorkflowMemento*>& ProcessDataStore::getBackupStorage() const {
     return backupStorage; 
    }

void ProcessDataStore::accept(WorkflowVisitor& visitor) {
         visitor.visitProcessDataStore(*this); 
    }