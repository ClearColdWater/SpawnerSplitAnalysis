#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <set>
#include <vector>
#include <map>

int main() {
    std::ifstream fin("backleftskirt.log");
    
    // Core data structures
    std::unordered_map<int, int> logicTimer;      // ID -> current logic timer
    std::unordered_map<int, int> spawnTick;       // ID -> spawn batch tick
    std::unordered_map<int, std::set<int>> batchMap;  // batch -> IDs in this batch
    std::unordered_map<int, int> lastSeen;        // ID -> last seen tick
    std::unordered_map<int, bool> isAlive;        // ID -> alive status
    
    // For handling continuous spawn
    int currentTick = 0;
    int currentBatchStartTick = -1;
    bool inContinuousSpawn = false;
    std::set<int> seenThisTick;
    std::vector<int> newIdsThisTick;
    
    // Record logic lifespan distribution
    std::map<int, int> lifespanDistribution;
    
    std::string line;
    int lineNum = 0;
    
    std::cout << "=== Starting Log Analysis ===" << std::endl;
    
    while (std::getline(fin, line)) {
        lineNum++;
        
        // Check if it's a tick end marker
        if (line.find("Tick!") != std::string::npos) {
            bool hasActivity = false;
            
            // 1. Handle continuous spawn batch logic
            if (!newIdsThisTick.empty()) {
                hasActivity = true;
                std::cout << "[TICK " << currentTick << "] New spawns: " << newIdsThisTick.size() << std::endl;
                
                // New spawns this tick
                if (!inContinuousSpawn) {
                    // Start a new continuous spawn sequence
                    inContinuousSpawn = true;
                    currentBatchStartTick = currentTick;
                    std::cout << "  Starting new continuous spawn sequence, batch start tick=" 
                              << currentBatchStartTick << std::endl;
                }
                
                // Assign all new IDs to the starting tick of current batch
                for (int newId : newIdsThisTick) {
                    spawnTick[newId] = currentBatchStartTick;
                    batchMap[currentBatchStartTick].insert(newId);
                    logicTimer[newId] = 0;
                    lastSeen[newId] = currentTick;
                    isAlive[newId] = true;
                    
                    std::cout << "    ID " << newId << " assigned to batch " 
                              << currentBatchStartTick << std::endl;
                }
                newIdsThisTick.clear();
            } else if (inContinuousSpawn) {
                // No new spawns this tick, end current continuous spawn sequence
                hasActivity = true;
                std::cout << "  Ending continuous spawn sequence, batch start tick=" 
                          << currentBatchStartTick << std::endl;
                inContinuousSpawn = false;
                currentBatchStartTick = -1;
            }
            
            // 2. Detect deaths: IDs seen in previous tick but not in this tick
            std::vector<int> deadIDs;
            for (const auto& pair : lastSeen) {
                int id = pair.first;
                if (pair.second == currentTick - 1 && seenThisTick.find(id) == seenThisTick.end()) {
                    deadIDs.push_back(id);
                }
            }
            
            if (!deadIDs.empty()) {
                hasActivity = true;
                std::cout << "  Detected " << deadIDs.size() << " deaths:" << std::endl;
            }
            
            // 3. Process death events
            for (int deadID : deadIDs) {
                // Record logic timer at death
                int lifespan = logicTimer[deadID];
                lifespanDistribution[lifespan]++;
                
                std::cout << "    ID " << deadID << " died, logic lifespan=" << lifespan 
                          << ", batch=" << spawnTick[deadID] << std::endl;
                
                isAlive[deadID] = false;
                
                // Reset other IDs in the same batch
                int batch = spawnTick[deadID];
                int resetCount = 0;
                for (int batchID : batchMap[batch]) {
                    if (batchID != deadID && isAlive[batchID]) {
                        logicTimer[batchID] = 0;
                        resetCount++;
                        std::cout << "      Reset timer for ID " << batchID << std::endl;
                    }
                }
                if (resetCount > 0) {
                    std::cout << "    Reset " << resetCount 
                              << " blazes in same batch" << std::endl;
                }
            }
            
            // 4. Update logic timers for IDs seen this tick
            for (int id : seenThisTick) {
                if (isAlive[id]) {
                    logicTimer[id]++;
                    lastSeen[id] = currentTick;
                }
            }
            
            if (hasActivity && !seenThisTick.empty()) {
                std::cout << "  Active blazes this tick: " << seenThisTick.size() << std::endl;
            }
            
            currentTick++;
            seenThisTick.clear();
            continue;
        }
        
        // Parse data line
        size_t pos = line.find("!Data ");
        if (pos != std::string::npos) {
            std::string dataPart = line.substr(pos + 6);
            std::istringstream iss(dataPart);
            int id, b_time;
            if (iss >> id >> b_time) {
                seenThisTick.insert(id);
                
                // Check if this is a new ID
                if (spawnTick.find(id) == spawnTick.end()) {
                    newIdsThisTick.push_back(id);
                } else {
                    // Update last seen tick for existing ID
                    lastSeen[id] = currentTick;
                }
            }
        }
    }
    
    // Handle blazes alive at the end of log
    int aliveAtEnd = 0;
    for (const auto& pair : isAlive) {
        if (pair.second) {
            int id = pair.first;
            int lifespan = logicTimer[id];
            lifespanDistribution[lifespan]++;
            aliveAtEnd++;
        }
    }
    
    if (aliveAtEnd > 0) {
        std::cout << "\n=== Processing Blazes Alive at Log End ===" << std::endl;
        std::cout << aliveAtEnd << " blazes alive at log end" << std::endl;
    }
    
    // Output logic lifespan distribution
    std::cout << "\n=== Logic Lifespan Distribution ===" << std::endl;
    for (const auto& pair : lifespanDistribution) {
        std::cout << pair.first << " " << pair.second << std::endl;
    }
    
    return 0;
}
