#include <algorithm>
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

struct MatchingState {
  vector<char> used_x;
  vector<char> used_y;
  vector<pair<int, int>> matching;

  explicit MatchingState(int s) : used_x(s + 1), used_y(s + 1) {}

  void reset() {
    ranges::fill(used_x, 0);
    ranges::fill(used_y, 0);
    matching.clear();
  }
};

struct ShiftedGraph {
  int s;
  vector<pair<int, int>> antipath;

  generator<const pair<int, int> &> edges() const {
    int x = 1;
    for (auto [x0, y0] : antipath) {
      for (; x <= x0; ++x) {
        for (int y = 1; y <= y0; ++y) {
          co_yield {x, y};
        }
      }
    }
  }

  bool lies_inside(const ShiftedGraph &other) const {
    set<pair<int, int>> other_edges;
    for (const auto &pr : other.edges()) {
      other_edges.insert(pr);
    }
    for (const auto &pr : edges()) {
      if (!other_edges.contains(pr)) {
        return false;
      }
    }
    return true;
  }
};

generator<ShiftedGraph &&> gen_graphs(int s, int current_x, int max_y) {
  // cerr << "Generating graph " << s << " " << edge_budget << " " << current_x
  // << " " << max_y << "\n";
  if (current_x >= s || max_y <= 0) {
    co_yield ShiftedGraph{s, {}};
  } else {
    for (int x = current_x; x <= s; ++x) {
      for (int y = 1; y <= max_y; ++y) {
        for (auto &&graph : gen_graphs(s, x + 1, y - 1)) {
          graph.antipath.emplace_back(x, y);
          co_yield std::move(graph);
        }
      }
    }
  }
}

bool gen_matching(input_iterator auto graphs_begin,
                  input_iterator auto graphs_end, MatchingState &state) {
  if (graphs_begin == graphs_end) {
    return true;
  }
  int x = 1;
  for (auto [x0, y0] : (graphs_begin++)->get().antipath) {
    for (; x <= x0; ++x) {
      if (state.used_x[x]) {
        continue;
      }
      for (int y = 1; y <= y0; ++y) {
        if (state.used_y[y]) {
          continue;
        }
        state.used_x[x] = true;
        state.used_y[y] = true;
        state.matching.emplace_back(x, y);
        if (gen_matching(graphs_begin, graphs_end, state)) {
          return true;
        }
        state.matching.pop_back();
        state.used_x[x] = false;
        state.used_y[y] = false;
      }
    }
  }
  return false;
}

generator<vector<int> &&> gen_size_sequence(int s, int leftover, int min_size) {
  if (leftover <= 0) {
    co_yield {};
  } else {
    for (int size = min_size; size <= s * s; ++size) {
      for (auto &&sequence : gen_size_sequence(s, leftover - 1, size)) {
        sequence.push_back(size);
        co_yield sequence;
        sequence.pop_back();
      }
    }
  }
}

generator<vector<reference_wrapper<const ShiftedGraph>>>
gen_graph_sequences(input_iterator auto size_begin,
                    input_iterator auto size_end,
                    const vector<vector<ShiftedGraph>> &graphs_by_size) {
  if (size_begin == size_end) {
    co_yield {};
  } else {
    // cerr << "Trying to generate graph sequences" << "\n";
    for (auto &&sequence :
         gen_graph_sequences(next(size_begin), size_end, graphs_by_size)) {
      for (auto &&graph : graphs_by_size[*size_begin]) {
        sequence.push_back(graph);
        co_yield sequence;
        sequence.pop_back();
      }
    }
  }
}

void print_graph_sequence(
    const vector<reference_wrapper<const ShiftedGraph>> &graph_sequence,
    vector<pair<int, int>> matching = {}) {
  cout << endl;
  // cout << "ANTIPATHS:\n";
  // for (const auto &graph : graph_sequence) {
  //   for (const auto &[x, y] : graph.get().antipath) {
  //     cout << format("({},{})", x, y) << " ";
  //   }
  //   cout << endl;
  // }
  int s = graph_sequence[0].get().s;
  string alphabet = "ABCDEFGHIJKLMNOPQRST";
  for (int y = s + 1; y >= 0; --y) {
    cout << (y >= 1 ? alphabet[y - 1] : '.') << '|';
    for (const auto &[i, graph] :
         graph_sequence | views::enumerate | views::reverse) {
      auto [xm, ym] = (i < matching.size() ? matching[i] : pair{-1, -1});
      int s = graph.get().s;
      int last_x0 = 1;
      for (auto [x0, y0] : graph.get().edges()) {
        if (y0 == y) {
          for (; last_x0 < x0; ++last_x0) {
            cout << ' ';
          }
          cout << (x0 == xm && y0 == ym ? 'x' : '@');
          ++last_x0;
        }
      }
      for (; last_x0 <= s; ++last_x0) {
        if (1 <= y && y <= s) {
          cout << " ";
        } else if (y > s) {
          cout << "-";
        } else {
          cout << last_x0 % 10;
        }
      }
      cout << '|';
    }
    cout << "\n";
  }
}

bool check_embedded(
    const vector<reference_wrapper<const ShiftedGraph>> &graph_sequence) {
  for (unsigned i = 0; i + 1 < graph_sequence.size(); ++i) {
    if (!graph_sequence[i + 1].get().lies_inside(graph_sequence[i].get())) {
      return false;
    }
  }
  return true;
}

void draw_graph(std::vector<int> size_sequence,
                const vector<vector<ShiftedGraph>> &graphs_by_size) {
  cout << "sequence {";
  for (auto it = size_sequence.begin(); next(it) != size_sequence.end(); ++it) {
    cout << *it << ',';
  }
  cout << size_sequence.back();
  cout << '}' << "\n";
  cout << "doesn't admit a matching, corresponding sequences:\n";
  int s = size_sequence.size();
  MatchingState matching(s);
  for (const auto &graph_sequence : gen_graph_sequences(
           size_sequence.begin(), size_sequence.end(), graphs_by_size)) {
    bool ok = true;
    vector<pair<int, int>> first_matching;
    for (int i = 0; i < graph_sequence.size(); ++i) {
      vector<reference_wrapper<const ShiftedGraph>> subsequence;
      for (int j = 0; j < graph_sequence.size(); ++j) {
        if (i != j) {
          subsequence.push_back(graph_sequence[j]);
        }
      }
      matching.reset();
      if (!gen_matching(subsequence.begin(), subsequence.end(), matching)) {
        ok = false;
        break;
      }
      if (i + 1 == graph_sequence.size()) {
        first_matching = matching.matching;
      }
    }
    if (!ok) {
      continue;
    }
    matching.reset();
    if (!gen_matching(graph_sequence.begin(), graph_sequence.end(), matching)) {
      print_graph_sequence(graph_sequence, first_matching);
      // break;
      // cout << "BREAKING BREAKING BREAKING" << "\n";
    } else {
    }
  }
}

int main() {
  int s;
  {
    string tmp;
    getline(cin, tmp);
    s = stoi(tmp);
  }
  vector<vector<ShiftedGraph>> graphs_by_size(s * s + 1);
  for (auto &&graph : gen_graphs(s, 1, s)) {
    ranges::reverse(graph.antipath);
    auto edges = graph.edges();
    graphs_by_size[std::ranges::distance(edges.begin(), edges.end())].push_back(
        move(graph));
  }
  string line;
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
    draw_graph(current, graphs_by_size);
    cout << "-------------------------" << endl;
  }
}
