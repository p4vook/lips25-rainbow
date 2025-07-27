#include <cassert>
#include <format>
#include <generator>
#include <iostream>
#include <iterator>
#include <numeric>
#include <set>
#include <sstream>
#include <thread>
#include <utility>
#include <vector>

using namespace std;

struct ShiftedGraph {
  int s;
  vector<pair<int, int>> antipath;

  generator<const pair<int, int> &> edges(vector<char> *used = nullptr) const {
    int x = 1;
    for (auto [x0, y0] : antipath | views::reverse) {
      for (; x <= x0; ++x) {
        if (used && (*used)[x]) {
          continue;
        }
        for (int y = 1; y <= min(x - 1, y0); ++y) {
          co_yield {x, y};
        }
      }
    }
  }

  bool lies_inside(const ShiftedGraph &other) const {
    auto other_edges = other.edges() | ranges::to<vector>();
    set<pair<int, int>> edge_set(other_edges.begin(), other_edges.end());
    for (const auto &pr : edges()) {
      if (!edge_set.contains(pr)) {
        return false;
      }
    }
    return true;
  }
};

generator<ShiftedGraph &&> gen_graphs(int s, int edge_budget, int current_x,
                                      int max_y) {
  // cerr << "Generating graph " << s << " " << edge_budget << " " << current_x
  // << " " << max_y << "\n";
  assert(edge_budget >= 0);
  if (edge_budget == 0) {
    co_yield ShiftedGraph{s, {}};
  } else if (current_x <= 2 * s) {
    for (int x = current_x; x <= min(2 * s, current_x + edge_budget); ++x) {
      auto calc_new_edges = [&](int y) {
        int total = 0;
        // Split at y+1: vertices <= y+1 contribute (vertex-1) edges each
        // vertices > y+1 contribute y edges each

        if (current_x > y + 1) {
          // All vertices from current_x to x contribute y edges
          total = (x - current_x + 1) * y;
        } else if (x <= y + 1) {
          // All vertices contribute (vertex-1) edges
          // Sum from current_x-1 to x-1
          total = (x - current_x + 1) * (current_x + x - 2) / 2;
        } else {
          // Mixed case: split at y+1
          // Vertices [current_x, y+1] contribute sum from current_x-1 to y
          total = (y - current_x + 2) * (current_x + y - 1) / 2;
          // Vertices [y+2, x] contribute y edges each
          total += (x - y - 1) * y;
        }
        return total;
      };
      for (int y = 1;
           y <= x - 1 && y <= max_y && calc_new_edges(y) <= edge_budget; ++y) {
        for (auto &&graph :
             gen_graphs(s, edge_budget - calc_new_edges(y), x + 1, y - 1)) {
          graph.antipath.emplace_back(x, y);
          co_yield move(graph);
        }
      }
    }
  }
}

optional<vector<pair<int, int>>> gen_matching(input_iterator auto graphs_begin,
                                              input_iterator auto graphs_end,
                                              vector<char> &used) {
  if (graphs_begin == graphs_end) {
    return optional{vector<pair<int, int>>{}};
  }
  auto next_graph = std::next(graphs_begin);
  for (const auto &[x0, y0] : graphs_begin->get().edges(&used)) {
    // cerr << "Taking edge " << x0 << " " << y0 << "\n";
    if (used[x0] || used[y0]) {
      continue;
    }
    used[x0] = true;
    used[y0] = true;
    if (auto matching = gen_matching(next_graph, graphs_end, used)) {
      matching->emplace_back(x0, y0);
      return optional{matching};
    }
    used[x0] = false;
    used[y0] = false;
  }
  return {};
}

generator<vector<int> &&> gen_size_sequence(int s, int leftover, int min_size,
                                            optional<int> max_size = {}) {
  if (leftover <= 0) {
    co_yield {};
  } else {
    for (int size = min_size; size <= (max_size ? *max_size : s * (2 * s - 1));
         ++size) {
      for (auto &&sequence : gen_size_sequence(s, leftover - 1, size)) {
        sequence.push_back(size);
        co_yield sequence;
        sequence.pop_back();
      }
    }
  }
}

generator<vector<reference_wrapper<ShiftedGraph>> &&>
gen_graph_sequences(int s, input_iterator auto size_begin,
                    input_iterator auto size_end) {
  if (size_begin == size_end) {
    co_yield {};
  } else {
    // cerr << "Trying to generate graph sequences" << "\n";
    for (auto &&sequence : gen_graph_sequences(s, next(size_begin), size_end)) {
      for (auto &&graph : gen_graphs(s, *size_begin, 1, 2 * s)) {
        sequence.push_back(graph);
        co_yield sequence;
        sequence.pop_back();
      }
    }
  }
}

void print_graph_sequence(
    const vector<reference_wrapper<ShiftedGraph>> &graph_sequence,
    const optional<vector<pair<int, int>>> &matching = {}) {
  cout << "GRAPH SEQUENCE" << "\n";
  int ss = graph_sequence[0].get().s;
  for (int y = 2 * ss + 1; y >= 0; --y) {
    cout << y << '|';
    for (int i = graph_sequence.size() - 1; i >= 0; --i) {
      const auto &graph = graph_sequence[i];
      int xm = -1;
      int ym = -1;
      if (i < graph_sequence.size() - 1) {
        xm = (*matching)[graph_sequence.size() - 2 - i].first;
        ym = (*matching)[graph_sequence.size() - 2 - i].second;
      }

      int s = graph.get().s;
      int last_x0 = 1;
      for (auto [x0, y0] : graph.get().edges()) {
        if (y0 == y) {
          for (; last_x0 < x0; ++last_x0) {
            cout << ' ';
          }
          cout << (x0 == xm && y0 == ym ? '!' : '#');
          ++last_x0;
        }
      }
      for (; last_x0 <= 2 * s; ++last_x0) {
        if (1 <= y && y <= 2 * ss) {
          cout << " ";
        } else if (y > 2 * ss) {
          cout << "-";
        } else {
          cout << last_x0;
        }
      }
    }
    cout << '|';
    cout << "\n";
  }
}

bool check_embedded(
    const vector<reference_wrapper<ShiftedGraph>> &graph_sequence) {
  for (unsigned i = 0; i + 1 < graph_sequence.size(); ++i) {
    if (!graph_sequence[i].get().lies_inside(graph_sequence[i + 1].get())) {
      return false;
    }
  }
  return true;
}

void draw_graph(std::vector<int> size_sequence) {
  optional<vector<pair<int, int>>> matching;
  cout << "sequence {";
  for (auto it = size_sequence.begin(); next(it) != size_sequence.end(); ++it) {
    cout << *it << ',';
  }
  cout << size_sequence.back();
  cout << '}' << "\n";
  cout << "doesn't admit a matching, corresponding sequences:\n";
  int s = size_sequence.size();
  vector<char> used(2 * s + 1);
  for (const auto &graph_sequence :
       gen_graph_sequences(s, size_sequence.begin(), size_sequence.end())) {
    // cerr << "Checking ";
    // print_graph_sequence(graph_sequence);
    bool ok = true;
    optional<vector<pair<int, int>>> first_matching;
    for (int i = 0; i < graph_sequence.size(); ++i) {
      vector<reference_wrapper<ShiftedGraph>> subsequence;
      for (int j = 0; j < graph_sequence.size(); ++j) {
        if (i != j) {
          subsequence.push_back(graph_sequence[j]);
        }
      }
      ranges::fill(used, 0);
      auto matching =
          gen_matching(subsequence.begin(), subsequence.end(), used);
      if (!matching) {
        ok = false;
        break;
      }
      if (i == graph_sequence.size() - 1) {
        first_matching = matching;
      }
    }
    if (!ok) {
      continue;
    }
    ranges::fill(used, 0);
    if (matching =
            gen_matching(graph_sequence.begin(), graph_sequence.end(), used);
        !matching) { //      reverse((*matching).begin(), (*matching).end());
      if (!check_embedded(graph_sequence)) {
        cout << "##############################################" << endl;
        cout << "NOT EMBEDDED" << endl;
        cout << "##############################################" << endl;
      }
      print_graph_sequence(graph_sequence, first_matching);
      // cout << "BREAKING BREAKING BREAKING" << "\n";
    }
  }
}

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
    draw_graph(s);
    cout << "-------------------------" << endl;
  }
}
