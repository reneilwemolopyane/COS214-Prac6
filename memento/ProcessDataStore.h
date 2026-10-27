#ifndef PROCESSDATASTORE_H
#define PROGRESSDATASTORE_H


#include "../visitor/WorkflowElement.h"
#include "../visitor/WorkflowVisitor.h"
#include <iostream>

class WorkflowVisitor;
#include "WorkflowMemento.h"

class ProcessDataStore:public WorkflowElement {        
public:
    ProcessDataStore(size_t maxThreshold = 60);  
    void append(WorkflowMemento* mem);             
    WorkflowMemento* undo(); 
    bool IspastThreshold() const;
    void backupstorage();
    void accept(WorkflowVisitor& visitor) override;
    ~ProcessDataStore();

     const std::vector<WorkflowMemento*>& getLog() const;
    const std::vector<WorkflowMemento*>& getBackupStorage() const;

private:

std::vector<WorkflowMemento*> log;
std::vector<WorkflowMemento*> backupStorage;     
size_t threshold;

};

#endif