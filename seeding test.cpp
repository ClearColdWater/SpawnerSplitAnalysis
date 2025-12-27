#include<bits/stdc++.h>
using namespace std;
int main()
{
	uint32_t ans = 0;
	mt19937 rng(12345);
	for(int i = 0; i < 1e8; i++)
	{
		
		ans ^= rng();
	}
	cout << ans << endl;
	return 0;
}

