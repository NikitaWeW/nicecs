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

TEST_CASE("Create and validate entities", "[ecs]") 
{
    ECS_PROFILE();
    ecs::registry reg;

    REQUIRE(reg.getEntities().empty());

    auto e1 = reg.create<>();
    auto e2 = reg.create<>();

    REQUIRE(reg.valid(e1));
    REQUIRE(reg.valid(e2));
    REQUIRE(e1 != e2);

    auto all = reg.getEntities();
    REQUIRE(all.size() == 2);
    REQUIRE(std::find(all.begin(), all.end(), e1) != all.end());
    REQUIRE(std::find(all.begin(), all.end(), e2) != all.end());
}
TEST_CASE("emplace, has and get components", "[ecs]") 
{
    ECS_PROFILE();
    ecs::registry reg;
    auto e = reg.create<>();

    REQUIRE_FALSE(reg.has<Position>(e));

    reg.emplace<Position>(e, 1.0f, 2.0f);
    REQUIRE(reg.has<Position>(e));

    auto const &constPos = reg.get<Position>(e);
    REQUIRE(constPos.x == 1.0f);
    REQUIRE(constPos.y == 2.0f);

    reg.get<Position>(e).x = 5.0f;
    REQUIRE(reg.get<Position>(e).x == 5.0f);
}
TEST_CASE("Remove components", "[ecs]") 
{
    ECS_PROFILE();
    ecs::registry reg;
    auto e = reg.create<>();

    reg.emplace<Velocity>(e, 0.1f, 0.2f);
    REQUIRE(reg.has<Velocity>(e));

    reg.remove<Velocity>(e);
    REQUIRE_FALSE(reg.has<Velocity>(e));

    REQUIRE_THROWS_AS(reg.remove<Velocity>(e), std::out_of_range);
}
TEST_CASE("View entities by component signature", "[ecs]") 
{
    ECS_PROFILE();
    ecs::registry reg;

    auto a = reg.create<Position>();
    auto b = reg.create<Position, Velocity>();
    auto c = reg.create<Velocity>();

    {
        auto posOnly = reg.view<Position>();
        REQUIRE(posOnly.size() == 2);
        REQUIRE(std::find(posOnly.begin(), posOnly.end(), a) != posOnly.end());
        REQUIRE(std::find(posOnly.begin(), posOnly.end(), b) != posOnly.end());
    }
    {
        auto purePos = reg.view<Position>(ecs::exclude_t<Velocity>{});
        REQUIRE(purePos.size() == 1);
        REQUIRE(purePos.front() == a);
    }
    {
        auto velOnly = reg.view<Velocity>();
        REQUIRE(velOnly.size() == 2);
        REQUIRE(std::find(velOnly.begin(), velOnly.end(), b) != velOnly.end());
        REQUIRE(std::find(velOnly.begin(), velOnly.end(), c) != velOnly.end());
    }
}
TEST_CASE("Destroy entity and its components", "[ecs]") 
{
    ECS_PROFILE();
    ecs::registry reg;
    auto e = reg.create<Position, Velocity>();

    REQUIRE(reg.valid(e));
    REQUIRE(reg.has<Position>(e));
    REQUIRE(reg.has<Velocity>(e));

    reg.destroy(e);
    REQUIRE_FALSE(reg.valid(e));

    REQUIRE_THROWS_AS(reg.getSignature(e), std::invalid_argument);
    REQUIRE_THROWS_AS(reg.has<Position>(e), std::invalid_argument);
}
TEST_CASE("Invalid handles and double-add errors", "[ecs]") 
{
    ECS_PROFILE();
    ecs::registry reg;
    ecs::entity bad = 100001;

    REQUIRE_THROWS_AS(reg.has<Position>(bad), std::invalid_argument);
    REQUIRE_THROWS_AS(reg.add<Position>(bad, Position{}), std::invalid_argument);

    auto e = reg.create<>();
    reg.add<Position>(e, Position{});
    REQUIRE_THROWS_AS(reg.add<Position>(e, Position{}), std::invalid_argument);
}
TEST_CASE("Basic entity creation and destruction", "[ecs]") 
{
    ECS_PROFILE();
    ecs::registry registry;

    SECTION("Create and validate") 
    {
        auto e = registry.create<>();
        REQUIRE(registry.valid(e));
    }

    SECTION("Destroy and invalidate") 
    {
        auto e = registry.create<>();
        REQUIRE(registry.valid(e));
        registry.destroy(e);
        REQUIRE_FALSE(registry.valid(e));
    }

    SECTION("Destroy invalid entity throws") 
    {
        REQUIRE_THROWS_AS(registry.destroy(9999u), std::invalid_argument);
    }
}
TEST_CASE("Component add / remove / access", "[ecs]") 
{
    ECS_PROFILE();
    ecs::registry registry;
    auto e = registry.create<>();
    
    SECTION("Initially no component") 
    {
        REQUIRE_FALSE(registry.has<Position>(e));
    }

    SECTION("Add, has, get, modify, remove") 
    {
        registry.add<Position>(e, Position{1.5f, 2.5f});
        REQUIRE( registry.has<Position>(e) );

        {
            auto p_read = registry.get<Position>(e);
            REQUIRE( p_read.x == 1.5f );
            REQUIRE( p_read.y == 2.5f );
        }

        {
            auto &p_write = registry.get<Position>(e);
            p_write.x = 9.0f;
            p_write.y = -3.0f;
        }
        {
            auto p_read = registry.get<Position>(e);
            REQUIRE( p_read.x == 9.0f );
            REQUIRE( p_read.y == -3.0f );
        }

        registry.remove<Position>(e);
        REQUIRE_FALSE(registry.has<Position>(e));
        REQUIRE_THROWS_AS(registry.get<Position>(e), std::out_of_range);
    }

    SECTION("Adding duplicate component throws") 
    {
        registry.add<Position>(e, Position{0,0});
        REQUIRE_THROWS_AS(registry.add<Position>(e, Position{0,0}), std::invalid_argument);
    }

    SECTION("Removing non-existent component throws") 
    {
        REQUIRE_THROWS_AS(registry.remove<Velocity>(e), std::out_of_range);
    }

    SECTION("Access on invalid entity throws") 
    {
        REQUIRE_THROWS_AS(registry.get<Position>(999u), std::invalid_argument);
        REQUIRE_THROWS_AS(registry.get<Position>(999u), std::invalid_argument);
        REQUIRE_THROWS_AS(registry.has<Position>(999u), std::invalid_argument);
    }
}
TEST_CASE("Views: include and exclude semantics", "[ecs]") 
{
    ECS_PROFILE();
    ecs::registry registry;

    auto e1 = registry.create<Position>(Position{1,1});
    auto e2 = registry.create<Position>(Position{2,2});
    auto e3 = registry.create<Position, Velocity>(Position{3,3}, Velocity{0.5f,0.5f});
    auto e4 = registry.create<Velocity>(Velocity{9,9});

    SECTION("Include only Position") 
    {
        auto viewPos = registry.view<Position>();
        std::vector<ecs::entity> got(viewPos.begin(), viewPos.end());
        std::sort(got.begin(), got.end());
        REQUIRE( got == std::vector<ecs::entity>{e1, e2, e3} );
    }

    SECTION("Include Position, exclude Velocity") 
    {
        auto viewPosNoVel = registry.view<Position>(ecs::exclude_t<Velocity>{});
        std::vector<ecs::entity> got(viewPosNoVel.begin(), viewPosNoVel.end());
        std::sort(got.begin(), got.end());
        REQUIRE( got == std::vector<ecs::entity>{e1, e2} );
    }

    SECTION("Include both Position and Velocity") 
    {
        auto viewPosVel = registry.view<Position, Velocity>();
        REQUIRE( viewPosVel.size() == 1 );
        REQUIRE( viewPosVel.front() == e3 );
    }

    SECTION("Exclude Position only Velocity left") 
    {
        auto viewVelOnly = registry.view<Velocity>(ecs::exclude_t<Position>{});
        REQUIRE( viewVelOnly.size() == 1 );
        REQUIRE( viewVelOnly.front() == e4 );
    }
}
TEST_CASE("Registry basic add/get/remove/has semantics", "[ecs]") 
{
    ECS_PROFILE();
    ecs::registry reg;

    REQUIRE_THROWS_AS(reg.has<Position>(0), std::invalid_argument);
    REQUIRE_THROWS_AS(reg.get<Position>(0), std::invalid_argument);
    REQUIRE_THROWS_AS(reg.remove<Position>(0), std::invalid_argument);
    REQUIRE_THROWS_AS(reg.add<Position>(0, Position{}), std::invalid_argument);

    auto e = reg.create<>();
    REQUIRE(reg.valid(e));
    REQUIRE_FALSE(reg.has<Position>(e));

    REQUIRE_THROWS_AS(reg.get<Position>(e), std::out_of_range);

    Position p{10, 20};
    reg.add<Position>(e, p);
    REQUIRE(reg.has<Position>(e));
    REQUIRE(reg.get<Position>(e) == p);

    REQUIRE_THROWS_AS(reg.add<Position>(e, Position{}), std::invalid_argument);

    reg.remove<Position>(e);
    REQUIRE_FALSE(reg.has<Position>(e));

    REQUIRE_THROWS_AS(reg.remove<Position>(e), std::out_of_range);
}
TEST_CASE("Registry create with initial components", "[ecs]") 
{
    ECS_PROFILE();
    ecs::registry reg;

    auto e = reg.create<Position, Velocity>({5, 5}, {1, -1});
    REQUIRE(reg.has<Position>(e));
    REQUIRE(reg.has<Velocity>(e));
    REQUIRE(reg.get<Position>(e) == Position{5,5});
    REQUIRE(reg.get<Velocity>(e) == Velocity{1,-1});
}
TEST_CASE("Registry destroy cleans up components and signature", "[ecs]") 
{
    ECS_PROFILE();
    ecs::registry reg;
    auto e = reg.create<Position, Velocity>({1,2}, Velocity{0, 1});

    REQUIRE(reg.has<Position>(e));
    REQUIRE(reg.has<Velocity>(e));

    reg.destroy(e);
    REQUIRE_FALSE(reg.valid(e));

    REQUIRE_THROWS_AS(reg.has<Position>(e), std::invalid_argument);
    REQUIRE_THROWS_AS(reg.get<Position>(e), std::invalid_argument);
    REQUIRE_THROWS_AS(reg.remove<Velocity>(e), std::invalid_argument);
}
TEST_CASE("Registry getEntities reflects live entities only", "[ecs]") 
{
    ECS_PROFILE();
    ecs::registry reg;
    reg.create<>();
    auto e2 = reg.create<>();
    reg.create<>();

    auto all = reg.getEntities();
    REQUIRE(all.size() == 3);
    REQUIRE(std::find(all.begin(), all.end(), e2) != all.end());

    reg.destroy(e2);

    auto after = reg.getEntities();
    REQUIRE(after.size() == 2);
    REQUIRE(std::find(after.begin(), after.end(), e2) == after.end());
}
TEST_CASE("Registry example", "[ecs]")
{
    struct position {
        float x;
        float y;

        position(float x = 0, float y = 0) : x(x), y(y) 
        {}
    };
    struct velocity {
        float dx;
        float dy;

        velocity(float dx = 0, float dy = 0) : dx(dx), dy(dy) 
        {}
    };
    struct tag {};

    ecs::registry registry;

    for(auto i = 0u; i < 10u; ++i) 
    {
        const auto entity = registry.create();
        registry.emplace<position>(entity, i * 1.f, i * 1.f);
        if(i % 2 == 0) 
        { registry.emplace<velocity>(entity, i * .1f, i * .1f); }
        
        std::vector<ecs::entity> view = registry.view<position, velocity>(ecs::exclude_t<tag>{});

        for(auto const &e : view) 
        {
            registry.get<position>(e).x += registry.get<velocity>(e).dx;
            registry.get<position>(e).y += registry.get<velocity>(e).dy;
        }

        if(i%2==0)
            registry.destroy(entity);
    }
}
TEST_CASE("Default constructed ecs::sparse_set is empty", "[ecs]")
{
    ecs::sparse_set<std::string> s;

    REQUIRE_FALSE(s.contains(42));
    REQUIRE(s.data().empty());
    REQUIRE(s.begin() == s.end());
}
TEST_CASE("insert and operator[] access elements", "[ecs]")
{
    ecs::sparse_set<std::string> s;
    s.insert(1, "hello");
    s.insert(3, "world");

    REQUIRE(s.contains(1));
    REQUIRE(s.contains(3));
    REQUIRE_FALSE(s.contains(2));

    REQUIRE(s.get(1) == "hello");
    REQUIRE(s[3] == "world");
    REQUIRE(s.data().size() == 2);

    std::vector<std::string> seen;
    for (auto &str : s)
        seen.push_back(str);

    auto actual = s.data();
    REQUIRE(seen == actual);
}
TEST_CASE("emplace constructs non-copyable types in place", "[ecs]")
{
    struct EmplaceOnly
    {
        int value;
        explicit EmplaceOnly(int v) : value(v) 
        {}
        EmplaceOnly(const EmplaceOnly&) = delete;
        EmplaceOnly& operator=(const EmplaceOnly&) = delete;
        EmplaceOnly(EmplaceOnly&&) = default;
        EmplaceOnly& operator=(EmplaceOnly&&) = default;
        bool operator==(EmplaceOnly const& other) const { return value == other.value; }
    };

    ecs::sparse_set<EmplaceOnly> s;
    s.emplace(10, 123);

    REQUIRE(s.contains(10));
    auto &eo = s.get(10);
    REQUIRE(eo.value == 123);
}
TEST_CASE("insert duplicate sparse index throws std::invalid_argument", "[ecs]")
{
    ecs::sparse_set<int> s;
    s.insert(5, 100);
    REQUIRE_THROWS_AS(s.insert(5, 200), std::invalid_argument);
    REQUIRE_THROWS_AS(s.emplace(5, 300), std::invalid_argument);
}
TEST_CASE("remove middle and last elements and maintain packing", "[ecs]")
{
    ecs::sparse_set<std::string> s;
    s.insert(1, "A");
    s.insert(2, "B");
    s.insert(3, "C");
    s.insert(4, "D");
    s.insert(5, "E");
    s.insert(6, "F");

    s.remove(2);
    REQUIRE(s.contains(1));
    REQUIRE_FALSE(s.contains(2));
    REQUIRE(s.contains(3));
    REQUIRE(s.contains(4));
    REQUIRE(s.contains(5));
    REQUIRE(s.contains(6));
    std::vector<std::string> expected1 = { "A", "F", "C", "D", "E" };
    REQUIRE(s.data() == expected1);

    s.remove(5);
    REQUIRE_FALSE(s.contains(5));
    REQUIRE(s.data().size() == 4);
    REQUIRE(std::find(s.data().begin(), s.data().end(), "A") != s.data().end());
    REQUIRE(std::find(s.data().begin(), s.data().end(), "D") != s.data().end());
}
TEST_CASE("remove of non-existing index throws std::out_of_range", "[ecs]")
{
    ecs::sparse_set<int> s;
    REQUIRE_THROWS_AS(s.remove(99), std::out_of_range);
}
TEST_CASE("get of non-existing index throws std::out_of_range", "[ecs]")
{
    ecs::sparse_set<int> s;
    REQUIRE_THROWS_AS(s.get(7), std::out_of_range);
    REQUIRE_THROWS_AS(s[7], std::out_of_range);
}
TEST_CASE("reserve increases dense storage capacity", "[ecs]")
{
    ecs::sparse_set<int> s;
    size_t before = s.data().capacity();
    s.reserve(100);
    REQUIRE(s.data().capacity() >= 100);
    REQUIRE(s.data().capacity() >= before);
}
TEST_CASE("copy and move semantics preserve data correctly", "[ecs]")
{
    ecs::sparse_set<int> original;
    original.insert(1, 10);
    original.insert(2, 20);

    ecs::sparse_set<int> copy = original;
    REQUIRE(copy.contains(1));
    REQUIRE(copy.contains(2));
    REQUIRE(copy.get(1) == 10);
    REQUIRE(copy.get(2) == 20);

    ecs::sparse_set<int> moved = std::move(original);
    REQUIRE(moved.contains(1));
    REQUIRE(moved.contains(2));
    REQUIRE(moved.get(1) == 10);
    REQUIRE(moved.get(2) == 20);
    REQUIRE_NOTHROW(original.contains(1));
}
TEST_CASE("emplacement of a aggregate type in the sparse set", "[ecs]")
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

/** \endcond */