#include "catch2/catch_test_macros.hpp"
struct AssertException {};
#define ECS_ASSERT(x, msg) if(!(x)) { throw AssertException{}; }
#include "nicecs/ecs.hpp"
#include "nicecs/ecs.hpp" // check if there are no odr issues
#include "types.hpp"

#include <vector>
#include <set>

/*! \cond Doxygen_Suppress */

TEST_CASE("Sparse set tests", "[ecs][ecs::sparse_set]")
{
    ecs::sparse_set<std::string> s;
    REQUIRE(s.empty());
    REQUIRE(s.size() == 0);
    s.emplace(1, "Position");
    s.emplace(2, "Velocity");
    s.emplace(3, "C");
    s.emplace(4, "D");
    s.emplace(5, "E");
    s.emplace(6, "F");
    REQUIRE(s.size() == 6);
    REQUIRE_FALSE(s.empty());

    REQUIRE_THROWS_AS(s.emplace(1), AssertException);

    s.erase(2);
    REQUIRE(s.size() == 5);
    REQUIRE(s.contains(1));
    REQUIRE_FALSE(s.contains(2));
    REQUIRE(s.contains(3));
    REQUIRE(s.contains(4));
    REQUIRE(s.contains(5));
    REQUIRE(s.contains(6));
    std::vector<std::string> expected1 = { "Position", "F", "C", "D", "E" };
    REQUIRE(s.data() == expected1);

    s.erase(5);
    REQUIRE(s.size() == 4);
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
    REQUIRE(s.empty());
    REQUIRE(s.size() == 0);
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

    REQUIRE_THROWS_AS(s.get(0), AssertException);
    REQUIRE_THROWS_AS(s.erase(0), AssertException);
    REQUIRE_NOTHROW(s[0]);
    s.erase(0);

    s.emplace(1, "Position");
    s.emplace(2, "Velocity");
    s.emplace(3, "C");
    s.emplace(4, "D");
    s.emplace(5, "E");
    s.emplace(6, "F");

    REQUIRE(s.data().size() == 6);
    REQUIRE(s.getDenseToSparse().size() == 6);

    s.clear();

    REQUIRE(s.data().size() == 0);
    REQUIRE(s.sparseData().size() == 0);
    REQUIRE(s.getDenseToSparse().size() == 0);

    s.emplace(2, "Velocity");
    s.emplace(4, "D");
    s.emplace(6, "F");
    s.emplace(1, "Position");
    s.emplace(5, "E");
    s.emplace(3, "C");

    std::vector<std::pair<std::size_t, std::string>> seen;
    for (auto it = s.begin(); it != s.end(); ++it) {
        auto [s, d] = *it;
        seen.emplace_back(s, d);
    }
    REQUIRE(seen.size() == 6);

    REQUIRE(seen[0].first == 2); REQUIRE(seen[0].second == "Velocity");
    REQUIRE(seen[1].first == 4); REQUIRE(seen[1].second == "D");
    REQUIRE(seen[2].first == 6); REQUIRE(seen[2].second == "F");
    REQUIRE(seen[3].first == 1); REQUIRE(seen[3].second == "Position");
    REQUIRE(seen[4].first == 5); REQUIRE(seen[4].second == "E");
    REQUIRE(seen[5].first == 3); REQUIRE(seen[5].second == "C");

    for (auto it = s.begin(); it != s.end(); ++it) {
        auto [s, d] = *it;
        if (s == 3) d = "Cucumber";
    }
    REQUIRE(s.get(3) == "Cucumber");

    auto it = s.begin();
    REQUIRE((it + 1) != it);
    auto pair1 = *(it + 1);
    REQUIRE(pair1.first == s.getDenseToSparse()[1]);
    auto pr = it[2];
    REQUIRE(pr.second == "F");

    auto it2 = s.begin() + 3;
    REQUIRE((it2 - s.begin()) == 3);
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
        Position copythis{4, 2};
        ecs::entity e4 = reg.create(copythis);
        REQUIRE(reg.size() == 4);

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

        REQUIRE(reg.valid(e4));
        REQUIRE_FALSE(reg.empty(e4));
        REQUIRE(reg.size(e4) == 1);
        REQUIRE(reg.has<Position>(e4));
        REQUIRE_FALSE(reg.has<Velocity>(e4));
        REQUIRE(reg.get<Position>(e4) == Position{4, 2});
        REQUIRE(copythis == Position{4, 2});

        reg.destroy(e0);
        REQUIRE_FALSE(reg.valid(e0));
        REQUIRE(reg.size() == 3);
        
        reg.clear();
        REQUIRE(reg.size() == 0);
        REQUIRE_FALSE(reg.valid(e0));
        REQUIRE_FALSE(reg.valid(e1));
        REQUIRE_FALSE(reg.valid(e2));
        REQUIRE_FALSE(reg.valid(e4));
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

    SECTION("views")
    {
        auto e0 = reg.create();
        auto e1 = reg.create<Position>();
        auto e2 = reg.create<Position, Velocity>();
        auto e3 = reg.create<Position>();
        auto e4 = reg.create<Velocity>();

        {
            auto posView = reg.view<Position>();
            REQUIRE(posView.size() == 3);
            REQUIRE(std::find(posView.begin(), posView.end(), e0) == posView.end());
            REQUIRE(std::find(posView.begin(), posView.end(), e1) != posView.end());
            REQUIRE(std::find(posView.begin(), posView.end(), e2) != posView.end());
            REQUIRE(std::find(posView.begin(), posView.end(), e3) != posView.end());
            REQUIRE(std::find(posView.begin(), posView.end(), e4) == posView.end());
    
            auto posOnly = reg.view<Position>(ecs::exclude_t<Velocity>{});
            REQUIRE(posOnly.size() == 2);
            REQUIRE(std::find(posOnly.begin(), posOnly.end(), e0) == posOnly.end());
            REQUIRE(std::find(posOnly.begin(), posOnly.end(), e1) != posOnly.end());
            REQUIRE(std::find(posOnly.begin(), posOnly.end(), e2) == posOnly.end());
            REQUIRE(std::find(posOnly.begin(), posOnly.end(), e3) != posOnly.end());
            REQUIRE(std::find(posOnly.begin(), posOnly.end(), e4) == posOnly.end());
    
            auto velView = reg.view<Velocity>();
            REQUIRE(posOnly.size() == 2);
            REQUIRE(std::find(velView.begin(), velView.end(), e0) == velView.end());
            REQUIRE(std::find(velView.begin(), velView.end(), e1) == velView.end());
            REQUIRE(std::find(velView.begin(), velView.end(), e2) != velView.end());
            REQUIRE(std::find(velView.begin(), velView.end(), e3) == velView.end());
            REQUIRE(std::find(velView.begin(), velView.end(), e4) != velView.end());
        }

        {
            auto posView = reg.viewAny<Position>();
            REQUIRE(posView.size() == 3);
            REQUIRE(std::find(posView.begin(), posView.end(), e0) == posView.end());
            REQUIRE(std::find(posView.begin(), posView.end(), e1) != posView.end());
            REQUIRE(std::find(posView.begin(), posView.end(), e2) != posView.end());
            REQUIRE(std::find(posView.begin(), posView.end(), e3) != posView.end());
            REQUIRE(std::find(posView.begin(), posView.end(), e4) == posView.end());
    
            auto posOnly = reg.viewAny<Position>(ecs::exclude_t<Velocity>{});
            REQUIRE(posOnly.size() == 2);
            REQUIRE(std::find(posOnly.begin(), posOnly.end(), e0) == posOnly.end());
            REQUIRE(std::find(posOnly.begin(), posOnly.end(), e1) != posOnly.end());
            REQUIRE(std::find(posOnly.begin(), posOnly.end(), e2) == posOnly.end());
            REQUIRE(std::find(posOnly.begin(), posOnly.end(), e3) != posOnly.end());
            REQUIRE(std::find(posOnly.begin(), posOnly.end(), e4) == posOnly.end());
    
            auto velView = reg.viewAny<Position, Velocity>();
            REQUIRE(velView.size() == 4);
            REQUIRE(std::find(velView.begin(), velView.end(), e0) == velView.end());
            REQUIRE(std::find(velView.begin(), velView.end(), e1) != velView.end());
            REQUIRE(std::find(velView.begin(), velView.end(), e2) != velView.end());
            REQUIRE(std::find(velView.begin(), velView.end(), e3) != velView.end());
            REQUIRE(std::find(velView.begin(), velView.end(), e4) != velView.end());
        }

        REQUIRE(reg.view<>().size() == reg.size());
        REQUIRE(reg.viewAny<>().size() == 0);
    }

    SECTION("merge")
    {
        ecs::registry reg2;

        auto e0 = reg.create(Position{1, 0});
        auto e1 = reg.create(Position{0, 1}, Velocity{1, 1});

        auto e2 = reg2.create(Tag{"Hello, World!"});
        auto e3 = reg2.create(Position{1, 1}, Velocity{0, 0});
        auto e4 = reg2.create(Position{1, 2});
        auto e5 = reg2.create(Position{4, 1}, Health{99});

        reg.merge(reg2);

        REQUIRE(reg.size() == 6);
        REQUIRE(reg.view<Position>().size() == 5);

        unsigned e0_found = 0, e1_found = 0, e2_found = 0, e3_found = 0, e4_found = 0, e5_found = 0;
        for(auto e : reg.view<>())
        {
            e0_found += reg.same(e, e0) && reg.get<Position>(e) == Position{1, 0};
            e1_found += reg.same(e, e1) && reg.get<Position>(e) == Position{0, 1} && reg.get<Velocity>(e) == Velocity{1, 1};
            e2_found += reg.same(e, e2, reg2) && reg.get<Tag>(e).s == "Hello, World!";
            e3_found += reg.same(e, e3, reg2) && reg.get<Position>(e) == Position{1, 1} && reg.get<Velocity>(e) == Velocity{0, 0};
            e4_found += reg.same(e, e4, reg2) && reg.get<Position>(e) == Position{1, 2};
            e5_found += reg.same(e, e5, reg2) && reg.get<Position>(e) == Position{4, 1} && reg.get<Health>(e).hp == 99;
        }

        REQUIRE(e0_found == 1);
        REQUIRE(e1_found == 1);
        REQUIRE(e2_found == 1);
        REQUIRE(e3_found == 1);
        REQUIRE(e4_found == 1);
        REQUIRE(e5_found == 1);

        reg.clear();

        reg.merge(reg2.view<Position>(ecs::exclude_t<Velocity>{}), reg2);

        REQUIRE(reg.size() == 2);
    }

    SECTION("registry component copy semantics")
    {
        auto e1 = reg.create<Health>(Health{42});

        ecs::registry second = reg;

        REQUIRE(second.valid(e1));
        REQUIRE(second.has<Health>(e1));
        CHECK(second.get<Health>(e1).hp == 42);

        second.get<Health>(e1).hp = 7;

        REQUIRE(reg.get<Health>(e1).hp != 7);
    }
    SECTION("registry entity copy semantics")
    {
        ecs::registry reg;

        std::vector<ecs::entity> original_ids;
        for(int i = 0; i < 10; ++i) 
        {
            auto e = reg.create<Position, Health>(Position{float(i), float(i)}, Health{unsigned(i)});
            original_ids.push_back(e);
        }
        std::set<ecs::entity> all_ids(original_ids.begin(), original_ids.end());
        REQUIRE(all_ids.size() == original_ids.size());

        ecs::registry copy = reg;

        std::vector<ecs::entity> copy_ids;
        for(int i = 0; i < 10; ++i) 
        {
            auto e = copy.create<Position, Health>(Position{float(i+100), float(i+100)}, Health{unsigned(i)+100});
            copy_ids.push_back(e);
        }

        REQUIRE(reg.size() == original_ids.size());
        REQUIRE(copy.size() == copy_ids.size() + original_ids.size());

        for(auto e : copy_ids) 
        {
            REQUIRE(all_ids.find(e) == all_ids.end());
        }

        for(auto e : original_ids) 
        {
            REQUIRE(reg.valid(e));
            REQUIRE(reg.has<Position>(e));
            REQUIRE(reg.has<Health>(e));
        }

        for(auto e : copy_ids) 
        {
            REQUIRE(copy.valid(e));
            REQUIRE(copy.has<Position>(e));
            REQUIRE(copy.has<Health>(e));
        }
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