// Build in release mode

#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <algorithm>
#include <random>

#include <entt/entt.hpp>
#include "ecs.hpp"

/*! \cond Doxygen_Suppress */

struct Position { float x, y; };
struct Velocity { float x, y; };
struct Health   { int   hp; };

TEST_CASE("Create entities", "[benchmark]") {
    {
        BENCHMARK("entt: create() ") {
            entt::registry reg;
            auto dummy = reg.create();
            (void)dummy;
            return 0;
        };
    }

    {
        ecs::registry reg;
        BENCHMARK("custom ecs: create() ") {
            reg.create();
            return 0;
        };
    }
}

TEST_CASE("emplace three components", "[benchmark]") {
    {
        BENCHMARK("entt: emplace<Position, Velocity, Health> to entity") {
            entt::registry reg;
            auto e = reg.create();
            reg.emplace<Position>(e, 0.0f, 0.0f);
            reg.emplace<Velocity>(e, 1.0f, 1.0f);
            reg.emplace<Health>(e, 100);
            return 0;
        };
    }

    {
        ecs::registry reg;
        BENCHMARK("custom ecs: emplace<Position, Velocity, Health> to entity") {
            auto e = reg.create();
            reg.emplace<Position>(e, 0.0f, 0.0f);
            reg.emplace<Velocity>(e, 1.0f, 1.0f);
            reg.emplace<Health>(e, 100);
            return 0;
        };
    }
}

TEST_CASE("Iterate view of entities with Position & Velocity", "[benchmark]") {
    entt::registry entt_reg;
    ecs::registry  custom_reg;

    for (std::size_t i = 0; i < 10000; ++i) {
        auto e1 = entt_reg.create();
        entt_reg.emplace<Position>(e1, float(i), float(i)+1.0f);
        entt_reg.emplace<Velocity>(e1, float(i)*0.5f, float(i)*0.5f);

        auto e2 = custom_reg.create();
        custom_reg.emplace<Position>(e2, float(i), float(i)+1.0f);
        custom_reg.emplace<Velocity>(e2, float(i)*0.5f, float(i)*0.5f);
    }

    BENCHMARK("entt: view(Position, Velocity) iteration") {
        auto view = entt_reg.view<Position, Velocity>();
        for (auto [entity, pos, vel] : view.each()) {
            pos.x += vel.x;
            pos.y += vel.y;
        }
        return 0;
    };

    BENCHMARK("custom ecs: view<Position, Velocity> iteration") {
        auto entities = custom_reg.view<Position, Velocity>();
        for (auto &entity: entities) {
            auto &pos = custom_reg.get<Position>(entity);
            auto &vel = custom_reg.get<Velocity>(entity);
            pos.x += vel.x;
            pos.y += vel.y;
        }
        return 0;
    };
}

static std::vector<std::size_t> make_indices(std::size_t n) {
    std::vector<std::size_t> idx(n);
    std::iota(idx.begin(), idx.end(), 0);
    std::shuffle(idx.begin(), idx.end(), std::mt19937{std::random_device{}()});
    return idx;
}
TEST_CASE("Benchmark: ecs::sparse_set insert", "[benchmark][ecs::sparse_set]") {
    constexpr std::size_t N = 100000;
    auto indices = make_indices(N);

    ecs::sparse_set<int> set;
    set.reserve(N);

    BENCHMARK("insert N integers") {
        // clear and re-reserve for consistent baseline
        set = ecs::sparse_set<int>{};
        set.reserve(N);

        for (std::size_t i = 0; i < N; ++i) {
            set.insert(indices[i], static_cast<int>(i));
        }
    };
}
TEST_CASE("Benchmark: ecs::sparse_set lookup", "[benchmark][ecs::sparse_set]") {
    constexpr std::size_t N    = 100000;
    constexpr std::size_t Q    = 50000;   // number of lookups
    auto indices = make_indices(N);
    auto queries = make_indices(Q);

    ecs::sparse_set<int> set;
    set.reserve(N);
    // Pre-populate
    for (std::size_t i = 0; i < N; ++i) {
        set.insert(indices[i], static_cast<int>(i));
    }

    BENCHMARK("contains Q times") {
        std::size_t hits = 0;
        for (auto s : queries) {
            if (set.contains(s)) ++hits;
        }
        return hits;
    };

    BENCHMARK("get via operator[] Q times") {
        long sum = 0;
        for (auto s : queries) {
            sum += set[s];
        }
        return sum;
    };
}
TEST_CASE("Benchmark: ecs::sparse_set remove", "[benchmark][ecs::sparse_set]") {
    constexpr std::size_t N = 100000;
    auto indices = make_indices(N);

    ecs::sparse_set<int> set;
    set.reserve(N);
    for (std::size_t i = 0; i < N; ++i) {
        set.insert(indices[i], static_cast<int>(i));
    }

    BENCHMARK("remove N elements") {
        // Refill each iteration
        ecs::sparse_set<int> local = set;
        for (std::size_t i = 0; i < N; ++i) {
            local.remove(indices[i]);
        }
    };
}
TEST_CASE("Benchmark: dense iteration", "[benchmark][ecs::sparse_set]") {
    constexpr std::size_t N = 200000;
    ecs::sparse_set<int> set;
    set.reserve(N);
    for (std::size_t i = 0; i < N; ++i) {
        set.insert(i, static_cast<int>(i));
    }

    BENCHMARK("iterate over dense data") {
        long total = 0;
        for (auto& v : set) {
            total += v;
        }
        return total;
    };
}



/*! \endcond */