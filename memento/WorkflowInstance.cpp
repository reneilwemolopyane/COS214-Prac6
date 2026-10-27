#include "WorkflowInstance.h"
#include "WorkflowMemento.h"

WorkflowInstance::WorkflowInstance(std::string version_,WorkItemState* state_){
    version=version_;
    state=state_;
}
std::string WorkflowInstance::getversion(){
    return version;
}
    WorkItemState* WorkflowInstance::getState(){
        return state;
    }
    void WorkflowInstance::setVersion(std::string v){
        version=v;
    }
    void WorkflowInstance::display(){
        std::cout<<"Currently we are at the "<<state<<"running version :"<<version<<std::endl;
    }
    WorkflowMemento* WorkflowInstance::createMemento(){
        return new WorkflowMemento(version,state, std::chrono::system_clock::now());
    }
    void WorkflowInstance::restore(WorkflowMemento* mem){
       if(!mem){
        return;
       }
    this->state = mem->getState();
    this->version = mem->getVersion();
        display();
    }
