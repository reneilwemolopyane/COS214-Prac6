#pragma once
#include <set>
#include <string>

/**
 * @brief A person or system that can be assigned work items.
 * Roles decide eligibility; workload is the number of items currently
 * assigned. Participants objects belong to one workflow instance, so
 * workload never leaks between instances
 */
class Participant{
    private:
      std::string id;
      std::string name;
      std::set<std::string>roles;
      int workload;
    public:
      Participant(const std::string& id, const std::string& name);

      const std::string& getId()const;
      const std::string& getName()const;

      /** @brief Grants a role (e.g. "Assessor", "Nurse"). Duplicates are ignored. */
      void addRole(const std::string& role);
      bool hasRole(const std::string& role)const;
      const std::set<std::string>& getRoles()const;

      /**  @return number of items currently assigned(used by ShortedQueueStrategy)*/
      int getWorkload()const;

      /** @brief Called when an item is assigned to this participant. */
       void itemAssigned();

       /** @brief Called when an assigned item completes, is rejected or is escalated away. */
        void itemReleased();

};