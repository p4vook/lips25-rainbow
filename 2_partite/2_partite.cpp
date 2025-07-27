#include <cassert>
#include <format>
#include <generator>
#include <iostream>
#include <iterator>
#include <numeric>
#include <set>
#include <thread>
#include <utility>
#include <vector>

using namespace std;

struct MatchingState {
  vector<char> used_x;
  vector<char> used_y;

  explicit MatchingState(int s) : used_x(s + 1), used_y(s + 1) {}

  void reset() {
    ranges::fill(used_x, 0);
    ranges::fill(used_y, 0);
  }
};

struct ShiftedGraph {
  int s;
  vector<pair<int, int>> antipath;

  generator<const pair<int, int> &>
  edges(MatchingState *state = nullptr) const {
    int x = 1;
    for (auto [x0, y0] : antipath | views::reverse) {
      for (; x <= x0; ++x) {
        if (state && state->used_x[x]) {
          continue;
        }
        for (int y = 1; y <= y0; ++y) {
          if (state && state->used_y[y]) {
            continue;
          }
          co_yield {x, y};
        }
      }
    }
  }
};

generator<ShiftedGraph &&> gen_graphs(int s, int current_x, int max_y) {
  // cerr << "Generating graph " << s << " " << edge_budget << " " << current_x
  // << " " << max_y << "\n";
  if (current_x >= s) {
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
  auto next_graph = std::next(graphs_begin);
  for (const auto &[x0, y0] : graphs_begin->get().edges(&state)) {
    if (state.used_x[x0] || state.used_y[y0]) {
      continue;
    }
    state.used_x[x0] = true;
    state.used_y[y0] = true;
    if (gen_matching(next_graph, graphs_end, state)) {
      return true;
    }
    state.used_x[x0] = false;
    state.used_y[y0] = false;
  }
  return {};
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
    const vector<reference_wrapper<const ShiftedGraph>> &graph_sequence) {
  // cerr << "GRAPH SEQUENCE\n";
  // cerr << "ANTIPATHS:\n";
  for (const auto &graph : graph_sequence) {
    for (const auto &[x, y] : graph.get().antipath) {
      cerr << format("({},{})", x, y) << " ";
    }
    cerr << endl;
  }
  int ss = graph_sequence[0].get().s;
  for (int y = ss + 1; y >= 0; --y) {
    cerr << y << '|';
    for (const auto &graph : graph_sequence) {
      int s = graph.get().s;
      int last_x0 = 1;
      for (auto [x0, y0] : graph.get().edges()) {
        if (y0 == y) {
          for (; last_x0 < x0; ++last_x0) {
            cerr << ' ';
          }
          cerr << '@';
          ++last_x0;
        }
      }
      for (; last_x0 <= s; ++last_x0) {
        if (1 <= y && y <= ss) {
          cerr << " ";
        } else if (y > 2 * ss) {
          cerr << "-";
        } else {
          cerr << last_x0 % 10;
        }
      }
    }
    cerr << '|';
    cerr << "\n";
  }
}

bool is_better(const vector<int> &lhs, const vector<int> &rhs) {
  for (int i = 0; i < lhs.size(); ++i) {
    if (lhs[i] < rhs[i]) {
      return false;
    }
  }
  return true;
}

void do_stuff(int s, const std::ranges::range auto &size_sequences,
              const vector<vector<ShiftedGraph>> &graphs_by_size,
              set<vector<int>> &results) {
  for (const auto &size_sequence : size_sequences) {
    bool best = true;
    for (const auto &v : results) {
      if (is_better(v, size_sequence)) {
        best = false;
        break;
      }
    }
    if (!best) {
      continue;
    }
    // cerr << "Checking sequence ";
    // for (const int &x : size_sequence) {
    // cerr << x << " ";
    // }
    // cerr << "\n";
    MatchingState state(s);
    for (const auto &graph_sequence : gen_graph_sequences(
             size_sequence.begin(), size_sequence.end(), graphs_by_size)) {
      // print_graph_sequence(graph_sequence);
      state.reset();
      if (gen_matching(graph_sequence.begin(), graph_sequence.end(), state)) {

#if 0
        cout << '{';
        for (auto it = size_sequence.begin(); next(it) != size_sequence.end();
             ++it) {
          cout << *it << ',';
        }
        cout << size_sequence.back();
        cout << '}' << "\n";
        ;
        cerr << "admits a matching" << "\n";
        for (auto [x, y] : *matching) {
          cerr << format("({}, {}) ", x, y);
        }
        cerr << "\n" << "of a corresponding ";
        print_graph_sequence(graph_sequence);
#endif
        // cout << "BREAKING BREAKING BREAKING" << "\n";
        // results->push_back(move(size_sequence));
      } else {
        vector<vector<int>> to_remove;
        for (const auto &cand : results) {
          if (is_better(size_sequence, cand)) {
            to_remove.push_back(cand);
          }
        }
        for (const auto &tmp : to_remove) {
          results.erase(tmp);
        }
        results.insert(size_sequence);
        break;
      }
    }
  }
}

int main() {
  int s;
  cin >> s;
  auto size_sequences = gen_size_sequence(s, s, 1) | ranges::to<vector>();
  vector<vector<ShiftedGraph>> graphs_by_size(s * s + 1);
  for (auto &&graph : gen_graphs(s, 1, s)) {
    auto edges = graph.edges();
    graphs_by_size[std::ranges::distance(edges.begin(), edges.end())].push_back(
        move(graph));
  }
  int thread_count =
      min((int)thread::hardware_concurrency(), (int)size_sequences.size());
  vector<set<vector<int>>> results(thread_count);
  {
    vector<jthread> threads;
    for (int i = 0; i < thread_count; ++i) {
      threads.emplace_back([&] {
        do_stuff(s,
                 size_sequences | views::reverse | views::drop(i) |
                     views::stride(thread_count),
                 graphs_by_size, results[i]);
      });
    }
  }
  set<vector<int>> total = move(results[0]);
  for (const auto &thrd : results | views::drop(1)) {
    for (const auto &seq : thrd) {
      bool best = true;
      for (const auto &cand : total) {
        if (is_better(cand, seq)) {
          best = false;
          continue;
        }
      }
      if (!best) {
        continue;
      }
      for (const int &x : seq) {
        cout << x << " ";
      }
      cout << endl;
      vector<vector<int>> to_remove;
      for (const auto &cand : total) {
        if (is_better(seq, cand)) {
          to_remove.push_back(cand);
        }
      }
      for (const auto &tmp : to_remove) {
        total.erase(tmp);
      }
      total.insert(seq);
    }
  }
}
