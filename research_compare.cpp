#include <algorithm>
#include <array>
#include <chrono>
#include <cstdlib>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <bits/stl_function.h>

#define sjtu sjtu_bench_baseline
#define ESET_INLINE_VALUE 0
#define ESET_MEMORY_POOL 0
#undef SJTU_EXCEPTIONS_HPP
#undef SJTU_RBTREE_HPP
#include "rb_tree.hpp"
#undef ESET_INLINE_VALUE
#undef ESET_MEMORY_POOL
#undef sjtu

#define sjtu sjtu_bench_inline
#define ESET_INLINE_VALUE 1
#define ESET_MEMORY_POOL 0
#undef SJTU_EXCEPTIONS_HPP
#undef SJTU_RBTREE_HPP
#include "rb_tree.hpp"
#undef ESET_INLINE_VALUE
#undef ESET_MEMORY_POOL
#undef sjtu

#define sjtu sjtu_bench_pool
#define ESET_INLINE_VALUE 1
#define ESET_MEMORY_POOL 1
#undef SJTU_EXCEPTIONS_HPP
#undef SJTU_RBTREE_HPP
#include "rb_tree.hpp"
#undef ESET_INLINE_VALUE
#undef ESET_MEMORY_POOL
#undef sjtu

template<class Key, class Compare = std::less<Key>>
class ESetBaseline {
public:
    using iterator = typename sjtu_bench_baseline::Rbtree<Key, Compare>::iterator;

private:
    sjtu_bench_baseline::Rbtree<Key, Compare> tree;

public:
    ESetBaseline() = default;
    ~ESetBaseline() = default;
    ESetBaseline(const ESetBaseline& other) = default;
    ESetBaseline& operator=(const ESetBaseline& other) = default;
    ESetBaseline(ESetBaseline&& other) noexcept = default;
    ESetBaseline& operator=(ESetBaseline&& other) noexcept = default;

    template<class... Args>
    std::pair<iterator, bool> emplace(Args&&... args) {
        return tree.emplace(std::forward<Args>(args)...);
    }
    size_t erase(const Key& key) { return tree.erase(key); }
    iterator find(const Key& key) const { return tree.find(key); }
    size_t range(const Key& l, const Key& r) const { return tree.range(l, r); }
    size_t size() const noexcept { return tree.size(); }
    iterator lower_bound(const Key& key) const { return tree.lower_bound(key); }
    iterator upper_bound(const Key& key) const { return tree.upper_bound(key); }
    iterator begin() const noexcept { return tree.begin(); }
    iterator end() const noexcept { return tree.end(); }
};

template<class Key, class Compare = std::less<Key>>
class ESetInline {
public:
    using iterator = typename sjtu_bench_inline::Rbtree<Key, Compare>::iterator;

private:
    sjtu_bench_inline::Rbtree<Key, Compare> tree;

public:
    ESetInline() = default;
    ~ESetInline() = default;
    ESetInline(const ESetInline& other) = default;
    ESetInline& operator=(const ESetInline& other) = default;
    ESetInline(ESetInline&& other) noexcept = default;
    ESetInline& operator=(ESetInline&& other) noexcept = default;

    template<class... Args>
    std::pair<iterator, bool> emplace(Args&&... args) {
        return tree.emplace(std::forward<Args>(args)...);
    }
    size_t erase(const Key& key) { return tree.erase(key); }
    iterator find(const Key& key) const { return tree.find(key); }
    size_t range(const Key& l, const Key& r) const { return tree.range(l, r); }
    size_t size() const noexcept { return tree.size(); }
    iterator lower_bound(const Key& key) const { return tree.lower_bound(key); }
    iterator upper_bound(const Key& key) const { return tree.upper_bound(key); }
    iterator begin() const noexcept { return tree.begin(); }
    iterator end() const noexcept { return tree.end(); }
};

template<class Key, class Compare = std::less<Key>>
class ESetInlinePool {
public:
    using iterator = typename sjtu_bench_pool::Rbtree<Key, Compare>::iterator;

private:
    sjtu_bench_pool::Rbtree<Key, Compare> tree;

public:
    ESetInlinePool() = default;
    ~ESetInlinePool() = default;
    ESetInlinePool(const ESetInlinePool& other) = default;
    ESetInlinePool& operator=(const ESetInlinePool& other) = default;
    ESetInlinePool(ESetInlinePool&& other) noexcept = default;
    ESetInlinePool& operator=(ESetInlinePool&& other) noexcept = default;

    template<class... Args>
    std::pair<iterator, bool> emplace(Args&&... args) {
        return tree.emplace(std::forward<Args>(args)...);
    }
    size_t erase(const Key& key) { return tree.erase(key); }
    iterator find(const Key& key) const { return tree.find(key); }
    size_t range(const Key& l, const Key& r) const { return tree.range(l, r); }
    size_t size() const noexcept { return tree.size(); }
    iterator lower_bound(const Key& key) const { return tree.lower_bound(key); }
    iterator upper_bound(const Key& key) const { return tree.upper_bound(key); }
    iterator begin() const noexcept { return tree.begin(); }
    iterator end() const noexcept { return tree.end(); }
};

namespace {

const int SET_CNT = 25;
const long long MISS = -(1LL << 60);
const long long KEY_RANGE = 2000000;
volatile long long sink_value = 0;

enum Kind {
    EMP = 0,
    ERASE,
    FIND,
    RANGE,
    LOWER,
    UPPER,
    NEXT,
    PREV,
    ASSIGN,
    BEGIN_OP,
    END_OP,
    KIND_CNT
};

const char* kind_name[KIND_CNT] = {
    "emplace", "erase", "find", "range", "lower_bound", "upper_bound",
    "++it", "--it", "assignment", "begin", "end"
};

const char* summary_ops[KIND_CNT + 1] = {
    "total", "emplace", "erase", "find", "range", "lower_bound", "upper_bound",
    "++it", "--it", "assignment", "begin", "end"
};

struct Op {
    Kind kind;
    int a;
    int b;
    long long x;
    long long y;
};

struct OpStat {
    uint64_t cnt = 0;
    uint64_t ns = 0;
};

struct Result {
    std::array<OpStat, KIND_CNT> op;
    uint64_t checksum = 1469598103934665603ull;
    uint64_t total_ns = 0;
    size_t final_size = 0;
};

struct RunRow {
    std::string variant;
    uint64_t seed = 0;
    int repeat = 0;
    std::string operation;
    double count = 0.0;
    double eset_ns = 0.0;
    double stl_ns = 0.0;
    bool pass = false;
};

struct SummaryRow {
    std::string operation;
    double count = 0.0;
    double stl_ns = 0.0;
    double baseline_ns = 0.0;
    double inline_ns = 0.0;
    double pool_ns = 0.0;
    bool pass = false;
};

struct RangeQuery {
    long long l = 0;
    long long r = 0;
};

struct RangeSummary {
    std::string operation;
    size_t count = 0;
    double stl_ns = 0.0;
    double baseline_ns = 0.0;
    double inline_ns = 0.0;
    double pool_ns = 0.0;
    bool pass = false;
};

uint64_t mix(uint64_t h, uint64_t x) {
    h ^= x + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
    return h;
}

void add_result(Result& r, Kind k, long long v) {
    r.checksum = mix(r.checksum, static_cast<uint64_t>(k));
    r.checksum = mix(r.checksum, static_cast<uint64_t>(v));
}

long long gen_key(std::mt19937_64& rng) {
    if (rng() % 10 < 7) {
        return static_cast<long long>(rng() % (KEY_RANGE / 10)) - KEY_RANGE / 20;
    }
    return static_cast<long long>(rng() % KEY_RANGE) - KEY_RANGE / 2;
}

long long gen_range_span(std::mt19937_64& rng) {
    uint64_t bucket = rng() % 100;
    if (bucket < 70) return static_cast<long long>(rng() % 101);
    if (bucket < 90) return static_cast<long long>(rng() % 1001);
    if (bucket < 98) return static_cast<long long>(rng() % 10001);
    return static_cast<long long>(rng() % 100001);
}

std::vector<Op> gen_ops(size_t n, uint64_t seed) {
    std::mt19937_64 rng(seed);
    size_t assign_cnt = std::min<size_t>(25, n);
    size_t emp_cnt = n / 2;
    size_t erase_cnt = n / 6;
    size_t find_cnt = n / 9;
    size_t range_cnt = n / 18;
    size_t used = emp_cnt + erase_cnt + find_cnt + range_cnt + assign_cnt;
    size_t rem = n > used ? n - used : 0;
    size_t lower_cnt = rem / 6;
    size_t upper_cnt = rem / 6;
    size_t next_cnt = rem / 6;
    size_t prev_cnt = rem / 6;
    size_t begin_cnt = rem / 6;
    size_t end_cnt = rem - lower_cnt - upper_cnt - next_cnt - prev_cnt - begin_cnt;

    std::vector<Op> ops;
    ops.reserve(n);
    auto add = [&](Kind k, size_t cnt) {
        for (size_t i = 0; i < cnt; ++i) {
            Op op;
            op.kind = k;
            op.a = static_cast<int>(rng() % SET_CNT);
            op.b = static_cast<int>(rng() % SET_CNT);
            op.x = gen_key(rng);
            long long span = k == RANGE ? gen_range_span(rng)
                                         : static_cast<long long>(rng() % 2000);
            op.y = op.x + span;
            if ((rng() & 31) == 0) std::swap(op.x, op.y);
            if (k == ASSIGN && op.a == op.b) op.b = (op.b + 1) % SET_CNT;
            ops.push_back(op);
        }
    };

    add(EMP, emp_cnt);
    add(ERASE, erase_cnt);
    add(FIND, find_cnt);
    add(RANGE, range_cnt);
    add(LOWER, lower_cnt);
    add(UPPER, upper_cnt);
    add(NEXT, next_cnt);
    add(PREV, prev_cnt);
    add(ASSIGN, assign_cnt);
    add(BEGIN_OP, begin_cnt);
    add(END_OP, end_cnt);
    std::shuffle(ops.begin(), ops.end(), rng);
    return ops;
}

double avg_ns(const OpStat& s) {
    return s.cnt == 0 ? 0.0 : static_cast<double>(s.ns) / static_cast<double>(s.cnt);
}

template<class Set>
Result run_eset(const std::vector<Op>& ops) {
    using Clock = std::chrono::steady_clock;
    std::array<Set, SET_CNT> s;
    typename Set::iterator it;
    int it_set = -1;
    bool valid = false;
    Result r;

    auto total_begin = Clock::now();
    for (const Op& op : ops) {
        OpStat& st = r.op[op.kind];
        ++st.cnt;
        long long out = 0;

        if (op.kind == ERASE && valid && it_set == op.a && *it == op.x) valid = false;
        if (op.kind == ASSIGN && valid && it_set == op.b) valid = false;

        auto t0 = Clock::now();
        switch (op.kind) {
            case EMP: {
                auto p = s[op.a].emplace(op.x);
                out = p.second ? 1 : 0;
                if (p.second) {
                    it = p.first;
                    it_set = op.a;
                    valid = true;
                }
                break;
            }
            case ERASE:
                out = static_cast<long long>(s[op.a].erase(op.x));
                break;
            case FIND: {
                auto p = s[op.a].find(op.x);
                if (p == s[op.a].end()) out = MISS;
                else {
                    out = *p;
                    it = p;
                    it_set = op.a;
                    valid = true;
                }
                break;
            }
            case RANGE:
                out = static_cast<long long>(s[op.a].range(op.x, op.y));
                break;
            case LOWER: {
                auto p = s[op.a].lower_bound(op.x);
                if (p == s[op.a].end()) out = MISS;
                else {
                    out = *p;
                    it = p;
                    it_set = op.a;
                    valid = true;
                }
                break;
            }
            case UPPER: {
                auto p = s[op.a].upper_bound(op.x);
                if (p == s[op.a].end()) out = MISS;
                else {
                    out = *p;
                    it = p;
                    it_set = op.a;
                    valid = true;
                }
                break;
            }
            case NEXT:
                if (!valid) out = MISS;
                else {
                    auto p = it;
                    ++p;
                    if (p == s[it_set].end()) {
                        valid = false;
                        out = MISS;
                    } else {
                        it = p;
                        out = *it;
                    }
                }
                break;
            case PREV:
                if (!valid) out = MISS;
                else {
                    auto p = it;
                    --p;
                    if (p == it) {
                        valid = false;
                        out = MISS;
                    } else {
                        it = p;
                        out = *it;
                    }
                }
                break;
            case ASSIGN:
                s[op.b] = s[op.a];
                out = static_cast<long long>(s[op.b].size());
                break;
            case BEGIN_OP: {
                auto p = s[op.a].begin();
                if (p == s[op.a].end()) out = MISS;
                else {
                    out = *p;
                    it = p;
                    it_set = op.a;
                    valid = true;
                }
                break;
            }
            case END_OP: {
                auto p = s[op.a].end();
                out = (p == s[op.a].end()) ? 1 : 0;
                break;
            }
            default:
                break;
        }
        auto t1 = Clock::now();
        st.ns += static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
        add_result(r, op.kind, out);
    }
    auto total_end = Clock::now();
    r.total_ns = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(total_end - total_begin).count());
    for (const auto& x : s) r.final_size += x.size();
    r.checksum = mix(r.checksum, r.final_size);
    return r;
}

Result run_stl(const std::vector<Op>& ops) {
    using Clock = std::chrono::steady_clock;
    std::array<std::set<long long>, SET_CNT> s;
    std::set<long long>::iterator it;
    int it_set = -1;
    bool valid = false;
    Result r;

    auto total_begin = Clock::now();
    for (const Op& op : ops) {
        OpStat& st = r.op[op.kind];
        ++st.cnt;
        long long out = 0;

        if (op.kind == ERASE && valid && it_set == op.a && *it == op.x) valid = false;
        if (op.kind == ASSIGN && valid && it_set == op.b) valid = false;

        auto t0 = Clock::now();
        switch (op.kind) {
            case EMP: {
                auto p = s[op.a].emplace(op.x);
                out = p.second ? 1 : 0;
                if (p.second) {
                    it = p.first;
                    it_set = op.a;
                    valid = true;
                }
                break;
            }
            case ERASE:
                out = static_cast<long long>(s[op.a].erase(op.x));
                break;
            case FIND: {
                auto p = s[op.a].find(op.x);
                if (p == s[op.a].end()) out = MISS;
                else {
                    out = *p;
                    it = p;
                    it_set = op.a;
                    valid = true;
                }
                break;
            }
            case RANGE:
                if (op.y < op.x) out = 0;
                else {
                    auto l = s[op.a].lower_bound(op.x);
                    auto rr = s[op.a].upper_bound(op.y);
                    out = static_cast<long long>(std::distance(l, rr));
                }
                break;
            case LOWER: {
                auto p = s[op.a].lower_bound(op.x);
                if (p == s[op.a].end()) out = MISS;
                else {
                    out = *p;
                    it = p;
                    it_set = op.a;
                    valid = true;
                }
                break;
            }
            case UPPER: {
                auto p = s[op.a].upper_bound(op.x);
                if (p == s[op.a].end()) out = MISS;
                else {
                    out = *p;
                    it = p;
                    it_set = op.a;
                    valid = true;
                }
                break;
            }
            case NEXT:
                if (!valid) out = MISS;
                else {
                    auto p = it;
                    ++p;
                    if (p == s[it_set].end()) {
                        valid = false;
                        out = MISS;
                    } else {
                        it = p;
                        out = *it;
                    }
                }
                break;
            case PREV:
                if (!valid || it == s[it_set].begin()) {
                    valid = false;
                    out = MISS;
                } else {
                    --it;
                    out = *it;
                }
                break;
            case ASSIGN:
                s[op.b] = s[op.a];
                out = static_cast<long long>(s[op.b].size());
                break;
            case BEGIN_OP: {
                auto p = s[op.a].begin();
                if (p == s[op.a].end()) out = MISS;
                else {
                    out = *p;
                    it = p;
                    it_set = op.a;
                    valid = true;
                }
                break;
            }
            case END_OP: {
                auto p = s[op.a].end();
                out = (p == s[op.a].end()) ? 1 : 0;
                break;
            }
            default:
                break;
        }
        auto t1 = Clock::now();
        st.ns += static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
        add_result(r, op.kind, out);
    }
    auto total_end = Clock::now();
    r.total_ns = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(total_end - total_begin).count());
    for (const auto& x : s) r.final_size += x.size();
    r.checksum = mix(r.checksum, r.final_size);
    return r;
}

RunRow make_row(const std::string& variant, uint64_t seed, int repeat,
                const std::string& operation, double count,
                double eset_ns, double stl_ns, bool pass) {
    RunRow row;
    row.variant = variant;
    row.seed = seed;
    row.repeat = repeat;
    row.operation = operation;
    row.count = count;
    row.eset_ns = eset_ns;
    row.stl_ns = stl_ns;
    row.pass = pass;
    return row;
}

void append_rows(std::vector<RunRow>& rows, const std::string& variant,
                 uint64_t seed, int repeat, const Result& eset,
                 const Result& stl, bool pass, size_t ops) {
    rows.push_back(make_row(variant, seed, repeat, "total",
                            static_cast<double>(ops),
                            static_cast<double>(eset.total_ns) / ops,
                            static_cast<double>(stl.total_ns) / ops, pass));
    for (int i = 0; i < KIND_CNT; ++i) {
        rows.push_back(make_row(variant, seed, repeat, kind_name[i],
                                static_cast<double>(eset.op[i].cnt),
                                avg_ns(eset.op[i]), avg_ns(stl.op[i]), pass));
    }
}

double ratio(double a, double b) {
    return b == 0.0 ? 0.0 : a / b;
}

double mean(const std::vector<double>& v) {
    if (v.empty()) return 0.0;
    double s = 0.0;
    for (double x : v) s += x;
    return s / static_cast<double>(v.size());
}

SummaryRow summarize_op(const std::string& operation,
                        const std::vector<RunRow>& raw) {
    std::vector<double> counts;
    std::vector<double> stl;
    std::vector<double> base;
    std::vector<double> inl;
    std::vector<double> pool;
    bool pass = true;

    for (const RunRow& r : raw) {
        if (r.operation != operation) continue;
        if (r.variant == "baseline") {
            counts.push_back(r.count);
            stl.push_back(r.stl_ns);
            base.push_back(r.eset_ns);
        } else if (r.variant == "inline_value") {
            inl.push_back(r.eset_ns);
        } else if (r.variant == "inline_value+memory_pool") {
            pool.push_back(r.eset_ns);
        }
        pass = pass && r.pass;
    }

    SummaryRow row;
    row.operation = operation;
    row.count = mean(counts);
    row.stl_ns = mean(stl);
    row.baseline_ns = mean(base);
    row.inline_ns = mean(inl);
    row.pool_ns = mean(pool);
    row.pass = pass && !base.empty() && !inl.empty() && !pool.empty();
    return row;
}

std::vector<SummaryRow> summarize(const std::vector<RunRow>& raw) {
    std::vector<SummaryRow> rows;
    for (const char* op : summary_ops) rows.push_back(summarize_op(op, raw));
    return rows;
}

const SummaryRow* find_summary(const std::vector<SummaryRow>& rows,
                               const std::string& operation) {
    for (const SummaryRow& r : rows) {
        if (r.operation == operation) return &r;
    }
    return nullptr;
}

std::string fmt(double x) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(3) << x;
    return ss.str();
}

void write_raw_csv(const std::string& path, const std::vector<RunRow>& rows) {
    std::ofstream out(path.c_str());
    out << "variant,seed,repeat,sets,operation,count,eset_ns,stl_ns,ratio,correctness_pass\n";
    out << std::fixed << std::setprecision(4);
    for (const RunRow& r : rows) {
        out << r.variant << "," << r.seed << "," << r.repeat << ","
            << SET_CNT << "," << r.operation << "," << r.count << ","
            << r.eset_ns << "," << r.stl_ns << "," << ratio(r.eset_ns, r.stl_ns)
            << "," << (r.pass ? "true" : "false") << "\n";
    }
}

void write_summary_csv(const std::string& path,
                       const std::vector<SummaryRow>& rows) {
    std::ofstream out(path.c_str());
    out << "operation,count,stl_ns,baseline_ns,baseline_over_stl,"
           "inline_ns,inline_over_stl,inline_speedup_vs_baseline,"
           "inline_pool_ns,inline_pool_over_stl,pool_speedup_vs_inline,"
           "total_speedup_vs_baseline,correctness_pass\n";
    out << std::fixed << std::setprecision(4);
    for (const SummaryRow& r : rows) {
        out << r.operation << "," << r.count << "," << r.stl_ns << ","
            << r.baseline_ns << "," << ratio(r.baseline_ns, r.stl_ns) << ","
            << r.inline_ns << "," << ratio(r.inline_ns, r.stl_ns) << ","
            << ratio(r.baseline_ns, r.inline_ns) << ","
            << r.pool_ns << "," << ratio(r.pool_ns, r.stl_ns) << ","
            << ratio(r.inline_ns, r.pool_ns) << ","
            << ratio(r.baseline_ns, r.pool_ns) << ","
            << (r.pass ? "true" : "false") << "\n";
    }
}

std::vector<long long> gen_range_data(size_t n, uint64_t seed) {
    std::mt19937_64 rng(seed);
    std::set<long long> seen;
    while (seen.size() < n) {
        long long x = static_cast<long long>(rng() % KEY_RANGE) - KEY_RANGE / 2;
        seen.emplace(x);
    }
    return std::vector<long long>(seen.begin(), seen.end());
}

std::vector<RangeQuery> gen_range_queries(const std::string& operation,
                                          size_t count, uint64_t seed) {
    std::mt19937_64 rng(seed);
    std::vector<RangeQuery> q;
    q.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        RangeQuery item;
        item.l = static_cast<long long>(rng() % KEY_RANGE) - KEY_RANGE / 2;
        long long span = 0;
        if (operation == "range_small") {
            span = static_cast<long long>(rng() % 101);
        } else if (operation == "range_medium") {
            span = static_cast<long long>(rng() % 10001);
        } else {
            span = static_cast<long long>(rng() % (KEY_RANGE + 1));
        }
        item.r = item.l + span;
        q.push_back(item);
    }
    return q;
}

template<class Set>
double run_range_eset(const std::vector<long long>& data,
                      const std::vector<RangeQuery>& q,
                      uint64_t& checksum, size_t& final_size) {
    using Clock = std::chrono::steady_clock;
    Set s;
    for (long long x : data) s.emplace(x);

    checksum = 1469598103934665603ull;
    auto t0 = Clock::now();
    for (const RangeQuery& x : q) {
        size_t ans = s.range(x.l, x.r);
        checksum = mix(checksum, static_cast<uint64_t>(ans));
        sink_value += static_cast<long long>(ans & 1);
    }
    auto t1 = Clock::now();
    final_size = s.size();
    return static_cast<double>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count()) / q.size();
}

double run_range_stl(const std::vector<long long>& data,
                     const std::vector<RangeQuery>& q,
                     uint64_t& checksum, size_t& final_size) {
    using Clock = std::chrono::steady_clock;
    std::set<long long> s;
    for (long long x : data) s.emplace(x);

    checksum = 1469598103934665603ull;
    auto t0 = Clock::now();
    for (const RangeQuery& x : q) {
        size_t ans = 0;
        if (x.r >= x.l) {
            auto l = s.lower_bound(x.l);
            auto r = s.upper_bound(x.r);
            ans = static_cast<size_t>(std::distance(l, r));
        }
        checksum = mix(checksum, static_cast<uint64_t>(ans));
        sink_value += static_cast<long long>(ans & 1);
    }
    auto t1 = Clock::now();
    final_size = s.size();
    return static_cast<double>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count()) / q.size();
}

RangeSummary run_range_case(const std::string& operation, size_t count,
                            uint64_t query_seed,
                            const std::vector<long long>& data) {
    std::vector<RangeQuery> q = gen_range_queries(operation, count, query_seed);

    uint64_t stl_checksum = 0;
    uint64_t base_checksum = 0;
    uint64_t inline_checksum = 0;
    uint64_t pool_checksum = 0;
    size_t stl_size = 0;
    size_t base_size = 0;
    size_t inline_size = 0;
    size_t pool_size = 0;

    RangeSummary row;
    row.operation = operation;
    row.count = q.size();
    row.stl_ns = run_range_stl(data, q, stl_checksum, stl_size);
    row.baseline_ns = run_range_eset<ESetBaseline<long long>>(
        data, q, base_checksum, base_size);
    row.inline_ns = run_range_eset<ESetInline<long long>>(
        data, q, inline_checksum, inline_size);
    row.pool_ns = run_range_eset<ESetInlinePool<long long>>(
        data, q, pool_checksum, pool_size);
    row.pass = base_checksum == stl_checksum
            && inline_checksum == stl_checksum
            && pool_checksum == stl_checksum
            && base_size == stl_size
            && inline_size == stl_size
            && pool_size == stl_size;
    return row;
}

std::vector<RangeSummary> run_range_sweep() {
    std::vector<long long> data = gen_range_data(120000, 2026051699ull);
    std::vector<RangeSummary> rows;
    rows.push_back(run_range_case("range_small", 50000, 2026051601ull, data));
    rows.push_back(run_range_case("range_medium", 50000, 2026051602ull, data));
    rows.push_back(run_range_case("range_large", 3000, 2026051603ull, data));
    return rows;
}

void write_range_csv(const std::string& path,
                     const std::vector<RangeSummary>& rows) {
    std::ofstream out(path.c_str());
    out << "operation,count,stl_ns,baseline_ns,baseline_over_stl,"
           "inline_ns,inline_over_stl,inline_speedup_vs_baseline,"
           "inline_pool_ns,inline_pool_over_stl,pool_speedup_vs_inline,"
           "total_speedup_vs_baseline,correctness_pass\n";
    out << std::fixed << std::setprecision(4);
    for (const RangeSummary& r : rows) {
        out << r.operation << "," << r.count << "," << r.stl_ns << ","
            << r.baseline_ns << "," << ratio(r.baseline_ns, r.stl_ns) << ","
            << r.inline_ns << "," << ratio(r.inline_ns, r.stl_ns) << ","
            << ratio(r.baseline_ns, r.inline_ns) << ","
            << r.pool_ns << "," << ratio(r.pool_ns, r.stl_ns) << ","
            << ratio(r.inline_ns, r.pool_ns) << ","
            << ratio(r.baseline_ns, r.pool_ns) << ","
            << (r.pass ? "true" : "false") << "\n";
    }
}

void write_report(const std::string& path, const std::string& raw_path,
                  const std::string& summary_path,
                  const std::string& range_path,
                  const std::vector<SummaryRow>& rows,
                  const std::vector<RangeSummary>& range_rows,
                  size_t ops, int repeats,
                  const std::vector<uint64_t>& seeds, bool all_pass) {
    std::ofstream out(path.c_str());
    out << "# ESet Three-Variant Ablation Benchmark\n\n";
    out << "## Correctness\n\n";
    out << "- ops per trace: " << ops << "\n";
    out << "- sets: " << SET_CNT << "\n";
    out << "- repeats per seed: " << repeats << "\n";
    out << "- seeds:";
    for (uint64_t seed : seeds) out << " " << seed;
    out << "\n";
    out << "- trace policy: 每个 seed 只生成一次 trace，STL 与三个 ESet variant 共用同一份 trace。\n";
    out << "- correctness: " << (all_pass ? "PASS" : "FAIL") << "\n";
    out << "- CSV: `" << raw_path << "`, `" << summary_path << "`, `"
        << range_path << "`\n\n";

    out << "## Summary\n\n";
    out << "| operation | STL ns | baseline ns | baseline/STL | inline ns | inline/STL | inline speedup vs baseline | inline+pool ns | inline+pool/STL | pool speedup vs inline | total speedup vs baseline |\n";
    out << "|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|\n";
    out << std::fixed << std::setprecision(2);
    for (const SummaryRow& r : rows) {
        out << "| " << r.operation << " | " << r.stl_ns
            << " | " << r.baseline_ns
            << " | " << ratio(r.baseline_ns, r.stl_ns)
            << " | " << r.inline_ns
            << " | " << ratio(r.inline_ns, r.stl_ns)
            << " | " << ratio(r.baseline_ns, r.inline_ns)
            << " | " << r.pool_ns
            << " | " << ratio(r.pool_ns, r.stl_ns)
            << " | " << ratio(r.inline_ns, r.pool_ns)
            << " | " << ratio(r.baseline_ns, r.pool_ns)
            << " |\n";
    }

    out << "\n## Range Sweep\n\n";
    out << "| operation | count | STL ns | baseline ns | baseline/STL | inline ns | inline/STL | inline speedup vs baseline | inline+pool ns | inline+pool/STL | pool speedup vs inline | inline+pool speedup vs baseline | pass |\n";
    out << "|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|\n";
    for (const RangeSummary& r : range_rows) {
        out << "| " << r.operation << " | " << r.count
            << " | " << r.stl_ns
            << " | " << r.baseline_ns
            << " | " << ratio(r.baseline_ns, r.stl_ns)
            << " | " << r.inline_ns
            << " | " << ratio(r.inline_ns, r.stl_ns)
            << " | " << ratio(r.baseline_ns, r.inline_ns)
            << " | " << r.pool_ns
            << " | " << ratio(r.pool_ns, r.stl_ns)
            << " | " << ratio(r.inline_ns, r.pool_ns)
            << " | " << ratio(r.baseline_ns, r.pool_ns)
            << " | " << (r.pass ? "true" : "false") << " |\n";
    }

    const SummaryRow* total = find_summary(rows, "total");
    const SummaryRow* emp = find_summary(rows, "emplace");
    const SummaryRow* assign = find_summary(rows, "assignment");

    out << "\n## 中文分析\n\n";
    out << "- mixed benchmark 评估日常综合性能：同一批 trace 同时覆盖插入、删除、查找、边界查询、迭代器移动、赋值和保守分布的 range。\n";
    out << "- range sweep 单独评估区间查询优势：`range_small`、`range_medium`、`range_large` 使用固定数据集和完全相同的查询 trace，观察区间跨度变大时 ESet 的 rank 方案相对 STL 遍历方案的收益。\n";
    out << "- OJ 结果补充：baseline 约 8000ms，inline value 7286ms，inline+memory_pool 6488ms；这组结果更贴近提交环境下的整体运行时间。\n";
    if (total != nullptr) {
        out << "- 总体看，inline value 相对 baseline 的 ESet 自身 speedup 为 "
            << fmt(ratio(total->baseline_ns, total->inline_ns))
            << "，inline+pool 相对 inline value 的 speedup 为 "
            << fmt(ratio(total->inline_ns, total->pool_ns))
            << "，inline+pool 相对 baseline 的总 speedup 为 "
            << fmt(ratio(total->baseline_ns, total->pool_ns)) << "。\n";
    }
    out << "- inline value 的主要作用是减少 `Key*` 间接访问，并去掉每个 Key 的单独分配。这个收益会体现在插入、查找、区间 rank 以及拷贝时的缓存局部性上。\n";
    out << "- memory pool 的主要作用是减少 Node 小对象频繁 `new/delete` 的成本；pool 只管理 raw memory，普通节点仍然通过 placement new 正常构造，析构时也会调用 `Node` 析构函数。\n";
    if (assign != nullptr) {
        out << "- assignment 的 pool speedup 为 "
            << fmt(ratio(assign->inline_ns, assign->pool_ns))
            << "。如果它是所有操作里改善最大的项目，这是预期结果，因为深拷贝会批量创建大量节点，最容易受节点分配成本影响。\n";
    }
    if (emp != nullptr) {
        double sp = ratio(emp->inline_ns, emp->pool_ns);
        if (sp < 1.03) {
            out << "- 本次 pool 对 emplace 的改善不明显，可能原因包括 free-list/chunk 管理本身也有成本、测试规模还不够放大分配差距、系统 allocator 已经较快，以及运行噪声影响。\n";
        } else {
            out << "- 本次 pool 对 emplace 有可见改善，说明节点分配成本在插入路径中已经占到一定比例。\n";
        }
    }
    out << "- `range` 的判断仍应和 STL ratio 分开看：ESet 使用 subtree size 做两次 rank 搜索，STL baseline 用 `distance(lower_bound, upper_bound)` 逐步遍历区间，因此区间分布会显著影响 STL ratio。\n";
    out << "- 最终结论应以同 trace 下的 ESet 自身 ns/speedup 为准，而不是用不同进程、不同 trace 或单独三次运行的结果手工拼接。\n";
}

Result run_variant(const std::string& variant, const std::vector<Op>& ops) {
    if (variant == "baseline") return run_eset<ESetBaseline<long long>>(ops);
    if (variant == "inline_value") return run_eset<ESetInline<long long>>(ops);
    return run_eset<ESetInlinePool<long long>>(ops);
}

} // namespace

int main(int argc, char** argv) {
    size_t ops = argc > 1 ? static_cast<size_t>(std::stoull(argv[1])) : 200000;
    int repeats = argc > 2 ? std::max(1, std::atoi(argv[2])) : 3;
    int seed_cnt = argc > 3 ? std::max(1, std::atoi(argv[3])) : 4;
    std::string prefix = argc > 4 ? argv[4] : "eset_ablation";

    if (ops < 100000) ops = 100000;
    if (ops > 300000) ops = 300000;

    std::vector<uint64_t> seeds;
    for (int i = 0; i < seed_cnt; ++i) {
        seeds.push_back(2026051601ull + static_cast<uint64_t>(i) * 1000003ull);
    }

    const std::array<std::string, 3> variants = {
        "baseline", "inline_value", "inline_value+memory_pool"
    };

    std::vector<RunRow> raw;
    bool all_pass = true;
    for (size_t si = 0; si < seeds.size(); ++si) {
        uint64_t seed = seeds[si];
        std::vector<Op> trace = gen_ops(ops, seed);
        for (int r = 0; r < repeats; ++r) {
            Result stl = run_stl(trace);
            size_t offset = (si + static_cast<size_t>(r)) % variants.size();
            std::array<Result, 3> result;
            std::array<bool, 3> seen = {{false, false, false}};

            for (size_t i = 0; i < variants.size(); ++i) {
                size_t id = (offset + i) % variants.size();
                result[id] = run_variant(variants[id], trace);
                seen[id] = true;
            }

            for (size_t i = 0; i < variants.size(); ++i) {
                bool pass = seen[i] && result[i].checksum == stl.checksum
                         && result[i].final_size == stl.final_size;
                all_pass = all_pass && pass;
                append_rows(raw, variants[i], seed, r, result[i], stl, pass, ops);
                std::cout << "seed=" << seed << " repeat=" << r
                          << " variant=" << variants[i]
                          << " correctness=" << (pass ? "PASS" : "FAIL") << "\n";
            }
        }
    }

    std::vector<SummaryRow> summary = summarize(raw);
    for (const SummaryRow& r : summary) all_pass = all_pass && r.pass;
    std::vector<RangeSummary> range_rows = run_range_sweep();
    for (const RangeSummary& r : range_rows) all_pass = all_pass && r.pass;

    std::string raw_path = prefix + "_raw.csv";
    std::string summary_path = prefix + "_summary.csv";
    std::string range_path = prefix + "_range.csv";
    std::string report_path = prefix + "_report.md";
    write_raw_csv(raw_path, raw);
    write_summary_csv(summary_path, summary);
    write_range_csv(range_path, range_rows);
    write_report(report_path, raw_path, summary_path, range_path, summary, range_rows,
                 ops, repeats, seeds, all_pass);

    std::cout << "wrote " << raw_path << "\n";
    std::cout << "wrote " << summary_path << "\n";
    std::cout << "wrote " << range_path << "\n";
    std::cout << "wrote " << report_path << "\n";
    std::cout << "overall " << (all_pass ? "PASS" : "FAIL") << "\n";
    return all_pass ? 0 : 1;
}
