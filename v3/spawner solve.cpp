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
int simulate(Spawner &spawner, int rodCount = 0, int ticksToOverlap = 300, int ticksToClean = MAX_CLEAN_TICKS - 1, int startingCD = 20) // pre-set 'clean until' is also in cleanUntilTick
{
	bool lavaChecked = false;
	int lavaTicks = 0;
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
vector<int> generateSpawnerTimeCostDistributionSingleThread(int iterations, Spawner &spawner, int defaultBlazeKillTime = 48, int rodCount = 0, int ticksToOverlap = 300, int ticksToClean = MAX_CLEAN_TICKS - 1, int startingKilled = 0)
{
	int localDist[MAX_TOTAL_TICKS] = {0};
	uniform_int_distribution<int> spawnCycleCooldownDistritubion(200, 799);
	for(int i = 0; i < iterations; i++)
	{
		if(i % resampleIterations == 0)spawner.resample();
		int startingCD = startingKilled ? spawnCycleCooldownDistritubion(rng) - startingKilled * defaultBlazeKillTime : 20;
		localDist[simulate<LootType>(spawner, rodCount, ticksToOverlap, ticksToClean, startingCD)]++;
	}
	vector<int> SpawnerTimeCostDistribution(localDist, localDist + MAX_TOTAL_TICKS);
    return SpawnerTimeCostDistribution;
}

template <typename LootType>
double generateSpawnerTimeCostAverageMultiThread(int iterations, Spawner &spawner, int defaultBlazeKillTime = 48, int rodCount = 0, int ticksToOverlap = 300, int ticksToClean = MAX_CLEAN_TICKS - 1, int startingKilled = 0)
{
	vector<int> results[Threads];
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
			ticksToClean, 
			startingKilled
		);
	}
	
	long double ret = 0;
	for(int i = 0; i < Threads; i++)
	{
		results[i] = futures[i].get();
		for(int j = 0; j < MAX_TOTAL_TICKS; j++)
			ret += results[i][j] * j;
	}
	return ret / iterations;
}

string saveTo;
template <typename LootType>
void solveSpawnerFor(int iterations, Spawner &spawner, int defaultBlazeKillTime = 48, int maxTicksToOverlap = 500, int maxTicksToClean = MAX_CLEAN_TICKS - 1, int startingKilled = 0)
{
	const int realIterations = iterations; iterations = 1;
	constexpr int maxRodCount = 7;
	
	ofstream fout(saveTo);
	for(int rodCount = maxRodCount; rodCount >= 0 ; rodCount--)
	{
		if(rodCount < maxRodCount)iterations = realIterations;
		for(int secondsToClean = 0; secondsToClean <= maxTicksToClean / 20 + 1; secondsToClean++)
		{
			for(int secondsToOverlap = 0; secondsToOverlap <= maxTicksToOverlap / 20; secondsToOverlap++)
			{
				spawner.cleanUntilSecond[rodCount][secondsToClean][secondsToOverlap] = 0;
				double average_0 = generateSpawnerTimeCostAverageMultiThread<LootType>(iterations, spawner, defaultBlazeKillTime, rodCount, secondsToOverlap * 20, secondsToClean * 20, startingKilled);
				bool zeroIsBetter = true;
				
				int l = 0, r = secondsToClean;
				while(l < r)
				{
					int t = (r - l) / 3;
					int m1 = l + t, m2 = r - t;
					
					spawner.cleanUntilSecond[rodCount][secondsToClean][secondsToOverlap] = m1;
					double average_m1 = generateSpawnerTimeCostAverageMultiThread<LootType>(iterations, spawner, defaultBlazeKillTime, rodCount, secondsToOverlap * 20, secondsToClean * 20, startingKilled);
					
					spawner.cleanUntilSecond[rodCount][secondsToClean][secondsToOverlap] = m2;
					double average_m2 = generateSpawnerTimeCostAverageMultiThread<LootType>(iterations, spawner, defaultBlazeKillTime, rodCount, secondsToOverlap * 20, secondsToClean * 20, startingKilled);
					if(min(average_m1, average_m2) < average_0)zeroIsBetter = false;
					
					if(average_m1 < average_m2)r = m2 - 1;
					else l = m1 + 1;
					cout << l << ' ' << r << ' ' << average_m1 << ' ' << average_m2 << endl;
				}
				fout << (spawner.cleanUntilSecond[rodCount][secondsToClean][secondsToOverlap] = (!zeroIsBetter) * l) << ' ';
			}
			fout << endl;
		}
		fout << endl;
	}
}

int main()
{
	Spawner OpenSingleBed, OpenIronPick, ClosedSingleBed, ClosedIronPick;
	/*OpenSingleBed.readData("blaze 1 bed open remade extracted.txt");
	saveTo = "blaze 1 bed open remade solved.txt";
	solveSpawnerFor<Looting0>(1000000, OpenSingleBed, 48, 500, MAX_CLEAN_TICKS - 1, 3);*/
	
	ClosedSingleBed.readData("blaze 1 bed closed extracted.txt");
	saveTo = "blaze 1 bed closed solved.txt";
	solveSpawnerFor<Looting0>(1000000, ClosedSingleBed, 48, 500, MAX_CLEAN_TICKS - 1, 3);
	
	/*OpenIronPick.readData("blaze iron pick open extracted.txt");
	saveTo = "blaze iron pick open solved.txt";
	solveSpawnerFor<Looting0>(1000000, OpenIronPick, 48, 500, MAX_CLEAN_TICKS - 1, 3);
	
	ClosedIronPick.readData("blaze iron pick closed extracted.txt");
	saveTo = "blaze iron pick closed solved.txt";
	solveSpawnerFor<Looting0>(1000000, ClosedIronPick, 48, 500, MAX_CLEAN_TICKS - 1, 3);*/
	
	/*double cur_best = 1e8;
	int cur_best_t = -1;
	for(int t = 200; t < MAX_CLEAN_TICKS; t++)
	{
		ClosedSingleBed.actualMaxCleanTicks = t;
		vector<int> output = generateSpawnerTimeCostDistributionMultiThread<Looting0>(10000000, ClosedSingleBed, 48, 300, 0, 1);
		double s = 0;
		for(uint64_t i = 0; i < output.size(); i++)s += static_cast<double>(i) * output[i] / 1e7;
		if(s < cur_best)cur_best = s, cur_best_t = t;
		cout << t << ' ' << s << ' ' << cur_best << endl;
	}
	cout << cur_best_t << ' ' << cur_best << endl;*/
	
	// solveSpawnerFor<Looting0>(1000000, OpenSingleBed, 48, 500, MAX_CLEAN_TICKS - 1, 3);
	
	/*vector<int> output = generateSpawnerTimeCostDistributionMultiThread<Looting0>(100000000, OpenSingleBed, 48, 300, 6, 4); /// ro 366.76
	ofstream fout("test.txt");
	fout << output.size() << '\n';
	for(uint64_t i = 0; i < output.size(); i++)fout << output[i] << ' ';
	fout << endl;
	fout.close();*/
	return 0;
}

