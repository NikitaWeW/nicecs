#include "catch2/catch_test_macros.hpp"
#include "catch2/benchmark/catch_benchmark.hpp"
#include "common.hpp"
#include "nicecs/ecs.hpp"

#include <random>
#include <vector>
#include <string>
#include <cstring>


TEST_CASE("ecs::sparse_set benchmarks", "[benchmark][ecs][ecs::sparse_set]") {
    ecs::sparse_set<Position> s;
    
    uint64_t i = 0;
    BENCHMARK("emplace") {
        ++i;
        return s.emplace(i, 3.6f*i, -2.4f*i);
    };

    i = 0;
    s.clear();

    BENCHMARK("emplace and erase") {
        ++i;
        s.emplace(i, 3.6f*i, -2.4f*i);
        s.erase(i);
        return i;
    };

    i = 0;
    s.clear();

    BENCHMARK("emplace and get") {
        ++i;
        s.emplace(i, 3.6f*i, -2.4f*i);
        return s.get(i);
    };
}
TEST_CASE("ecs::registry benchmarks", "[benchmark][ecs][ecs::registry]") {
    {
        ecs::registry registry;
        addEntities(registry, 100'000);
        BENCHMARK("view (100'000)") {
            auto v = registry.view<Position, Velocity>(ecs::exclude<Tag, Health>{});
            return v.size();
        };
    }
    {
        ecs::registry registry;
        BENCHMARK("create and emplace") {
            auto e = registry.create();
            registry.emplace<Position>(e, 1.0f, 2.0f);
            return e;
        };
    }
    {
        ecs::registry registry;
        BENCHMARK("create and destroy") {
            auto e = registry.create<Position>();
            registry.destroy(e);
            return e;
        };
    }
    {
        BENCHMARK("create registry") { return ecs::registry{}; };
    }
}
