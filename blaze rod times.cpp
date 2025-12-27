// 500t max to clear; 400t left to overlap; 0-6 rods left; 

#include<bits/stdc++.h>
using namespace std;

double a[505];
inline double gc(int x)
{
	if(x < 498)return (a[x] / 1000) * 0.9136;
	else return (a[498] / 1000) * 0.9136;
}

map<int, int> m;
mt19937 seeder(12345), rng(12345);

uniform_real_distribution<double> rdis(0.0, 1.0);
inline bool p(double x){ return x > rdis(rng); }

inline int sim(/*int seed*/)
{
	//rng = mt19937(seed);
	const int res[2] = {6, 7};
	uniform_int_distribution<int> tdis(200, 799);
	int rod = res[p(0.2)], t = 20, ret = 20;
	while(rod > 0)
	{
		double cur = gc(t);
		int spawned = 0;
		while(!spawned)
		{
			for(int i = 0; i < 4; i++)
			{
				//cout << cur << ' ' << t << endl;
				if(p(cur))
				{
					cur *= 0.9722122525247163;
					spawned++;
					if(p(0.5))rod--;
					if(rod == 0)break;
				}
			}
		}
		
		if(rod <= 0)
		{
			ret += 48 * spawned;
			break;
		}
		
		int nxt = tdis(rng);
		t += nxt - 48 * spawned;
		ret += nxt;
	}
	return ret + max(0, 300 - max(0, t - 498));
}

int main()
{
	ifstream fin("blaze 1 bed open remade extracted.txt"); 
	ofstream fout("test.txt");
	for(int i = 0; i < 499; i++)fin >> a[i];
	cout << a[498] << endl;
	
	for(int i = 0; i < 100000000; i++)
	{
		m[sim(/*i*/)]++;
		//cout << i << endl;
	}
	
	double ans = 0;
	for(int i = 0; i < 10000; i++)
	{
		fout << m[i] << endl;
		ans += (double)i * m[i] / 100000000;
	}
	cout << ans << endl;
	
	return 0;
}

