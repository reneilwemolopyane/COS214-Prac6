#include "tividy/core/Participant.h"

Participant::Participant(const std::string& id, const std::string& name) : id(pid), name(pname), workload(0) {}

const std::string& Participant::getId()const{
    return id;
}
const std::string& Participant::getName()const{
    return name;
}

void Participant::addRole(const std::string& role){
    roles.insert(role);
}
bool Participant::hasRole(const std::string& role)const{
    return roles.find(role) != roles.end();
}
const std::set<std::string>& Participant::getRoles()const{
    return roles;
}

int getWorkload()const{
    return workload;
}
void itemAssigned(){
    ++workload;
}
void itemReleased(){
    if(worklod > 0) --workload;
}
