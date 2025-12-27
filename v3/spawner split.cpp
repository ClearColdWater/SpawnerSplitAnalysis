#include"spawner_utils.h"

inline void updateCleanAndOverlap(Spawner &spawner, int sparedTicks, int rodCount, bool &lavaChecked, int &lavaTicks, int &ticksToClean, int &ticksToOverlap)
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
		if(P(spawner.lavaSpawnerRate))
		{
			cleanUntil += (lavaTicks = 100 + (200 * P(spawner.badLavaSpawnerRate))) - (spawner.floorCleanedTick - (MAX_CLEAN_TICKS - ticksToClean));
			ticksToClean = MAX_CLEAN_TICKS - spawner.floorCleanedTick;
	goto recalculate;
		}
	}
	
	if(currentCleaningTicks >= lavaTicks)ticksToClean -= currentCleaningTicks - lavaTicks, lavaTicks = 0;
	else lavaTicks -= currentCleaningTicks;
	ticksToOverlap -= currentOverlappingTicks;
}

template <typename LootType>
int simulate(Spawner &spawner, int rodCount = 0, int ticksToOverlap = 300, int startingCD = 20) // pre-set 'clean until' is also in cleanUntilTick
{
	bool lavaChecked = false;
	int ticksToClean = MAX_CLEAN_TICKS - 1, lavaTicks = 0;
	constexpr int totRodCount[2] = {6, 7};
	uniform_int_distribution<int> spawnCycleCooldownDistritubion(200, 799);
	int rodRequired = totRodCount[P(eyeBreakProbability)] - rodCount, totalTicks = startingCD;
	if(rodRequired <= 0)return ticksToOverlap;
	
	updateCleanAndOverlap(spawner, startingCD, rodCount, lavaChecked, lavaTicks, ticksToClean, ticksToOverlap);
	
	int spawned = 0;
	while(rodRequired > 0)
	{
		int cleanedTicks = MAX_CLEAN_TICKS - ticksToClean;
		double currentSpawnRate = spawner.getSpawnRate(cleanedTicks), currentDriftRate = spawner.getDriftRate(cleanedTicks);
		int currentBlazeKillTime = spawner.getKillTime(cleanedTicks);
		
		int cooldown = spawnCycleCooldownDistritubion(rng), curspawned = 0;
		while(!curspawned)
		{
			for(int attempt = 0; attempt < 4; attempt++)
			{
				if(P(currentSpawnRate))
				{
					currentSpawnRate *= 0.9722122525247163;
					curspawned++;
				}
			}
			if(!curspawned) cooldown++;
		}
		spawned = min(6, spawned + curspawned);
		
		while(spawned > 0 && rodRequired > 0)
		{
			if(cooldown < currentBlazeKillTime)break;
			
			spawned--;
			if(P(currentDriftRate))continue;
			
			int drops = LootType::drop();
			rodRequired -= drops;
			rodCount += drops;
			
			cooldown -= currentBlazeKillTime;
			totalTicks += currentBlazeKillTime;
		}
		
		if(rodRequired <= 0)break;
		totalTicks += cooldown;
		
		if(spawned == 0)updateCleanAndOverlap(spawner, cooldown, rodCount, lavaChecked, lavaTicks, ticksToClean, ticksToOverlap);
	}
	return totalTicks + ticksToOverlap;
}

template <typename LootType>
vector<int> generateSpawnerTimeCostDistributionSingleThread(int iterations, Spawner &spawner, int defaultBlazeKillTime = 48, int rodCount = 0, int ticksToOverlap = 300, int startingKilled = 0)
{
	int localDist[MAX_TOTAL_TICKS] = {0};
	uniform_int_distribution<int> spawnCycleCooldownDistritubion(200, 799);
	for(int i = 0; i < iterations; i++)
	{
		if(i % resampleIterations == 0)spawner.resample();
		int startingCD = startingKilled ? spawnCycleCooldownDistritubion(rng) - startingKilled * defaultBlazeKillTime : 20;
		localDist[simulate<LootType>(spawner, rodCount, ticksToOverlap, startingCD)]++;
	}
	vector<int> SpawnerTimeCostDistribution(localDist, localDist + MAX_TOTAL_TICKS);
    return SpawnerTimeCostDistribution;
}

template <typename LootType>
vector<int> generateSpawnerTimeCostDistributionMultiThread(int iterations, Spawner &spawner, int defaultBlazeKillTime = 48, int rodCount = 0, int ticksToOverlap = 300, int startingKilled = 0)
{
	vector<int> results[Threads], ret(MAX_TOTAL_TICKS, 0);
	future<vector<int>> futures[Threads];
	Spawner newSpawner[Threads];
	for(int i = 0; i < Threads; i++)
	{
		newSpawner[i] = spawner;
		futures[i] = async(launch::async, generateSpawnerTimeCostDistributionSingleThread<LootType>, 
			iterations / Threads + (i < iterations % Threads), 
			ref(newSpawner[i]), 
			defaultBlazeKillTime, 
			rodCount,
			ticksToOverlap, 
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
	Spawner OpenSingleBed, OpenIronPick, ClosedSingleBed, ClosedIronPick;
	OpenSingleBed.readData("blaze 1 bed open remade extracted.txt");
	OpenSingleBed.readStrategy("blaze 1 bed open solved.txt");
	//OpenSingleBed.generateCompareStrategy();
	
	//ClosedSingleBed.readData("blaze 1 bed closed extracted.txt");
	//OpenIronPick.readData("blaze iron pick open extracted.txt");
	//ClosedIronPick.readData("blaze iron pick closed extracted.txt");
	
	vector<int> output = generateSpawnerTimeCostDistributionMultiThread<Looting0>(100000000, OpenSingleBed, 48, 5, 300, 2); /// ro 366.76
	ofstream fout("test.txt");
	fout << output.size() << '\n';
	for(uint64_t i = 0; i < output.size(); i++)fout << output[i] << ' ';
	fout << endl;
	fout.close();/**/
	return 0;
}

