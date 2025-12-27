#include<bits/stdc++.h>
using namespace std;
int main()
{
	int tick, count;
	int totTick = 0, totCount = 0;
	while(cin >> tick >> count)totTick += tick * count, totCount += count;
	cout << totTick << ' ' << totCount << ' ' << static_cast<double>(totTick) / totCount << endl;
	return 0;
}

