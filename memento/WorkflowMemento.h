#ifndef WORKFLOWMEMENTO_H
#define WORKFOWMEMENTO_H

//memento

#pragma once
#include <vector>
#include <chrono>
#include <string>
#include <sstream>

//#include "tividy/core/Common.h"
/** @file Memento.h  Memento. NOTE: generic names (InstanceMemento). STUB */

class WorkItemState;

class WorkflowMemento {
   
    public:
    WorkflowMemento(std::string version_, WorkItemState* state_, std::chrono::system_clock::time_point timestamp_);
    std::string getVersion() const;
    WorkItemState* getState() const;
    std::string getStateName() const;
    std::chrono::system_clock::time_point WorkflowMemento::getTimestamp() const;
    std::string getTimestampString() const; 

    private:

    friend class WorkflowInstance;

    WorkItemState* state;
    std::string version;
    std::chrono::system_clock::time_point timestamp;
  

};

#endif