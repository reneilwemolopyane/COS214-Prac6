#ifndef PROCESSDATASTORE_H
#define PROCESSDATASTORE_H

#include <cstddef>
#include <vector>

#include "tividy/visitor/WorkflowElement.h"
#include "tividy/visitor/Visitor.h"
#include <iostream>

class WorkflowVisitor;
#include "tividy/memento/Memento.h"

class ProcessDataStore:public WorkflowElement {        
public:
    ProcessDataStore(size_t maxThreshold = 60);  
    void append(InstanceMemento* mem);             
    InstanceMemento* undo(); 
    bool IspastThreshold() const;
    void backupstorage();
    void accept(WorkflowVisitor& visitor) override;
    ~ProcessDataStore();

    const std::vector<InstanceMemento*>& getLog() const;
    const std::vector<InstanceMemento*>& getBackupStorage() const;

private:

std::vector<InstanceMemento*> log;
std::vector<InstanceMemento*> backupStorage;     
size_t threshold;

};

#endif