#ifndef PROCESSDATASTORE_H
#define PROCESSDATASTORE_H

#include <cstddef>
#include <vector>

#include "tividy/visitor/WorkflowElement.h"
#include "tividy/visitor/WorkflowVisitor.h"
#include <iostream>

class WorkflowVisitor;
#include "tividy/memento/WorkflowMemento.h"

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