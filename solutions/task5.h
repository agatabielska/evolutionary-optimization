#pragma once

#include "task3.h"
#include "task4.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

struct Task5Stats {
    long long evaluated_moves = 0;   // computed deltas, summed over all runs
    long long generated_moves = 0;   // improving moves added to LM, summed over all runs
    long long applied_moves = 0;     // applied moves, summed over all runs
    long long full_evaluations = 0;  // evaluations of the whole neighborhood, summed over all runs
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

// Heap order: the move with the smallest (best) delta is at the front.
struct BestFirst {
    bool operator()(const Move& l, const Move& r) const { return l.delta > r.delta; }
};

enum class Status { remove, deferred, ready };

// Candidate lists: for every node its k best partners (identical to Task 4).
struct Candidates {
    std::array<std::vector<int>, total_nodes> list;
    std::array<std::array<bool, total_nodes>, total_nodes> is;  // is[a][b]: b in list[a]
};

inline std::unique_ptr<Candidates> build_candidates(int dataset, int k) {
    auto c = std::make_unique<Candidates>();
    for (auto& row : c->is) row.fill(false);
    const std::vector<std::vector<int>> lists =
        task4_detail::build_candidate_lists(dataset, k);
    for (int a = 0; a < total_nodes; ++a) {
        for (int b : lists[a]) {
            c->list[a].push_back(b);
            c->is[a][b] = true;
        }
    }
    return c;
}

// Steepest local search with the list of improving moves LM, in the "alternative"
// version of the lecture:
//   - LM is filled by evaluating the whole neighborhood (moves applicable to the current
//     solution and moves with relatively inverted edges),
//   - moves are taken from LM from the best one; moves that are no longer valid are removed,
//     moves with relatively inverted edges are kept but not applied,
//   - new moves are NOT evaluated after every applied move; the whole neighborhood is evaluated
//     again only when LM contains no applicable move.
class Search {
public:
    // cand == nullptr -> full neighborhood; otherwise candidate moves only
    Search(const std::vector<int>& start, int dataset, Task5Stats& stats,
           const Candidates* cand = nullptr)
        : dataset_(dataset), stats_(stats), sol_(start), cand_(cand) {
        pos_.fill(-1);
        for (int i = 0; i < solution_size; ++i) pos_[sol_[i]] = i;
        lm_.reserve(20000);
    }

    const std::vector<int>& solution() const { return sol_; }

    void run() {
        evaluate_all();
        bool fresh = true;  // LM was just built from the whole neighborhood of the solution
        while (true) {
            if (apply_best_applicable()) {
                ++stats_.applied_moves;
                fresh = false;
                continue;
            }
            // No applicable move in LM.
            if (fresh) return;  // LM is up to date and still has no applicable move: local optimum
            evaluate_all();
            fresh = true;
        }
    }

private:
    int dataset_;
    Task5Stats& stats_;
    std::vector<int> sol_;
    const Candidates* cand_;
    std::array<int, total_nodes> pos_;
    std::vector<Move> lm_;        // LM stored as a binary heap, best move at the front
    std::vector<Move> deferred_;  // moves with relatively inverted edges met while browsing

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
    // two forward edges (u,u2) and (v,v2) are distinct and share no node
    static bool nonadj(int u, int u2, int v, int v2) { return u != v && u2 != v && v2 != u; }

    // ---------- evaluation ----------
    void offer_edge(int a, int a2, int b, int b2) {
        Move m{Kind::edge_exchange, a, a2, b, b2, 0};
        m.delta = D(a, b) + D(a2, b2) - D(a, a2) - D(b, b2);
        ++stats_.evaluated_moves;
        if (m.delta < 0) {
            lm_.push_back(m);
            ++stats_.generated_moves;
        }
    }

    void offer_exchange(int prev, int node, int next, int outside) {
        Move m{Kind::inter_route_exchange, prev, node, next, outside, 0};
        m.delta = NODE_COST[outside][dataset_] - NODE_COST[node][dataset_] +
                  D(prev, outside) + D(outside, next) - D(prev, node) - D(node, next);
        ++stats_.evaluated_moves;
        if (m.delta < 0) {
            lm_.push_back(m);
            ++stats_.generated_moves;
        }
    }

    // Evaluates the whole neighborhood of the current solution and puts the improving moves into
    // LM. When it is called LM holds only moves with inverted edges, which are generated again
    // here, so LM is emptied first (the result is LM extended without duplicates).
    void evaluate_all() {
        ++stats_.full_evaluations;
        lm_.clear();
        if (cand_) generate_candidate_moves();
        else generate_all_moves();
        std::make_heap(lm_.begin(), lm_.end(), BestFirst());
    }

    // ================= full neighborhood =================
    void generate_all_moves() {
        for (int i = 0; i < solution_size; ++i) {
            const int u = sol_[i];
            const int u2 = sol_[(i + 1) % solution_size];
            for (int j = i + 2; j < solution_size; ++j) {
                if (i == 0 && j == solution_size - 1) continue;
                const int v = sol_[j];
                const int v2 = sol_[(j + 1) % solution_size];
                offer_edge(u, u2, v, v2);  // applicable now
                offer_edge(u, u2, v2, v);  // relatively inverted edges
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

    // ================= candidate neighborhood =================
    // All moves that add the candidate edge (a,b), b in cand[a], a in the solution.
    void gen_pair(int a, int b) {
        if (a == b || !selected(a)) return;
        // a mutual candidate pair of two selected nodes is generated once, from the smaller node
        if (selected(b) && cand_->is[b][a] && b < a) return;

        const int sa = succ(a), pa = pred(a);
        if (selected(b)) {
            const int sb = succ(b), pb = pred(b);
            // two moves applicable now ...
            if (nonadj(a, sa, b, sb)) offer_edge(a, sa, b, sb);  // adds (a,b),(sa,sb)
            if (nonadj(pa, a, pb, b)) offer_edge(pa, a, pb, b);  // adds (pa,pb),(a,b)
            // ... and two with relatively inverted edges
            if (nonadj(a, sa, pb, b)) offer_edge(a, sa, b, pb);  // adds (a,b),(sa,pb)
            if (nonadj(pa, a, b, sb)) offer_edge(a, pa, b, sb);  // adds (a,b),(pa,sb)
        } else {
            offer_exchange(a, sa, succ(sa), b);                  // b replaces succ(a)
            offer_exchange(pred(pa), pa, a, b);                  // b replaces pred(a)
        }
    }

    void generate_candidate_moves() {
        for (int i = 0; i < solution_size; ++i) {
            const int a = sol_[i];
            for (int b : cand_->list[a]) gen_pair(a, b);
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
        if (!(f1 || r1) || !(f2 || r2)) return Status::remove;  // a removed edge is gone
        return (f1 == f2) ? Status::ready : Status::deferred;   // same / different direction
    }

    // Browses LM from the best move: removes moves that are no longer valid, keeps moves with
    // relatively inverted edges, applies the first applicable move (and removes it from LM).
    bool apply_best_applicable() {
        deferred_.clear();
        bool applied = false;
        while (!lm_.empty()) {
            std::pop_heap(lm_.begin(), lm_.end(), BestFirst());
            const Move m = lm_.back();
            lm_.pop_back();
            const Status status = check(m);
            if (status == Status::remove) continue;
            if (status == Status::deferred) {
                deferred_.push_back(m);
                continue;
            }
#ifdef TASK5_DEBUG_CHECK
            const int before = task4_detail::objective(sol_, dataset_);
#endif
            apply(m);
#ifdef TASK5_DEBUG_CHECK
            if (task4_detail::objective(sol_, dataset_) != before + m.delta) {
                std::cerr << "TASK5 delta mismatch\n";
                std::abort();
            }
#endif
            applied = true;
            break;
        }
        // deferred moves stay in LM
        for (const Move& m : deferred_) {
            lm_.push_back(m);
            std::push_heap(lm_.begin(), lm_.end(), BestFirst());
        }
        return applied;
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

inline Task5Stats run_all(ResultManager& result_manager,
                          const std::vector<std::vector<int>>& starts,
                          const Candidates* cand) {
    Task5Stats stats;
    for (const auto& start : starts) {
        const auto begin = Clock::now();
        Search search(start, result_manager.dataset, stats, cand);
        search.run();
        const auto end = Clock::now();
        std::vector<int> solution = search.solution();
        result_manager.check_solution(solution.data(), solution_size);
        result_manager.record_run_time(elapsed_ms(begin, end));
        ++stats.runs;
    }
    return stats;
}

}  // namespace task5_detail

// Experiment A: steepest local search + list of improving moves, full neighborhood.
inline Task5Stats task5_improving_list(
    ResultManager& result_manager,
    const std::vector<std::vector<int>>& starts) {
    return task5_detail::run_all(result_manager, starts, nullptr);
}

// Experiment B (optional): list of improving moves combined with candidate moves.
inline Task5Stats task5_improving_list_candidates(
    ResultManager& result_manager,
    const std::vector<std::vector<int>>& starts, int k) {
    const auto candidates = task5_detail::build_candidates(result_manager.dataset, k);
    return task5_detail::run_all(result_manager, starts, candidates.get());
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
         << std::setw(16) << "Applied" << std::setw(18) << "Full evaluations" << '\n';
    for (const auto& [name, stats] : methods) {
        file << std::left << std::setw(62) << name << std::right << std::fixed
             << std::setprecision(1)
             << std::setw(16) << static_cast<double>(stats.evaluated_moves) / stats.runs
             << std::setw(16) << static_cast<double>(stats.generated_moves) / stats.runs
             << std::setw(16) << static_cast<double>(stats.applied_moves) / stats.runs;
        if (stats.full_evaluations > 0) {  // the baseline has no list and no separate full evaluations
            file << std::setw(18) << static_cast<double>(stats.full_evaluations) / stats.runs;
        } else {
            file << std::setw(18) << "-";
        }
        file << '\n';
    }
}