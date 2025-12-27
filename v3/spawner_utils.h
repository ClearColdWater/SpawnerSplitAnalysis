#pragma once

#include<iostream>
#include<fstream>
#include<random>
#include<thread>
#include<future>
using namespace std;

thread_local mt19937_64 rng(random_device{}()); //
constexpr int Threads = 12;

constexpr int MAX_CLEAN_TICKS = 500, MAX_CLEAN_TICKS_LEN = 512, MAX_TOTAL_TICKS = 20000;
constexpr int resampleIterations = 1000;
constexpr double eyeBreakProbability = 0.2;
struct Spawner
{
	double spawnRateAvgAfteriTicks[MAX_CLEAN_TICKS_LEN], spawnRateStdAfteriTicks[MAX_CLEAN_TICKS_LEN];
	double spawnRateCurAfteriTicks[MAX_CLEAN_TICKS_LEN];
	
	double driftRateAfteriTicks[MAX_CLEAN_TICKS_LEN];
	int blazeKillTimeAfteriTicks[MAX_CLEAN_TICKS_LEN];
	
	int floorCleanedTick = MAX_CLEAN_TICKS - 1, actualMaxCleanTicks;
	double lavaSpawnerRate = 0, badLavaSpawnerRate = 0; 
	
	void readData(const string FileName)
	{
		ifstream fin(FileName);
		for(int i = 0; i < MAX_CLEAN_TICKS; i++)
		{
			fin >> spawnRateAvgAfteriTicks[i] >> spawnRateStdAfteriTicks[i] >> driftRateAfteriTicks[i] >> blazeKillTimeAfteriTicks[i];
			spawnRateAvgAfteriTicks[i] *= 0.0009136;
			spawnRateStdAfteriTicks[i] *= 0.0009136;
		}
		fin >> floorCleanedTick >> lavaSpawnerRate >> badLavaSpawnerRate; // can fail
		actualMaxCleanTicks = MAX_CLEAN_TICKS;
			
		fin.close();
	}
	
	void resample()
	{
		normal_distribution<double> dist(0, 0.5);
		double deviation = -1e18;
		while(-0.5 > deviation || deviation > 0.5)deviation = dist(rng);
		
		for(int i = 0; i < MAX_CLEAN_TICKS; i++)
			spawnRateCurAfteriTicks[i] = spawnRateAvgAfteriTicks[i] + spawnRateStdAfteriTicks[i] * deviation;
	}
	
	inline double getSpawnRate(int ticks)
	{
		double ret[2] = {spawnRateCurAfteriTicks[actualMaxCleanTicks - 1], spawnRateCurAfteriTicks[ticks & 511]};
		return ret[ticks < actualMaxCleanTicks - 1];
	}
	
	inline double getDriftRate(int ticks)
	{
		double ret[2] = {driftRateAfteriTicks[actualMaxCleanTicks - 1], driftRateAfteriTicks[ticks & 511]};
		return ret[ticks < actualMaxCleanTicks - 1];
	}
	
	inline int getKillTime(int ticks)
	{
		int ret[2] = {blazeKillTimeAfteriTicks[actualMaxCleanTicks - 1], blazeKillTimeAfteriTicks[ticks & 511]};
		return ret[ticks < actualMaxCleanTicks - 1];
	}
	
	constexpr static int MAX_OVERLAP_TICKS = 500;
	int cleanUntilSecond[8][MAX_CLEAN_TICKS / 20 + 5][MAX_OVERLAP_TICKS / 20 + 5];
	void readStrategy(const string FileName)
	{
		ifstream fin(FileName);
		for(int rodCount = 7; rodCount >= 0 ; rodCount--)
			for(int secondsToClean = 0; secondsToClean <= 499 / 20 + 1; secondsToClean++)
				for(int secondsToOverlap = 0; secondsToOverlap <= 500 / 20; secondsToOverlap++)
					fin >> cleanUntilSecond[rodCount][secondsToClean][secondsToOverlap];
	}
	void generateCompareStrategy()
	{
		for(int rodCount = 7; rodCount >= 0 ; rodCount--)
			for(int secondsToClean = 0; secondsToClean <= 499 / 20 + 1; secondsToClean++)
				for(int secondsToOverlap = 0; secondsToOverlap <= 500 / 20; secondsToOverlap++)
					cleanUntilSecond[rodCount][secondsToClean][secondsToOverlap] = secondsToClean;
	}
};

struct probabilityChecker
{
    static bool operator()(double probability) 
	{
		// if(probability <= 0.0 || probability >= 1.0)cout << probability << endl;
        bernoulli_distribution dist(probability);
        return dist(rng);
    }
    static bool operator()(double probability, double h) { return h < probability; }
};

thread_local probabilityChecker P;

struct Looting0 
{ 
	static inline int drop() { return P(1.0 / 2); }
	static inline int drop(double h) { return h < 1.0 / 2; }
}; 

struct Looting1 
{ 
	static inline int drop() { return P(3.0 / 4) * (1 + P(1.0 / 3)); }
	static inline int drop(double h) { return (h < 3.0 / 4) + (h < 1.0 / 4); }
}; 

struct Looting2 
{ 
	static inline int drop() { return P(7.0 / 8) * (1 + P(4.0 / 7) * (1 + P(1.0 / 4))); }
	static inline int drop(double h) { return (h < 7.0 / 8) + (h < 4.0 / 8) + (h < 1.0 / 8); }
}; 
struct Looting3 
{ 
	static inline int drop() { return P(11.0 / 12) * (1 + P(8.0 / 11) * (1 + P(4.0 / 8) * (1 + P(1.0 / 4)))); }
	static inline int drop(double h) { return (h < 11.0 / 12) + (h < 8.0 / 12) + (h < 4.0 / 12) + (h < 1.0 / 12); }
}; 

