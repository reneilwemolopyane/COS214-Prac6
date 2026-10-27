#include "WorkflowMemento.h"

#include <ctime>

WorkflowMemento::WorkflowMemento(std::string version_, WorkItemState* state_, std::chrono::system_clock::time_point ts)
    : version(version_), state(state_), timestamp(ts) {}

std::string WorkflowMemento::getVersion() const {
    return version;
    }
WorkItemState* WorkflowMemento::getState() const { 
   return state;
 }

std::string WorkflowMemento::getStateName() const {

    return state ? state->name() : "Unknown_State";
}
std::chrono::system_clock::time_point WorkflowMemento::getTimestamp() const { 
   return timestamp; 
}
//function to return the timestamp in year month date and time
std::string getTimestampString() const{
   //converting to classic time type
   std::time_t time_now=std::chrono::system_clock::to_time_t(timestamp);
   //converting to calendar time structure directly
   std::tm* local_time=std::locatime(&time_now);
   char buffer[80];
   //format the calendar numbers directly into standard text
   std::strftime(buffer,sizeof(buffer),"%Y-%m-%d %H:%M:%S", local_time)

   return std::string(buffer);

}