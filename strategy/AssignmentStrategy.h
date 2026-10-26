#pragma once
#include <vector>
#include <map>
#include <string>

class WorkItem;
class Participant;

/** 
 * @brief Strategy interface for choosing who does a work item.
 * Also the prototype role: clone() lets WorkAssigner keep its own copy.
 */
class AssignmentStrategy {
  public:
    virtual ~AssignmentStrategy() {}
    /** @return the chosen participant, or nullptr if nobody is eligible */
    virtual Participant* selectParticipant(const WorkItem& item, const std::vector<Participant*>&) = 0;

    /** @brief Prototype: caller owns the returned copy. */
    virtual AssignmentStrategy* clone()const = 0;

    /** @brief Name shown in the GUI and audit history. */
    virtual std::string name()const = 0;
  protected:
    /** @brief Keeps only candidates holding the role the item needs. */
};

/** @brief First eligible participant. */
class RoleBasedStrategy : public AssignmentStrategy{
  public: 
    Participant* selectParticipant(const WorkItem&, const std::vector<Participant*>&)override;
    AssignmentStrategy* clone()const override;
    std::string name()const;
};

/** @brief Eligible participant with the lowest workload. */
class ShortestQueueStrategy : public AssignmentStrategy{
  public: 
    Participant* selectParticipant(const WorkItem&, const std::vector<Participant*>&)override;
    AssignmentStrategy* clone()const override;
    std::string name()const;
};

/** @brief Rotates through eligible participants. Stateful, hence an object. */
class RoundRobinStrategy : public AssignmentStrategy{
  private:
    int lastIndex;
  public:
    RoundRobinStrategy() : lastIndex(-1){}
    Participant* selectParticipant(const WorkItem&, const std::vector<Participant*>&)override;
    AssignmentStrategy* clone()const override;
    std::string name()const;
};

/**
 * @brief Chooses the role from a data value on the item, e.g
 * claimType "vehicle" -> "VehicleAssessor". The industry definition fills
 * the map so the class stays generic
 */
class DataRuleStrategy : public AssignmentStrategy{
  private:
    std::string dataKey; //e.g "claimtype" or "faculty"
    std::map<std::string, std::string> roleByValue;
  public: 
    DataRuleStrategy(const std::string& key) : dataKey(key){}
    void addMapping(const std::string& value, const std::string& role);
    Participant* selectParticipant(const WorkItem&, const std::vector<Participant*>&)override;
    AssignmentStrategy* clone()const override;
    std::string name()const;
};