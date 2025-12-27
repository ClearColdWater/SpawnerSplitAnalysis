#include"spawner_utils.h"

struct EntropyPool
{
	static constexpr int MAX_CYCLES = 1024;
	struct SpawnCycleEntropy
	{
		double base_u[4];
		double corr_check[4];
		double independent_u[4];
		
		bool spawnerAFloorCleaned;
		int cooldown;
	}spawnCycleEntropyPool[MAX_CYCLES];
	
	double rates[32];
	int killedPointer = 0;
	inline double getRates() { return rates[killedPointer++]; }
	EntropyPool() { for(int i = 20; i < 32; i++)rates[i] = 0; } // smaller = higher chance to drop
	
	int startPointer = 0, usagePointer = MAX_CYCLES - 1, A_UsagePointer = MAX_CYCLES - 1;
	inline void refillCycle(int p)
	{
		uniform_real_distribution<double> dist(0, 1);
		uniform_int_distribution<int> spawnCycleCooldownDistritubion(200, 799);
		for(int i = 0; i < 4; i++)
		{
			spawnCycleEntropyPool[p].base_u[i] = dist(rng);
			spawnCycleEntropyPool[p].corr_check[i] = dist(rng);
			spawnCycleEntropyPool[p].independent_u[i] = dist(rng);
		}
		// spawnCycleEntropyPool[p].spawnerAFloorCleaned = false; // if its not initialized then it literally does not matter anyway
		spawnCycleEntropyPool[p].cooldown = spawnCycleCooldownDistritubion(rng);
	}
	
	inline SpawnCycleEntropy& getNextCycleEntropy() 
	{ 
	    SpawnCycleEntropy& ret = spawnCycleEntropyPool[usagePointer];
	    
	    usagePointer = (usagePointer + 1) & (MAX_CYCLES - 1);
	    // cout << usagePointer << endl;
		return ret;
	}
	
	void resetPointer() { killedPointer = 0, A_UsagePointer = usagePointer, usagePointer = startPointer; }
	void refillPool()
	{
		uniform_real_distribution<double> dist(0, 1);
		
		usagePointer = max(A_UsagePointer, usagePointer);
		int endpoint = usagePointer + MAX_CYCLES * (usagePointer < startPointer);
		for(int i = startPointer; i < endpoint; i++)
		{
			int real_i = i & (MAX_CYCLES - 1);
			refillCycle(real_i);
		}
		killedPointer = min(20, killedPointer);
		for(int i = 0; i < killedPointer; i++) rates[i] = dist(rng);
		killedPointer = 0;
		usagePointer = startPointer;
	}
};

inline void updateCleanAndOverlap(Spawner &spawner, int sparedTicks, int rodCount, bool &lavaChecked, int &lavaTicks, int &ticksToClean, int &ticksToOverlap, double lavaSpawnerh, double badLavaSpawnerh)
{
	int currentCleaningTicks = 0, currentOverlappingTicks = 0;
	int cleanUntil = spawner.cleanUntilSecond[rodCount][ticksToClean / 20][ticksToOverlap / 20] * 20 + lavaTicks;
	
	recalculate: 
		if(sparedTicks <= cleanUntil)currentCleaningTicks = sparedTicks; // ticksToClean -= sparedTicks;
		else if(sparedTicks <= cleanUntil + ticksToOverlap)currentCleaningTicks = cleanUntil, currentOverlappingTicks = sparedTicks - cleanUntil; // ticksToClean -= cleanUntil, ticksToOverlap -= sparedTicks - cleanUntil;
		else if(sparedTicks <= ticksToClean + ticksToOverlap)currentCleaningTicks = sparedTicks - ticksToOverlap, currentOverlappingTicks = ticksToOverlap; // ticksToClean -= sparedTicks - ticksToOverlap, ticksToOverlap = 0;
		else currentCleaningTicks = ticksToClean, currentOverlappingTicks = ticksToOverlap; // ticksToClean = ticksToOverlap = 0;
		
	if(!lavaChecked && MAX_CLEAN_TICKS - ticksToClean < spawner.floorCleanedTick && MAX_CLEAN_TICKS - (ticksToClean - currentCleaningTicks) >= spawner.floorCleanedTick)
	{
		lavaChecked = true;
		if(P(spawner.lavaSpawnerRate, lavaSpawnerh))
		{
			cleanUntil += (lavaTicks = 100 + (200 * P(spawner.badLavaSpawnerRate, badLavaSpawnerh))) - (spawner.floorCleanedTick - (MAX_CLEAN_TICKS - ticksToClean));
			ticksToClean = MAX_CLEAN_TICKS - spawner.floorCleanedTick;
	goto recalculate;
		}
	}
	
	if(currentCleaningTicks >= lavaTicks)ticksToClean -= currentCleaningTicks - lavaTicks, lavaTicks = 0;
	else lavaTicks -= currentCleaningTicks;
	ticksToOverlap -= currentOverlappingTicks;
}

template <typename LootType>
int simulateForComparison(EntropyPool &entropySource, Spawner &spawner, bool isSpawnerA, int totRodRequired, int ticksToOverlap = 300, int rodCount = 0, int startingCD = 20, double lavaSpawnerh = 0, double badLavaSpawnerh = 0)
{
	bool lavaChecked = false;
	int ticksToClean = MAX_CLEAN_TICKS - 1, lavaTicks = 0;
	
	int rodRequired = totRodRequired - rodCount;
	int totalTicks = startingCD;
	
	if(rodRequired <= 0)return ticksToOverlap;
	updateCleanAndOverlap(spawner, startingCD, rodCount, lavaChecked, lavaTicks, ticksToClean, ticksToOverlap, lavaSpawnerh, badLavaSpawnerh);

	// cout << "A" << endl;
	int spawned = 0;
	while(rodRequired > 0)
	{
		// cout << "B" << endl;
		
		int cleanedTicks = MAX_CLEAN_TICKS - ticksToClean;
		double currentSpawnRate = spawner.getSpawnRate(cleanedTicks), currentDriftRate = spawner.getDriftRate(cleanedTicks);
		int currentBlazeKillTime = spawner.getKillTime(cleanedTicks);
		
		// cout << "C" << endl;
		int cooldown, curspawned = 0;
		while(!curspawned)
		{
			EntropyPool::SpawnCycleEntropy& spawnCycleEntropy = entropySource.getNextCycleEntropy();
			cooldown = spawnCycleEntropy.cooldown;
			bool floorCleaned = cleanedTicks >= spawner.floorCleanedTick;
			for(int attempt = 0; attempt < 4; attempt++)
			{
				bool pass = false;
				if(isSpawnerA)
				{
					pass = spawnCycleEntropy.base_u[attempt] < currentSpawnRate;
					spawnCycleEntropy.spawnerAFloorCleaned = floorCleaned;
				}
				else 
				{
					bool ret[2] = {spawnCycleEntropy.base_u[attempt] < currentSpawnRate, spawnCycleEntropy.independent_u[attempt] < currentSpawnRate};
					pass = ret[spawnCycleEntropy.corr_check[attempt] < 0.2 + (spawnCycleEntropy.spawnerAFloorCleaned | floorCleaned) * 0.6];
				}
				
				if(pass)
				{
					currentSpawnRate *= 0.9722122525247163;
					curspawned++;
				}
			}
			if(!curspawned) cooldown++;
		}
		spawned = min(6, spawned + curspawned);
		// cout << "D" << ' ' << cooldown << endl;
		
		while(spawned > 0 && rodRequired > 0)
		{
			if(cooldown < currentBlazeKillTime)break;
			
			spawned--;
			// cout << cleanedTicks << ' ' << currentDriftRate << endl;
			if(P(currentDriftRate))continue; // drift rate assumed independent
			
			int drops = LootType::drop(entropySource.getRates());
			// cout << drops << endl;
			rodRequired -= drops;
			rodCount += drops;
			
			cooldown -= currentBlazeKillTime;
			totalTicks += currentBlazeKillTime;
		}
		// cout << "E" << endl;
		
		if(rodRequired <= 0)break;
		totalTicks += cooldown;
		
		// cout << "F" << ' ' << rodRequired << endl;
		if(spawned == 0)updateCleanAndOverlap(spawner, cooldown, rodCount, lavaChecked, lavaTicks, ticksToClean, ticksToOverlap, lavaSpawnerh, badLavaSpawnerh);
	}
	// cout << "G" << endl;
	return totalTicks + ticksToOverlap;
}

template <typename LootType>
vector<int> generateSpawnerTimeComparisonDistributionSingleThread(int iterations, Spawner &spawnerA, Spawner &spawnerB, int blazeKillTimeA = 48, int blazeKillTimeB = 48, int ticksToOverlapA = 300, int ticksToOverlapB = 300, int rodCountA = 0, int rodCountB = 0, int startingKilled = 0)
{
	rng = mt19937_64(random_device{}());
	int localDist[MAX_TOTAL_TICKS] = {0};
	constexpr int totRodCount[2] = {6, 7};
	uniform_int_distribution<int> spawnCycleCooldownDistritubion(200, 799);
	uniform_real_distribution<double> dist(0, 1);
	EntropyPool entropyPool;
	for(int i = 0; i < iterations; i++)
	{
		entropyPool.refillPool();
		int rodRequired = totRodCount[P(eyeBreakProbability)], startingCD = spawnCycleCooldownDistritubion(rng);
		int startingCDA = startingKilled ? startingCD - startingKilled * blazeKillTimeA : 20;
		int startingCDB = startingKilled ? startingCD - startingKilled * blazeKillTimeB : 20;
		
		double lavaSpawnerh = dist(rng), badLavaSpawnerh = dist(rng);
		
		if(i % resampleIterations == 0)spawnerA.resample(), spawnerB.resample();
		int timeA = simulateForComparison<LootType>(entropyPool, spawnerA,  true, rodRequired, ticksToOverlapA, rodCountA, startingCDA, lavaSpawnerh, badLavaSpawnerh);
		entropyPool.resetPointer();
		int timeB = simulateForComparison<LootType>(entropyPool, spawnerB, false, rodRequired, ticksToOverlapB, rodCountB, startingCDB, lavaSpawnerh, badLavaSpawnerh);
		// cout << timeA << ' ' << timeB << endl;
		localDist[timeB - timeA + (MAX_TOTAL_TICKS >> 1)]++;
		// localDist[timeA]++;
	}
	vector<int> SpawnerTimeCostDistribution(localDist, localDist + MAX_TOTAL_TICKS);
    return SpawnerTimeCostDistribution;
}

template <typename LootType>
vector<int> generateSpawnerTimeComparisonDistributionMultiThread(int iterations, Spawner &spawnerA, Spawner &spawnerB, int blazeKillTimeA = 48, int blazeKillTimeB = 48, int ticksToOverlapA = 300, int ticksToOverlapB = 300, int rodCountA = 0, int rodCountB = 0, int startingKilled = 0)
{
	vector<int> results[Threads], ret(MAX_TOTAL_TICKS, 0);
	future<vector<int>> futures[Threads];
	Spawner newSpawnerA[Threads], newSpawnerB[Threads];
	for(int i = 0; i < Threads; i++)
	{
		newSpawnerA[i] = spawnerA, newSpawnerB[i] = spawnerB;
		futures[i] = async(launch::async, generateSpawnerTimeComparisonDistributionSingleThread<LootType>, 
			iterations / Threads + (i < iterations % Threads), 
			ref(newSpawnerA[i]), 
			ref(newSpawnerB[i]), 
			blazeKillTimeA, 
			blazeKillTimeB, 
			ticksToOverlapA, 
			ticksToOverlapB, 
			rodCountA,
			rodCountB,
			startingKilled
		);
	}
	for(int i = 0; i < Threads; i++)
	{
		results[i] = futures[i].get();
		for(int j = 0; j < MAX_TOTAL_TICKS; j++)
			ret[j] += results[i][j];
	}
	return ret;
}

int main()
{
	Spawner ClosedSingleBed, ClosedIronPick;
	ClosedSingleBed.readData("blaze 1 bed open remade extracted.txt");
	ClosedSingleBed.readStrategy("blaze 1 bed open remade solved.txt");
	ClosedIronPick.readData("blaze iron pick open extracted.txt");
	ClosedIronPick.readStrategy("blaze iron pick open solved.txt");
	vector<int> output = generateSpawnerTimeComparisonDistributionMultiThread<Looting0>(100000000, ClosedSingleBed, ClosedIronPick, 48, 48, 300, 300);
	ofstream fout("blaze_1_bed_open_vs_blaze_iron_pick_open_overall.txt");
	fout << output.size() << '\n';
	for(uint64_t i = 0; i < output.size(); i++)fout << output[i] << ' ';
	fout << endl;
	fout.close();
	return 0;
}

