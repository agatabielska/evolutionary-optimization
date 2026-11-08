#pragma once

#include "task3.h"
#include "task4.h"
#include <array>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>

struct Task5Stats {
    long long evaluated_moves = 0;
    long long generated_moves = 0;
    long long applied_moves = 0;
    int runs = 0;
};

namespace task5_detail {

using local_search_detail::solution_size;
using local_search_detail::total_nodes;
using Clock = std::chrono::high_resolution_clock;

enum class Kind { inter_route_exchange, edge_exchange };

// edge_exchange:         removes (n1,n2),(n3,n4); adds (n1,n3),(n2,n4)
// inter_route_exchange:  node n2 with neighbours n1,n3 is replaced by outside node n4
struct Move {
    Kind kind;
    int n1, n2, n3, n4;
    int delta;
};

struct ByDelta {
    bool operator()(const Move& l, const Move& r) const { return l.delta < r.delta; }
};

enum class Status { remove, deferred, ready };

class Search {
public:
    Search(const std::vector<int>& start, int dataset, Task5Stats& stats)
        : dataset_(dataset), stats_(stats), sol_(start) {
        pos_.fill(-1);
        for (int i = 0; i < solution_size; ++i) pos_[sol_[i]] = i;
    }

    const std::vector<int>& solution() const { return sol_; }

    void run() {
        generate_initial();
        while (true) {
            bool applied = false;
            for (auto it = lm_.begin(); it != lm_.end();) {
                const Status status = check(*it);
                if (status == Status::remove) {
                    it = lm_.erase(it);
                } else if (status == Status::deferred) {
                    ++it;  // not applicable now, keep it and browse further
                } else {
                    const Move move = *it;
                    lm_.erase(it);
                    apply(move);
                    ++stats_.applied_moves;
                    generate_after(move);
                    applied = true;
                    break;
                }
            }
            // LM holds every improving applicable move, so if none could be
            // applied the solution is a local optimum.
            if (!applied) return;
        }
    }

private:
    int dataset_;
    Task5Stats& stats_;
    std::vector<int> sol_;
    std::array<int, total_nodes> pos_;
    std::multiset<Move, ByDelta> lm_;

    int D(int a, int b) const { return DISTANCE_MATRIX[a][b][dataset_]; }
    bool selected(int node) const { return pos_[node] >= 0; }
    int succ(int node) const {
        const int p = pos_[node] + 1;
        return sol_[p == solution_size ? 0 : p];
    }
    int pred(int node) const {
        const int p = pos_[node];
        return sol_[p == 0 ? solution_size - 1 : p - 1];
    }
    bool has_edge(int from, int to) const {
        return selected(from) && selected(to) && succ(from) == to;
    }

    // ---------- evaluation ----------
    void offer_edge(int a, int a2, int b, int b2) {
        Move m{Kind::edge_exchange, a, a2, b, b2, 0};
        m.delta = D(a, b) + D(a2, b2) - D(a, a2) - D(b, b2);
        ++stats_.evaluated_moves;
        if (m.delta < 0) {
            lm_.insert(m);
            ++stats_.generated_moves;
        }
    }

    void offer_exchange(int prev, int node, int next, int outside) {
        Move m{Kind::inter_route_exchange, prev, node, next, outside, 0};
        m.delta = NODE_COST[outside][dataset_] - NODE_COST[node][dataset_] +
                  D(prev, outside) + D(outside, next) - D(prev, node) - D(node, next);
        ++stats_.evaluated_moves;
        if (m.delta < 0) {
            lm_.insert(m);
            ++stats_.generated_moves;
        }
    }

    // edge (u,u2) in current direction against edge (v,v2) in current direction:
    // normal move + move with relatively inverted edges
    void offer_edge_pair(int u, int u2, int v, int v2) {
        offer_edge(u, u2, v, v2);
        offer_edge(u, u2, v2, v);
    }

    void generate_initial() {
        for (int i = 0; i < solution_size; ++i) {
            const int u = sol_[i];
            const int u2 = sol_[(i + 1) % solution_size];
            for (int j = i + 2; j < solution_size; ++j) {
                if (i == 0 && j == solution_size - 1) continue;
                offer_edge_pair(u, u2, sol_[j], sol_[(j + 1) % solution_size]);
            }
        }
        for (int i = 0; i < solution_size; ++i) {
            const int prev = sol_[(i + solution_size - 1) % solution_size];
            const int node = sol_[i];
            const int next = sol_[(i + 1) % solution_size];
            for (int o = 0; o < total_nodes; ++o)
                if (!selected(o)) offer_exchange(prev, node, next, o);
        }
    }

    // Only moves involving new edges / nodes with new neighbours / the removed node.
    void generate_after(const Move& m) {
        std::vector<std::pair<int, int>> new_edges;  // directed as in current solution
        std::array<bool, total_nodes> affected{};
        std::vector<int> affected_list;
        auto mark = [&](int x) {
            if (!affected[x]) {
                affected[x] = true;
                affected_list.push_back(x);
            }
        };
        auto add_edge = [&](int x, int y) {
            new_edges.push_back(succ(x) == y ? std::make_pair(x, y) : std::make_pair(y, x));
            mark(x);
            mark(y);
        };

        int removed_node = -1;
        if (m.kind == Kind::edge_exchange) {
            add_edge(m.n1, m.n3);
            add_edge(m.n2, m.n4);
        } else {
            const int o = m.n4;
            removed_node = m.n2;
            add_edge(pred(o), o);
            add_edge(o, succ(o));
        }

        // new-edge x other-edge pairs (both relative orientations)
        for (size_t j = 0; j < new_edges.size(); ++j) {
            const auto [u, u2] = new_edges[j];
            for (int i = 0; i < solution_size; ++i) {
                const int v = sol_[i];
                const int v2 = sol_[i + 1 == solution_size ? 0 : i + 1];
                if (v == u || v == u2 || v2 == u) continue;  // same or adjacent edge
                bool earlier_new = false;
                for (size_t t = 0; t < j; ++t)
                    if (new_edges[t].first == v && new_edges[t].second == v2) earlier_new = true;
                if (earlier_new) continue;
                offer_edge_pair(u, u2, v, v2);
            }
        }

        // nodes with changed neighbours: try every outside node
        for (int x : affected_list) {
            const int p = pred(x), n = succ(x);
            for (int o = 0; o < total_nodes; ++o)
                if (!selected(o)) offer_exchange(p, x, n, o);
        }
        // a node left the solution: try inserting it in place of every other node
        if (removed_node >= 0) {
            for (int i = 0; i < solution_size; ++i) {
                const int w = sol_[i];
                if (affected[w]) continue;  // already covered above
                offer_exchange(sol_[(i + solution_size - 1) % solution_size], w,
                               sol_[(i + 1) % solution_size], removed_node);
            }
        }
    }

    // ---------- applicability ----------
    Status check(const Move& m) const {
        if (m.kind == Kind::inter_route_exchange) {
            if (!selected(m.n2) || selected(m.n4)) return Status::remove;
            const int p = pred(m.n2), n = succ(m.n2);
            if ((p == m.n1 && n == m.n3) || (p == m.n3 && n == m.n1)) return Status::ready;
            return Status::remove;
        }
        const bool f1 = has_edge(m.n1, m.n2), r1 = has_edge(m.n2, m.n1);
        const bool f2 = has_edge(m.n3, m.n4), r2 = has_edge(m.n4, m.n3);
        if (!(f1 || r1) || !(f2 || r2)) return Status::remove;  // an edge is gone
        return (f1 == f2) ? Status::ready : Status::deferred;   // same / different direction
    }

    // ---------- applying ----------
    void apply(const Move& m) {
        if (m.kind == Kind::inter_route_exchange) {
            const int p = pos_[m.n2];
            sol_[p] = m.n4;
            pos_[m.n4] = p;
            pos_[m.n2] = -1;
            return;
        }
        if (has_edge(m.n1, m.n2)) reverse_path(m.n2, m.n3);  // both in saved direction
        else reverse_path(m.n1, m.n4);                       // both reversed
    }

    // reverse the forward path from node s to node t (or equivalently its complement)
    void reverse_path(int s, int t) {
        int i = pos_[s], j = pos_[t];
        int len = (j - i + solution_size) % solution_size + 1;
        if (2 * len > solution_size) {
            const int ni = (j + 1) % solution_size;
            const int nj = (i + solution_size - 1) % solution_size;
            i = ni;
            j = nj;
            len = solution_size - len;
        }
        for (int k = 0; k < len / 2; ++k) {
            const int a = sol_[i], b = sol_[j];
            sol_[i] = b;
            pos_[b] = i;
            sol_[j] = a;
            pos_[a] = j;
            if (++i == solution_size) i = 0;
            if (--j < 0) j = solution_size - 1;
        }
    }
};

inline double elapsed_ms(Clock::time_point begin, Clock::time_point end) {
    return std::chrono::duration<double, std::milli>(end - begin).count();
}

}  // namespace task5_detail

inline Task5Stats task5_improving_list(
    ResultManager& result_manager,
    const std::vector<std::vector<int>>& starts) {
    Task5Stats stats;
    for (const auto& start : starts) {
        const auto begin = task5_detail::Clock::now();
        task5_detail::Search search(start, result_manager.dataset, stats);
        search.run();
        const auto end = task5_detail::Clock::now();
        std::vector<int> solution = search.solution();
        result_manager.check_solution(solution.data(), task5_detail::solution_size);
        result_manager.record_run_time(task5_detail::elapsed_ms(begin, end));
        ++stats.runs;
    }
    return stats;
}

inline void write_task5_stats(
    const std::string& filename,
    const std::vector<std::pair<std::string, Task5Stats>>& methods) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening task 5 stats file: " << filename << '\n';
        return;
    }
    file << "Average move-list statistics per run\n";
    file << std::left << std::setw(62) << "Method" << std::right
         << std::setw(16) << "Evaluated" << std::setw(16) << "Added to LM"
         << std::setw(16) << "Applied" << '\n';
    for (const auto& [name, stats] : methods) {
        file << std::left << std::setw(62) << name << std::right << std::fixed
             << std::setprecision(1)
             << std::setw(16) << static_cast<double>(stats.evaluated_moves) / stats.runs
             << std::setw(16) << static_cast<double>(stats.generated_moves) / stats.runs
             << std::setw(16) << static_cast<double>(stats.applied_moves) / stats.runs
             << '\n';
    }
}