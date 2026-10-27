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
    size_t itemsToMove=10;

    for(size_t i=0;i<itemsToMove;++i){
        if(log.size() <=1) break;

        auto& oldestItem=log.at(1);
        backupStorage.push_back(std::move(oldestItem));

        log.erase(log.begin() + 1);

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