#include "catch2/catch_test_macros.hpp"
#define ECS_SPARSE_SET_THROW ECS_THROW
#include "ecs.hpp"
#include "ecs.hpp" // check if there are no odr issues
#include <vector>
#include "types.hpp"

/*! \cond Doxygen_Suppress */

TEST_CASE("Sparse set tests", "[ecs][ecs::sparse_set]")
{
    ecs::sparse_set<std::string> s;
    s.emplace(1, "Position");
    s.emplace(2, "Velocity");
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
    std::vector<std::string> expected1 = { "Position", "F", "C", "D", "E" };
    REQUIRE(s.data() == expected1);

    s.erase(5);
    REQUIRE_FALSE(s.contains(5));
    REQUIRE(s.data().size() == 4);
    REQUIRE(std::find(s.data().begin(), s.data().end(), "Position") != s.data().end());
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
            auto sig0 = ecs::make_signature<>();
            REQUIRE(sig0.none());
            auto sig1 = ecs::make_signature<Position>();
            REQUIRE(sig1.count() == 1);
            auto sig2 = ecs::make_signature<Position, Velocity>();
            REQUIRE(sig2.count() == 2);
        }
        {
            ecs::entity e = reg.create<Position, Velocity>();
            ecs::signature exected = ecs::make_signature<Position, Velocity>();
            REQUIRE(reg.getSignature(e) == exected);
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

        auto posOnly = reg.view(ecs::make_signature<Position>(), ecs::make_signature<Velocity>());
        REQUIRE(posOnly.size() == 1);
        REQUIRE(std::find(posOnly.begin(), posOnly.end(), e0) == posOnly.end());
        REQUIRE(std::find(posOnly.begin(), posOnly.end(), e1) != posOnly.end());
        REQUIRE(std::find(posOnly.begin(), posOnly.end(), e2) == posOnly.end());

        auto velView = reg.view<Velocity>();
        REQUIRE(posOnly.size() == 1);
        REQUIRE(std::find(velView.begin(), velView.end(), e0) == velView.end());
        REQUIRE(std::find(velView.begin(), velView.end(), e1) == velView.end());
        REQUIRE(std::find(velView.begin(), velView.end(), e2) != velView.end());

        REQUIRE(reg.view<>().size() == reg.size());
    }

    SECTION("merge")
    {
        ecs::registry reg2;

        auto e0 = reg.create(Position{1, 0});
        auto e1 = reg.create(Position{0, 1}, Velocity{1, 1});

        auto e2 = reg2.create(Tag{"Hello, World!"});
        auto e3 = reg2.create(Position{1, 1}, Velocity{0, 0});

        reg.merge(reg2);

        REQUIRE(reg.size() == 4);
        REQUIRE(reg.view<Position>().size() == 3);

        unsigned e0_found = 0, e1_found = 0, e2_found = 0, e3_found = 0;
        auto entities = reg.view<>();
        for(auto e : entities)
        {
            auto sig = reg.getSignature(e);
            e0_found += sig == reg.getSignature(e0) && reg.get<Position>(e) == Position{1, 0};
            e1_found += sig == reg.getSignature(e1) && reg.get<Position>(e) == Position{0, 1} && reg.get<Velocity>(e) == Velocity{1, 1};
            e2_found += sig == reg2.getSignature(e2) && reg.get<Tag>(e) == Tag{"Hello, World!"};
            e3_found += sig == reg2.getSignature(e3) && reg.get<Position>(e) == Position{1, 1} && reg.get<Velocity>(e) == Velocity{0, 0};
        }

        REQUIRE((e0_found == 1 && e1_found == 1 && e2_found == 1 && e3_found == 1));
    }
}
TEST_CASE("Registry example", "[ecs][ecs::registry]")
{
    ecs::registry registry;

    for(auto i = 0u; i < 10u; ++i) 
    {
        const auto entity = registry.create();
        registry.emplace<Position>(entity, i * 1.f, i * 1.f);
        if(i % 2 == 0) 
        { 
            registry.emplace<Velocity>(entity, i * .1f, i * .1f); 
        }
        if(i == 8)
        {
            registry.emplace<Tag>(entity);
        }
    }

    auto view = registry.view<Position, Velocity>(ecs::exclude_t<Tag>{});

    REQUIRE(view.size() == 4);

    for(auto const &e : view) 
    {
        registry.get<Position>(e).x += registry.get<Velocity>(e).dx;
        registry.get<Position>(e).y += registry.get<Velocity>(e).dy;
    }
}

/** \endcond */