#include <cassert>
#include <format>
#include <generator>
#include <iostream>
#include <iterator>
#include <numeric>
#include <thread>
#include <utility>
#include <vector>

using namespace std;

struct ShiftedGraph {
  int s;
  vector<pair<int, int>> antipath;

  generator<const pair<int, int> &> edges(vector<char> *used = nullptr) {
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
    const vector<reference_wrapper<ShiftedGraph>> &graph_sequence) {
  // cerr << "GRAPH SEQUENCE\n";
  // cerr << "ANTIPATHS:\n";
  for (const auto &graph : graph_sequence) {
    for (const auto &[x, y] : graph.get().antipath) {
      cerr << format("({},{})", x, y) << " ";
    }
    cerr << endl;
  }
  int ss = graph_sequence[0].get().s;
  for (int y = 2 * ss + 1; y >= 0; --y) {
    cerr << y << '|';
    for (const auto &graph : graph_sequence) {
      int s = graph.get().s;
      int last_x0 = 1;
      for (auto [x0, y0] : graph.get().edges()) {
        if (y0 == y) {
          for (; last_x0 < x0; ++last_x0) {
            cerr << ' ';
          }
          cerr << '#';
          ++last_x0;
        }
      }
      for (; last_x0 <= 2 * s; ++last_x0) {
        if (1 <= y && y <= 2 * ss) {
          cerr << " ";
        } else if (y > 2 * ss) {
          cerr << "-";
        } else {
          cerr << last_x0;
        }
      }
    }
    cerr << '|';
    cerr << "\n";
  }
}

void do_stuff(int s, int start, int end,
              std::vector<std::vector<int>> *results) {
  for (auto &&size_sequence : gen_size_sequence(s, s, start, end)) {
    optional<vector<pair<int, int>>> matching;
    // cerr << "Checking sequence ";
    // for (const int &x : size_sequence) {
    // cerr << x << " ";
    // }
    // cerr << "\n";
    vector<char> used(2 * s + 1);
    for (const auto &graph_sequence :
         gen_graph_sequences(s, size_sequence.begin(), size_sequence.end())) {
      // print_graph_sequence(graph_sequence);
      ranges::fill(used, 0);
      if (matching = gen_matching(graph_sequence.begin(), graph_sequence.end(),
                                  used)) {
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
        results->push_back(move(size_sequence));
        break;
      }
    }
  }
}

int main() {
  int s;
  cin >> s;
  std::vector<std::thread> threads;
  int max_size = s * (2 * s - 1);
  std::vector<std::vector<std::vector<int>>> results(max_size + 1);
  for (int i = 1; i <= max_size; ++i) {
    threads.emplace_back(do_stuff, s, i, i, &results[i]);
  }
  for (int i = 0; i < threads.size(); ++i) {
    threads[i].join();
  }
  for (const auto &thrd : results) {
    for (const auto &seq : thrd) {
      for (const int &x : seq) {
        cout << x << " ";
      }
      cout << "\n";
    }
  }
}
