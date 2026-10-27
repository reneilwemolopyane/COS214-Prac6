#include "BottleneckVisitor.h"
#include "ProcessDataStore.h"
#include "Command.h"



 void BottleneckVisitor::visitState(WorkItemstate& data) {
   auto currentTime = std::chrono::steady_clock::now();
    auto timeSpent = currentTime - data.enteredAt;
        
        // Convert the raw duration specifically into milliseconds or seconds
        auto durationInMs = std::chrono::duration_cast<std::chrono::milliseconds>(timeSpent).count();
        
        std::cout << "Transaction has been stalled in this state for: " << durationInMs << " ms\n";
        
        // SLA Threshold check ( alert if stuck for more than 500 milliseconds)
        if (durationInMs > 500) {
            //im thinking maybe have a command alert that can alert the requeid people to deal with this
            std::cout << "[ALERT] Bottleneck detected in this staging node!\n";
        }
 } 
    void BottleneckVisitor::visitProgressDataStore(ProcessDataStore& data) {
        if(data.IspastThreshold()){
             std::cout << "[ALERT] Memento Memory Bloat Size: " 
                      << data.getLog().size() << "\n";
            data.backupstorage();

        }

    } 
    void BottleneckVisitor::visitChain(ChainOfresponsibility& ) {
         auto currentTime = std::chrono::steady_clock::now();
    auto timeSpent = currentTime - data.enteredAt;
        
        // Convert the raw duration specifically into milliseconds or seconds
        auto durationInMs = std::chrono::duration_cast<std::chrono::milliseconds>(timeSpent).count();
        
        std::cout << "Transaction has been stalled in this state for: " << durationInMs << " ms\n";
        
        // SLA Threshold check (e.g., alert if stuck for more than 500 milliseconds)
        if (durationInMs > 500) {
            //im thinking maybe have a command alert that can alert the requeid people to deal with this
            std::cout << "[ALERT] Bottleneck detected in this staging node!\n";
        }

    }

