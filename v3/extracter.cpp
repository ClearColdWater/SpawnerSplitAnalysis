#include<bits/stdc++.h>
using namespace std;

string NAME = "blaze iron pick closed";
vector<double> Y[605];
double avg[605] = {0}, st[605] = {0};

inline double sq(double x){return x * x;}
int main()
{
	int n = 0;
	
	ifstream fin(NAME + ".log");
	
	string temp;
	while(!fin.eof())
	{
		getline(fin, temp);
		
		int pos = temp.find("[CHAT]");
		if(pos == -1)continue;
		
		istringstream iss(temp.substr(pos + 7));
		int x;
		double y;
		iss >> x >> y;
//		if(x < 101);
//		else if(101 <= x && x < 225)y *= 0.713;
//		else if(225 <= x && x < 296)y *= 0.713 + (0.797 - 0.713) * (x - 225) / (296 - 225);
//		else if(296 <= x && x < 376)y *= 0.797 + (0.882 - 0.797) * (x - 296) / (376 - 296);
//		else if(376 <= x && x < 455)y *= 0.882 + (0.967 - 0.882) * (x - 376) / (455 - 376);
//		else y *= 0.967;
		Y[x].push_back(y);
		avg[x] += y;
		if(x == 1)n++;
	}
	
	for(int i = 1; i <= 500; i++) avg[i] /= n;
	cout << n << endl;
	
	ofstream rawout(NAME + " raw.txt");
	for(int i = 0; i < n; i++)
	{
		for(int j = 1; j <= 500; j++)
		{ 
			if(!Y[j].empty())rawout << Y[j][i] << ' ';
			else rawout << Y[j - 1][i] << ' ';
		}
			
		rawout << '\n';
	}
		
	for(int i = 1; i <= 499; i++)
	{
		for(int j = 0; j < n; j++)
			st[i] += sq(Y[i][j] - avg[i]);
		
		st[i] = sqrt(st[i] / n);
		rawout << endl;
	}
	
	ofstream fout(NAME + " extracted.txt");
	
	constexpr double d[4] = {(38.0 + 1) / (122 + 2), (5.0 + 1) / (83 + 2), (3.0 + 1) / (94 + 2), (0.0 + 1) / (80 + 2)};
	constexpr double k[4] = {75.75, 55.37, 55.54, 51.14};
	for(int i = 1; i <= 500; i++)
	{
		fout << avg[i]  << ' ' << st[i] << ' ';
//		if(i < 101)fout << "0 48\n";
//		else if(101 <= i && i < 225) fout << d[0] << ' ' << static_cast<int>(k[0] + 0.5) << '\n';
//		else if(225 <= i && i < 296) fout << d[0] + (d[1] - d[0]) * (i - 225) / (296 - 225) << ' ' << static_cast<int>(k[0] + (k[1] - k[0]) * (i - 225) / (296 - 225) + 0.5) << '\n';
//		else if(296 <= i && i < 376) fout << d[1] + (d[2] - d[1]) * (i - 296) / (376 - 296) << ' ' << static_cast<int>(k[1] + (k[2] - k[1]) * (i - 296) / (376 - 296) + 0.5) << '\n';
//		else if(376 <= i && i < 455) fout << d[2] + (d[3] - d[2]) * (i - 376) / (455 - 376) << ' ' << static_cast<int>(k[2] + (k[3] - k[2]) * (i - 376) / (455 - 376) + 0.5) << '\n';
//		else fout << d[3] << ' ' << static_cast<int>(k[3] + 0.5) << '\n';
		fout << "0 48\n";
	}
	return 0; 
}

