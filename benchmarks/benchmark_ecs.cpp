// Build in release mode

#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include <entt/entt.hpp>
#include "ecs.hpp"

/*! \cond Doxygen_Suppress */

struct Position { float x, y; };
struct Velocity { float x, y; };
struct Health   { int   hp; };

TEST_CASE("Create entities", "[benchmark]") {
    {
        entt::registry reg;
        BENCHMARK("entt: create() ") {
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
        entt::registry reg;
        BENCHMARK("entt: emplace<Position, Velocity, Health> to entity") {
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

/*! \endcond */