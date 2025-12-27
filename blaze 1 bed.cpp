#include<bits/stdc++.h>
using namespace std;

string NAME = "blaze 1 bed open remade";
vector<int> Y[605];
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
		int x, y;
		iss >> x >> y;
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
			rawout << Y[j][i] << ' ';
		rawout << '\n';
	}
		
	for(int i = 1; i <= 500; i++)
	{
		for(int j = 0; j < n; j++)
			st[i] += sq(Y[i][j] - avg[i]);
		
		st[i] = sqrt(st[i] / n);
		rawout << endl;
	}
	
	ofstream fout(NAME + " extracted.txt");
	for(int i = 1; i <= 500; i++)fout << avg[i] << ' ' << st[i] << endl;
	return 0; 
}

