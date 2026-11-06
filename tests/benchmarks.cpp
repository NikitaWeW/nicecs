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
        return s.emplace(i, 3.6f * i, -2.4f * i);
    };

    i = 0;
    s.clear();

    BENCHMARK("emplace and erase") {
        ++i;
        s.emplace(i, 3.6f * i, -2.4f * i);
        s.erase(i);
        return i;
    };

    i = 0;
    s.clear();

    BENCHMARK("emplace and get") {
        ++i;
        s.emplace(i, 3.6f * i, -2.4f * i);
        return s.get(i);
    };
}
TEST_CASE("ecs::registry benchmarks", "[benchmark][ecs][ecs::registry]") {
    {
        ecs::registry reg;
        addEntities(reg, 100'000);
        BENCHMARK("view (100'000)") {
            // clang-format off
            auto v = ecs::make_view(
                std::tuple(&reg.storage<Position>(), &reg.storage<Velocity>(), &reg.storage<Position>("position"_hs), &reg.storage<Velocity>("velocity"_hs)),
                std::tuple(&reg.storage<Health>(), &reg.storage<Tag>(), &reg.storage<Health>("health"_hs), &reg.storage<Tag>("tag"_hs))
            );
            // clang-format on

            ecs::size_t size = 0;
            for(auto it = v.begin(); it != v.end(); ++it) {
                ++size;
            }

            return size;
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
        BENCHMARK("create registry") {
            return ecs::registry{ };
        };
    }
}
