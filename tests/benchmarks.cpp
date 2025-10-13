#include "catch2/catch_test_macros.hpp"
#include "catch2/benchmark/catch_benchmark.hpp"
#include "ecs.hpp"
#include "types.hpp"

#include <random>
#include <vector>
#include <string>
#include <cstring>

static constexpr std::size_t SIZE = 10'000;
static ecs::registry make_registry()
{
    ecs::registry reg;

    std::mt19937_64 rng(12345);
    std::uniform_int_distribution<int> dist(0, 1);

    for(std::size_t i = 0; i < SIZE; ++i)
    {
        bool hasPos = dist(rng);
        bool hasVel = dist(rng);
        bool hasHealth = dist(rng);
        bool hasData = dist(rng);

        if(hasData)
        {
            if(hasPos && hasVel && hasHealth)
                reg.create(Position{float(i), float(i+1)}, Velocity{float(i) * 123, float(i) * 312}, Health{unsigned(i)%100});
            else if(hasPos && hasVel)
                reg.create(Position{float(i), float(i+1)}, Velocity{float(i) * 123, float(i) * 312});
            else if(hasPos && hasHealth)
                reg.create(Position{float(i), float(i+1)}, Health{unsigned(i)%100});
            else if(hasVel && hasHealth)
                reg.create(Velocity{float(i) * 123, float(i) * 312}, Health{unsigned(i)%100});
            else if(hasPos)
                reg.create(Position{float(i), float(i+1)});
            else if(hasVel)
                reg.create(Velocity{float(i) * 123, float(i) * 312});
            else if(hasHealth)
                reg.create(Health{unsigned(i)%100});
            else
                reg.create<>();
        }
        else
        {
            if(hasPos && hasVel && hasHealth)
                reg.create<Position, Velocity, Health>();
            else if(hasPos && hasVel)
                reg.create<Position, Velocity>();
            else if(hasPos && hasHealth)
                reg.create<Position, Health>();
            else if(hasVel && hasHealth)
                reg.create<Velocity, Health>();
            else if(hasPos)
                reg.create<Position>();
            else if(hasVel)
                reg.create<Velocity>();
            else if(hasHealth)
                reg.create<Health>();
            else
                reg.create<>();
        }
    }

    return reg;
}

TEST_CASE("ecs::registry benchmarks", "[benchmark][ecs::registry]")
{
    {
        auto const registry = make_registry();
        BENCHMARK("view")
        {
            auto v = registry.view<Position, Velocity>(ecs::exclude_t<Tag, Health>{});
            return v.size();
        };
    }
    {
        auto const a = make_registry();
        auto const b = make_registry();
        BENCHMARK("merged")
        {
            return a.merged(b);
        };
    }
    {
        ecs::registry registry;
        BENCHMARK("create and emplace")
        {
            auto e = registry.create<>();
            registry.emplace<Position>(e, 1.0f, 2.0f);
            return e;
        };
    }
}
