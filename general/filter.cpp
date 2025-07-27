#include <iostream>
#include <set>
#include <sstream>
#include <vector>

using namespace std;

int main() {
  set<vector<int>, greater<vector<int>>> minimums;
  string line;

  auto better = [](const vector<int> &x, const vector<int> &y) {
    for (int i = 0; i < x.size(); ++i) {
      if (x[i] < y[i]) {
        return false;
      }
    }
    return true;
  };

  while (getline(cin, line)) {
    istringstream ss(line);
    int n;
    vector<int> current;
    while (ss >> n) {
      current.push_back(n);
    }
    cerr << "checking ";
    for (const auto &x : current) {
      cerr << x << " ";
    }
    cerr << endl;
    bool best = true;
    for (const auto &kek : minimums) {
      if (better(kek, current)) {
        best = false;
      }
    }
    if (best) {
      vector<vector<int>> to_erase;
      for (const auto &kek : minimums) {
        if (better(current, kek)) {
          to_erase.push_back(kek);
        }
      }
      for (auto &&v : to_erase) {
        minimums.erase(std::move(v));
      }
      minimums.insert(move(current));
    }
  }
  for (const auto &s : minimums) {
    for (const int &x : s) {
      cout << x << " ";
    }
    cout << endl;
  }
}
