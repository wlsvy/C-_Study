//string 내 또 다른 string 을 포함하고 있는지 확인하는 알고리즘. 시간복잡도 O(n+m)

vector<int> getPi(string p)
{
	int m = (int)p.size(), j = 0; vector<int> pi(m, 0);
	for (int i = 1; i < m; i++) { 
		while (j > 0 && p[i] != p[j]) 
			j = pi[j - 1]; 
		if (p[i] == p[j]) 
			pi[i] = ++j; 
	} 
	return pi;
} 

vector<int> kmp(string s, string p) { 
	vector<int> ans; 
	auto pi = getPi(p); 
	int n = (int)s.size(), m = (int)p.size(), j = 0; 
	for (int i = 0; i < n; i++) { 
		while (j > 0 && s[i] != p[j]) 
			j = pi[j - 1]; 
		if (s[i] == p[j]) { 
			if (j == m - 1) { 
				ans.push_back(i - m + 1); 
				j = pi[j]; } 
			else { 
				j++; 
			} 
		} 
	} 
	return ans; 
}

출처: https://bowbowbow.tistory.com/6 [멍멍멍]