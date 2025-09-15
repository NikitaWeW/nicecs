#include "catch2/catch_test_macros.hpp"
#include "catch2/benchmark/catch_benchmark.hpp"
#include "ecs.hpp"
#include "ecs.hpp" // check if there are no odr issues
#include <vector>

/*! \cond Doxygen_Suppress */

struct Position {
    float x = 0, y = 0;
    bool operator==(Position const& o) const { return x == o.x && y == o.y; }
};

struct Velocity {
    float dx = 0, dy = 0;
    bool operator==(Velocity const& o) const { return dx == o.dx && dy == o.dy; }
};


TEST_CASE("Sparse set tests", "[ecs][ecs::sparse_set]")
{
    ecs::sparse_set<std::string> s;
    s.emplace(1, "A");
    s.emplace(2, "B");
    s.emplace(3, "C");
    s.emplace(4, "D");
    s.emplace(5, "E");
    s.emplace(6, "F");

    REQUIRE_THROWS_AS(s.emplace(1), std::invalid_argument);

    s.erase(2);
    REQUIRE(s.contains(1));
    REQUIRE_FALSE(s.contains(2));
    REQUIRE(s.contains(3));
    REQUIRE(s.contains(4));
    REQUIRE(s.contains(5));
    REQUIRE(s.contains(6));
    std::vector<std::string> expected1 = { "A", "F", "C", "D", "E" };
    REQUIRE(s.data() == expected1);

    s.erase(5);
    REQUIRE_FALSE(s.contains(5));
    REQUIRE(s.data().size() == 4);
    REQUIRE(std::find(s.data().begin(), s.data().end(), "A") != s.data().end());
    REQUIRE(std::find(s.data().begin(), s.data().end(), "D") != s.data().end());

    for(size_t i = 0; i < s.sparseData().size(); ++i)
    {
        if(s.sparseData()[i] != ecs::sparse_set<int>::null)
            s.erase(i);
    }

    REQUIRE(s.data().empty());
    REQUIRE(s.getDenseToSparse().empty());

    s.shrink_to_fit();

    REQUIRE(s.data().capacity() == 0);
    REQUIRE(s.getDenseToSparse().capacity() == 0);
    REQUIRE(s.sparseData().capacity() == 0);
    REQUIRE(s.sparseData().empty());

    s.reserve(100);

    REQUIRE(s.data().capacity() == 100);
    REQUIRE(s.getDenseToSparse().capacity() == 100);
    REQUIRE(s.sparseData().capacity() == 100);

    REQUIRE_THROWS_AS(s.get(0), std::out_of_range);
    REQUIRE_THROWS_AS(s[0], std::out_of_range);
    REQUIRE_THROWS_AS(s.erase(0), std::out_of_range);
}
TEST_CASE("emplacement of a aggregate type in the sparse set", "[ecs][ecs::sparse_set]")
{
    ecs::sparse_set<Position> s;
    s.emplace(0, 0.0f, 0.0f);
    s.emplace(1, 0.1f, 0.1f);
    s.emplace(2, 0.2f, 0.2f);
    
    REQUIRE(s.contains(0));
    REQUIRE(s.contains(1));
    REQUIRE(s.contains(2));
    REQUIRE(s.get(0) == Position{0.0f, 0.0f});
    REQUIRE(s.get(1) == Position{0.1f, 0.1f});
    REQUIRE(s.get(2) == Position{0.2f, 0.2f});
}
TEST_CASE("Registry tests", "[ecs][ecs::registry]")
{
    ecs::registry reg;

    SECTION("entity creation and destruction")
    {
        ecs::entity e0 = reg.create();
        ecs::entity e1 = reg.create<Position>();
        ecs::entity e2 = reg.create<Position, Velocity>({0.1f, 10}, {1, 0});
        ecs::entity e3 = 0;

        REQUIRE(reg.valid(e0));
        REQUIRE(reg.empty(e0));
        REQUIRE(reg.size(e0) == 0);
        REQUIRE_FALSE(reg.has<Position>(e0));
        REQUIRE_FALSE(reg.has<Velocity>(e0));

        REQUIRE(reg.valid(e1));
        REQUIRE_FALSE(reg.empty(e1));
        REQUIRE(reg.size(e1) == 1);
        REQUIRE(reg.has<Position>(e1));
        REQUIRE_FALSE(reg.has<Velocity>(e1));
        REQUIRE(reg.get<Position>(e1) == Position{});

        REQUIRE(reg.valid(e2));
        REQUIRE_FALSE(reg.empty(e2));
        REQUIRE(reg.size(e2) == 2);
        REQUIRE(reg.has<Position>(e2));
        REQUIRE(reg.has<Velocity>(e2));
        REQUIRE(reg.get<Position>(e2) == Position{0.1f, 10});
        REQUIRE(reg.get<Velocity>(e2) == Velocity{1, 0});

        REQUIRE_FALSE(reg.valid(e3));
        REQUIRE_THROWS_AS(reg.has<Position>(e3), std::invalid_argument);
        REQUIRE_THROWS_AS(reg.get<Position>(e3), std::invalid_argument);

        reg.destroy(e0);
    }

    SECTION("component manipulation")
    {
        ecs::entity e = reg.create();
        REQUIRE(reg.valid(e));
        REQUIRE_FALSE(reg.has<Position>(e));
        REQUIRE_FALSE(reg.has<Velocity>(e));

        reg.emplace<Position>(e, 0.0f, 0.0f);
        REQUIRE(reg.valid(e));
        REQUIRE(reg.has<Position>(e));
        REQUIRE(reg.get<Position>(e) == Position{0.0f, 0.0f});

        reg.remove<Position>(e);
        REQUIRE(reg.valid(e));
        REQUIRE_FALSE(reg.has<Position>(e));
        REQUIRE(reg.empty(e));
        REQUIRE_THROWS_AS(reg.get<Position>(e), std::out_of_range);
    }

    SECTION("signature manipulation")
    {
        {
            ecs::signature sig;
            ecs::entity e = reg.create(sig);

            REQUIRE(reg.valid(e));
            REQUIRE(reg.empty(e));
            REQUIRE(reg.size(e) == 0);
            REQUIRE_FALSE(reg.has<Position>(e));
            REQUIRE_FALSE(reg.has<Velocity>(e));
            REQUIRE_THROWS_AS(reg.remove<Position>(e), std::out_of_range);
        }
        {
            auto sig0 = reg.makeSignature<>();
            REQUIRE(sig0.none());
            auto sig1 = reg.makeSignature<Position>();
            REQUIRE(sig1.count() == 1);
            auto sig2 = reg.makeSignature<Position, Velocity>();
            REQUIRE(sig2.count() == 2);
        }
        {
            ecs::entity e = reg.create<Position, Velocity>();
            ecs::signature exected = reg.makeSignature<Position, Velocity>();
            REQUIRE(reg.getSignature(e) == exected);
        }
        {
            ecs::entity e = reg.create(reg.makeSignature<Position, Velocity>());
            REQUIRE(reg.has<Position>(e));
            REQUIRE(reg.has<Velocity>(e));
            REQUIRE(reg.getSignature(e) == reg.makeSignature<Position, Velocity>());
            REQUIRE(reg.size(e) == 2);
        }
    }

    SECTION("views")
    {
        auto e0 = reg.create();
        auto e1 = reg.create<Position>();
        auto e2 = reg.create<Position, Velocity>();

        auto posView = reg.view<Position>();
        REQUIRE(posView.size() == 2);
        REQUIRE(std::find(posView.begin(), posView.end(), e0) == posView.end());
        REQUIRE(std::find(posView.begin(), posView.end(), e1) != posView.end());
        REQUIRE(std::find(posView.begin(), posView.end(), e2) != posView.end());

        auto posOnly = reg.view(reg.makeSignature<Position>(), reg.makeSignature<Velocity>());
        REQUIRE(posOnly.size() == 1);
        REQUIRE(std::find(posOnly.begin(), posOnly.end(), e0) == posOnly.end());
        REQUIRE(std::find(posOnly.begin(), posOnly.end(), e1) != posOnly.end());
        REQUIRE(std::find(posOnly.begin(), posOnly.end(), e2) == posOnly.end());

        auto velView = reg.view<Velocity>();
        REQUIRE(posOnly.size() == 1);
        REQUIRE(std::find(velView.begin(), velView.end(), e0) == velView.end());
        REQUIRE(std::find(velView.begin(), velView.end(), e1) == velView.end());
        REQUIRE(std::find(velView.begin(), velView.end(), e2) != velView.end());
    }
}
TEST_CASE("Registry example", "[ecs][ecs::registry]")
{
    struct position {
        float x;
        float y;
    };
    struct velocity {
        float dx;
        float dy;
    };
    struct tag {};

    ecs::registry registry;

    for(auto i = 0u; i < 10u; ++i) 
    {
        const auto entity = registry.create();
        registry.emplace<position>(entity, i * 1.f, i * 1.f);
        if(i % 2 == 0) 
        { 
            registry.emplace<velocity>(entity, i * .1f, i * .1f); 
        }
        if(i == 8)
        {
            registry.emplace<tag>(entity);
        }
    }

    auto view = registry.view<position, velocity>(ecs::exclude_t<tag>{});

    REQUIRE(view.size() == 4);

    for(auto const &e : view) 
    {
        registry.get<position>(e).x += registry.get<velocity>(e).dx;
        registry.get<position>(e).y += registry.get<velocity>(e).dy;
    }
}

/** \endcond */