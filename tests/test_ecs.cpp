#include "catch2/catch_test_macros.hpp"
#include "catch2/benchmark/catch_benchmark.hpp"
#include "ecs.hpp"
#include "ecs.hpp" // check if there are no odr issues
// #include <thread>
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

struct Health {
    float hp = 100;
    bool operator==(Health const& o) const { return hp == o.hp; }
};

TEST_CASE("Create and validate entities", "[entity]") {
    ECS_PROFILE();
    ecs::registry reg;

    // Initially no entities
    REQUIRE(reg.getEntities().empty());

    // Create two empty entities
    auto e1 = reg.create<>();
    auto e2 = reg.create<>();

    // Both should be valid and distinct
    REQUIRE(reg.valid(e1));
    REQUIRE(reg.valid(e2));
    REQUIRE(e1 != e2);

    // getEntities returns both
    auto all = reg.getEntities();
    REQUIRE(all.size() == 2);
    REQUIRE(std::find(all.begin(), all.end(), e1) != all.end());
    REQUIRE(std::find(all.begin(), all.end(), e2) != all.end());
}
TEST_CASE("emplace, has and get components", "[component]") {
    ECS_PROFILE();
    ecs::registry reg;
    auto e = reg.create<>();

    // Initially, no Position component
    REQUIRE_FALSE(reg.has<Position>(e));

    // Add and verify
    reg.emplace<Position>(e, 1.0f, 2.0f);
    REQUIRE(reg.has<Position>(e));

    auto const &constPos = reg.get<Position>(e);
    REQUIRE(constPos.x == 1.0f);
    REQUIRE(constPos.y == 2.0f);

    reg.get<Position>(e).x = 5.0f;
    REQUIRE(reg.get<Position>(e).x == 5.0f);
}
TEST_CASE("Remove components", "[component]") {
    ECS_PROFILE();
    ecs::registry reg;
    auto e = reg.create<>();

    reg.emplace<Velocity>(e, 0.1f, 0.2f);
    REQUIRE(reg.has<Velocity>(e));

    reg.remove<Velocity>(e);
    REQUIRE_FALSE(reg.has<Velocity>(e));

    // Removing again should throw out_of_range
    REQUIRE_THROWS_AS(reg.remove<Velocity>(e), std::out_of_range);
}
TEST_CASE("View entities by component signature", "[view]") {
    ECS_PROFILE();
    ecs::registry reg;

    // Create 3 entities with different combos
    auto a = reg.create<Position>();
    auto b = reg.create<Position, Velocity>();
    auto c = reg.create<Velocity>();

    {
        // View entities that have Position
        auto posOnly = reg.view<Position>();
        REQUIRE(posOnly.size() == 2);
        REQUIRE(std::find(posOnly.begin(), posOnly.end(), a) != posOnly.end());
        REQUIRE(std::find(posOnly.begin(), posOnly.end(), b) != posOnly.end());
    }
    {
        // View entities that have Position but exclude Velocity
        auto purePos = reg.view<Position>(ecs::exclude_t<Velocity>{});
        REQUIRE(purePos.size() == 1);
        REQUIRE(purePos.front() == a);
    }
    {
        // View only Velocity
        auto velOnly = reg.view<Velocity>();
        REQUIRE(velOnly.size() == 2);
        REQUIRE(std::find(velOnly.begin(), velOnly.end(), b) != velOnly.end());
        REQUIRE(std::find(velOnly.begin(), velOnly.end(), c) != velOnly.end());
    }
}
TEST_CASE("Destroy entity and its components", "[destroy]") {
    ECS_PROFILE();
    ecs::registry reg;
    auto e = reg.create<Position, Velocity>();

    REQUIRE(reg.valid(e));
    REQUIRE(reg.has<Position>(e));
    REQUIRE(reg.has<Velocity>(e));

    reg.destroy(e);
    REQUIRE_FALSE(reg.valid(e));

    // After destroy, getSignature or has should throw invalid_argument
    REQUIRE_THROWS_AS(reg.getSignature(e), std::invalid_argument);
    REQUIRE_THROWS_AS(reg.has<Position>(e), std::invalid_argument);
}
TEST_CASE("Invalid handles and double-add errors", "[errors]") {
    ECS_PROFILE();
    ecs::registry reg;
    ecs::entity bad = 100001;

    // invalid entity
    REQUIRE_THROWS_AS(reg.has<Position>(bad), std::invalid_argument);
    REQUIRE_THROWS_AS(reg.add<Position>(bad, Position{}), std::invalid_argument);

    auto e = reg.create<>();
    // double add same component
    reg.add<Position>(e, Position{});
    REQUIRE_THROWS_AS(reg.add<Position>(e, Position{}), std::invalid_argument);
}
TEST_CASE("Basic entity creation and destruction", "[entity]") {
    ECS_PROFILE();
    ecs::registry registry;

    SECTION("Create and validate") {
        auto e = registry.create<>();
        REQUIRE(registry.valid(e));
    }

    SECTION("Destroy and invalidate") {
        auto e = registry.create<>();
        REQUIRE(registry.valid(e));
        registry.destroy(e);
        REQUIRE_FALSE(registry.valid(e));
    }

    SECTION("Destroy invalid entity throws") {
        REQUIRE_THROWS_AS(registry.destroy(9999u), std::invalid_argument);
    }
}
TEST_CASE("Component add / remove / access", "[component]") {
    ECS_PROFILE();
    ecs::registry registry;
    auto e = registry.create<>();
    
    SECTION("Initially no component") {
        REQUIRE_FALSE(registry.has<Position>(e));
    }

    SECTION("Add, has, get, modify, remove") {
        registry.add<Position>(e, Position{1.5f, 2.5f});
        REQUIRE( registry.has<Position>(e) );

        {
            // shared get
            auto p_read = registry.get<Position>(e);
            REQUIRE( p_read.x == 1.5f );
            REQUIRE( p_read.y == 2.5f );
        }

        {
            // unique lock and modify
            auto &p_write = registry.get<Position>(e);
            p_write.x = 9.0f;
            p_write.y = -3.0f;
        }
        {
            auto p_read = registry.get<Position>(e);
            REQUIRE( p_read.x == 9.0f );
            REQUIRE( p_read.y == -3.0f );
        }

        // remove and recheck
        registry.remove<Position>(e);
        REQUIRE_FALSE(registry.has<Position>(e));
        REQUIRE_THROWS_AS(registry.get<Position>(e), std::out_of_range);
    }

    SECTION("Adding duplicate component throws") {
        registry.add<Position>(e, Position{0,0});
        REQUIRE_THROWS_AS(registry.add<Position>(e, Position{0,0}), std::invalid_argument);
    }

    SECTION("Removing non-existent component throws") {
        REQUIRE_THROWS_AS(registry.remove<Velocity>(e), std::out_of_range);
    }

    SECTION("Access on invalid entity throws") {
        REQUIRE_THROWS_AS(registry.get<Position>(999u), std::invalid_argument);
        REQUIRE_THROWS_AS(registry.get<Position>(999u), std::invalid_argument);
        REQUIRE_THROWS_AS(registry.has<Position>(999u), std::invalid_argument);
    }
}
TEST_CASE("Views: include and exclude semantics", "[view]") {
    ECS_PROFILE();
    ecs::registry registry;

    auto e1 = registry.create<Position>(Position{1,1});
    auto e2 = registry.create<Position>(Position{2,2});
    auto e3 = registry.create<Position, Velocity>(Position{3,3}, Velocity{0.5f,0.5f});
    auto e4 = registry.create<Velocity>(Velocity{9,9});

    SECTION("Include only Position") {
        auto viewPos = registry.view<Position>();
        std::vector<ecs::entity> got(viewPos.begin(), viewPos.end());
        std::sort(got.begin(), got.end());
        REQUIRE( got == std::vector<ecs::entity>{e1, e2, e3} );
    }

    SECTION("Include Position, exclude Velocity") {
        auto viewPosNoVel = registry.view<Position>(ecs::exclude_t<Velocity>{});
        std::vector<ecs::entity> got(viewPosNoVel.begin(), viewPosNoVel.end());
        std::sort(got.begin(), got.end());
        REQUIRE( got == std::vector<ecs::entity>{e1, e2} );
    }

    SECTION("Include both Position and Velocity") {
        auto viewPosVel = registry.view<Position, Velocity>();
        REQUIRE( viewPosVel.size() == 1 );
        REQUIRE( viewPosVel.front() == e3 );
    }

    SECTION("Exclude Position only Velocity left") {
        auto viewVelOnly = registry.view<Velocity>(ecs::exclude_t<Position>{});
        REQUIRE( viewVelOnly.size() == 1 );
        REQUIRE( viewVelOnly.front() == e4 );
    }
}
TEST_CASE("EntityManager: create, destroy, valid, signature", "[entity_manager]") {
    ECS_PROFILE();
    ecs::entity_manager em;

    // Initially no entities exist
    REQUIRE_FALSE(em.valid(1));

    // Create one entity
    auto e1 = em.createEntity();
    REQUIRE(e1 >= 1);
    REQUIRE(em.valid(e1));

    // Signature should be empty by default
    auto sig0 = em.getSignature(e1);
    for (size_t i = 0; i < sig0.size(); ++i)
        REQUIRE_FALSE(sig0.test(i));

    // Set and get signature
    ecs::signature s2;
    s2.set(3).set(5);
    em.setSignature(e1, s2);
    auto got = em.getSignature(e1);
    REQUIRE(got.test(3));
    REQUIRE(got.test(5));
    REQUIRE_FALSE(got.test(4));

    // Destroying makes it invalid
    em.destroyEntity(e1);
    REQUIRE_FALSE(em.valid(e1));
}
TEST_CASE("ComponentManager: register & id uniqueness", "[component_manager]") {
    ECS_PROFILE();
    ecs::component_manager cm;

    // Register two component types
    cm.registerComponent<Position>();
    cm.registerComponent<Velocity>();

    auto pid = cm.getComponentID<Position>();
    auto vid = cm.getComponentID<Velocity>();
    REQUIRE(pid != vid);

    // Re-registering Position doesn't change its ID
    cm.registerComponent<Position>();
    REQUIRE(cm.getComponentID<Position>() == pid);
}
TEST_CASE("Registry basic add/get/remove/has semantics", "[registry][components]") {
    ECS_PROFILE();
    ecs::registry reg;

    // Invalid entity queries
    REQUIRE_THROWS_AS(reg.has<Position>(0), std::invalid_argument);
    REQUIRE_THROWS_AS(reg.get<Position>(0), std::invalid_argument);
    REQUIRE_THROWS_AS(reg.remove<Position>(0), std::invalid_argument);
    REQUIRE_THROWS_AS(reg.add<Position>(0, Position{}), std::invalid_argument);

    // Create entity with no components
    auto e = reg.create<>();
    REQUIRE(reg.valid(e));
    REQUIRE_FALSE(reg.has<Position>(e));

    // get() on missing component
    REQUIRE_THROWS_AS(reg.get<Position>(e), std::out_of_range);

    // add a component
    Position p{10, 20};
    reg.add<Position>(e, p);
    REQUIRE(reg.has<Position>(e));
    REQUIRE(reg.get<Position>(e) == p);

    // add same component again → invalid_argument
    REQUIRE_THROWS_AS(reg.add<Position>(e, Position{}), std::invalid_argument);

    // remove component
    reg.remove<Position>(e);
    REQUIRE_FALSE(reg.has<Position>(e));

    // remove missing → out_of_range
    REQUIRE_THROWS_AS(reg.remove<Position>(e), std::out_of_range);
}
TEST_CASE("Registry create with initial components", "[registry][create]") {
    ECS_PROFILE();
    ecs::registry reg;

    auto e = reg.create<Position, Velocity>({5, 5}, {1, -1});
    REQUIRE(reg.has<Position>(e));
    REQUIRE(reg.has<Velocity>(e));
    REQUIRE(reg.get<Position>(e) == Position{5,5});
    REQUIRE(reg.get<Velocity>(e) == Velocity{1,-1});
}
TEST_CASE("Registry destroy cleans up components and signature", "[registry][destroy]") {
    ECS_PROFILE();
    ecs::registry reg;
    auto e = reg.create<Position, Health>({1,2}, Health{50});

    REQUIRE(reg.has<Position>(e));
    REQUIRE(reg.has<Health>(e));

    reg.destroy(e);
    REQUIRE_FALSE(reg.valid(e));

    // After destruction, queries throw invalid_argument
    REQUIRE_THROWS_AS(reg.has<Position>(e), std::invalid_argument);
    REQUIRE_THROWS_AS(reg.get<Position>(e), std::invalid_argument);
    REQUIRE_THROWS_AS(reg.remove<Health>(e), std::invalid_argument);
}
TEST_CASE("Registry view with includes and excludes", "[registry][view]") {
    ECS_PROFILE();
    ecs::registry reg;

    // Create 4 entities with various component combos
    // e1: Position only
    auto e1 = reg.create<Position>({0,0});
    // e2: Position + Velocity
    auto e2 = reg.create<Position, Velocity>({1,1}, {2,2});
    // e3: Velocity only
    auto e3 = reg.create<Velocity>({-1,0});
    // e4: Position + Health
    auto e4 = reg.create<Position, Health>({5,5}, Health{80});

    // View entities with Position (include)
    auto posOnly = reg.view<Position>();
    REQUIRE(std::find(posOnly.begin(), posOnly.end(), e1) != posOnly.end());
    REQUIRE(std::find(posOnly.begin(), posOnly.end(), e2) != posOnly.end());
    REQUIRE(std::find(posOnly.begin(), posOnly.end(), e4) != posOnly.end());
    REQUIRE(std::find(posOnly.begin(), posOnly.end(), e3) == posOnly.end());

    // View entities with Position but exclude Velocity
    auto vExcluded = reg.view<Position>(ecs::exclude_t<Velocity>{});
    REQUIRE(std::find(vExcluded.begin(), vExcluded.end(), e1) != vExcluded.end());
    REQUIRE(std::find(vExcluded.begin(), vExcluded.end(), e4) != vExcluded.end());
    REQUIRE(std::find(vExcluded.begin(), vExcluded.end(), e2) == vExcluded.end());

    // View pure Velocity
    auto velOnly = reg.view<Velocity>(ecs::exclude_t<Position>{});
    REQUIRE(velOnly.size() == 1);
    REQUIRE(velOnly[0] == e3);

    // View with no matches
    auto none = reg.view<Health>(ecs::exclude_t<Health>{});
    REQUIRE(none.empty());
}
TEST_CASE("Registry getEntities reflects live entities only", "[registry][entities]") {
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
TEST_CASE("Registry example", "[registry]")
{
    struct position {
        float x;
        float y;

        // cant emplace structs using brace initialization D:
        position(float x = 0, float y = 0) : x(x), y(y) {}
    };
    struct velocity {
        float dx;
        float dy;

        velocity(float dx = 0, float dy = 0) : dx(dx), dy(dy) {}
    };
    struct tag {};

    ecs::registry registry;

    for(auto i = 0u; i < 10u; ++i) {
        const auto entity = registry.create();
        registry.emplace<position>(entity, i * 1.f, i * 1.f);
        if(i % 2 == 0) { registry.emplace<velocity>(entity, i * .1f, i * .1f); }
        
        std::vector<ecs::entity> view = registry.view<position, velocity>(ecs::exclude_t<tag>{});

        for(auto const &e : view) {
            registry.get<position>(e).x += registry.get<velocity>(e).dx;
            registry.get<position>(e).y += registry.get<velocity>(e).dy;
        }

        if(i%2==0)
            registry.destroy(entity);
    }
}

TEST_CASE("Default constructed ecs::sparse_set is empty", "[ecs::sparse_set][empty]")
{
    ecs::sparse_set<int, std::string> s;

    REQUIRE_FALSE(s.contains(42));
    REQUIRE(s.data().empty());
    REQUIRE(s.begin() == s.end());
}

TEST_CASE("insert and operator[] access elements", "[ecs::sparse_set][insert][access]")
{
    ecs::sparse_set<int, std::string> s;
    s.insert(1, "hello");
    s.insert(3, "world");

    REQUIRE(s.contains(1));
    REQUIRE(s.contains(3));
    REQUIRE_FALSE(s.contains(2));

    REQUIRE(s.get(1) == "hello");
    REQUIRE(s[3] == "world");
    REQUIRE(s.data().size() == 2);

    // range-based for and iterators yield the dense storage order
    std::vector<std::string> seen;
    for (auto &str : s)
        seen.push_back(str);

    auto actual = s.data();
    REQUIRE(seen == actual);
}

TEST_CASE("emplace constructs non-copyable types in place", "[ecs::sparse_set][emplace]")
{
    struct EmplaceOnly
    {
        int value;
        explicit EmplaceOnly(int v) : value(v) {}
        EmplaceOnly(const EmplaceOnly&) = delete;
        EmplaceOnly& operator=(const EmplaceOnly&) = delete;
        EmplaceOnly(EmplaceOnly&&) = default;
        EmplaceOnly& operator=(EmplaceOnly&&) = default;
        bool operator==(EmplaceOnly const& other) const { return value == other.value; }
    };

    ecs::sparse_set<int, EmplaceOnly> s;
    s.emplace(10, 123);

    REQUIRE(s.contains(10));
    auto &eo = s.get(10);
    REQUIRE(eo.value == 123);
}

TEST_CASE("insert duplicate sparse index throws std::invalid_argument", "[ecs::sparse_set][error]")
{
    ecs::sparse_set<int, int> s;
    s.insert(5, 100);
    REQUIRE_THROWS_AS(s.insert(5, 200), std::invalid_argument);
    REQUIRE_THROWS_AS(s.emplace(5, 300), std::invalid_argument);
}

TEST_CASE("remove middle and last elements and maintain packing", "[ecs::sparse_set][remove]")
{
    ecs::sparse_set<int, std::string> s;
    s.insert(1, "A");
    s.insert(2, "B");
    s.insert(3, "C");
    s.insert(4, "D");
    // Dense layout: ["A","B","C","D"]

    // Remove an element in the middle (key=2)
    s.remove(2);
    REQUIRE_FALSE(s.contains(2));
    REQUIRE(s.contains(1));
    REQUIRE(s.contains(3));
    REQUIRE(s.contains(4));
    // "D" should have moved into B's slot
    std::vector<std::string> expected1 = { "A", "D", "C" };
    REQUIRE(s.data() == expected1);

    // Remove last element (key=3 or 4 depending on previous move)
    int lastKey = s.data().back() == "C" ? 3 : 4;
    s.remove(lastKey);
    REQUIRE_FALSE(s.contains(lastKey));
    REQUIRE(s.data().size() == 2);
    REQUIRE(std::find(s.data().begin(), s.data().end(), "A") != s.data().end());
    REQUIRE(std::find(s.data().begin(), s.data().end(), "D") != s.data().end());
}

TEST_CASE("remove of non-existing index throws std::out_of_range", "[ecs::sparse_set][error]")
{
    ecs::sparse_set<int, int> s;
    REQUIRE_THROWS_AS(s.remove(99), std::out_of_range);
}

TEST_CASE("get of non-existing index throws std::out_of_range", "[ecs::sparse_set][error]")
{
    ecs::sparse_set<int, int> s;
    REQUIRE_THROWS_AS(s.get(7), std::out_of_range);
    REQUIRE_THROWS_AS(s[7], std::out_of_range);
}

TEST_CASE("reserve increases dense storage capacity", "[ecs::sparse_set][reserve]")
{
    ecs::sparse_set<int, int> s;
    size_t before = s.data().capacity();
    s.reserve(100);
    REQUIRE(s.data().capacity() >= 100);
    REQUIRE(s.data().capacity() >= before);
}

TEST_CASE("copy and move semantics preserve data correctly", "[ecs::sparse_set][copy][move]")
{
    ecs::sparse_set<int, int> original;
    original.insert(1, 10);
    original.insert(2, 20);

    ecs::sparse_set<int, int> copy = original;
    REQUIRE(copy.contains(1));
    REQUIRE(copy.contains(2));
    REQUIRE(copy.get(1) == 10);
    REQUIRE(copy.get(2) == 20);

    ecs::sparse_set<int, int> moved = std::move(original);
    REQUIRE(moved.contains(1));
    REQUIRE(moved.contains(2));
    REQUIRE(moved.get(1) == 10);
    REQUIRE(moved.get(2) == 20);
    // original is in a valid but unspecified state; we at least know it won't crash
    REQUIRE_NOTHROW(original.contains(1));
}
TEST_CASE("emplacement of a aggregate type in the sparse set", "[ecs::sparse_set]")
{
    ecs::sparse_set<int, Position> s;
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

/*
TEST_CASE("Concurrent read access to the same component", "[concurrency][read]") {
    ECS_PROFILE();
    ecs::registry registry;
    constexpr int N = 1000;
    constexpr int R = 4;

    // create N entities with Position{x=1.0}
    for(int i = 0; i < N; ++i)
        registry.create<Position>(Position{1.0f, 0.0f});

    // spawn R reader threads that sum x
    std::vector<float> sums(R, 0.0f);
    std::vector<std::thread> readers;
    for(int t = 0; t < R; ++t) {
        readers.emplace_back([t, &registry, &sums]() {
            float local = 0.0f;
            auto view = registry.view<Position>();
            for(auto e : view)
                local += registry.get<Position>(e).x;
            sums[t] = local;
        });
    }
    for(auto &th : readers) th.join();

    // each reader should see sum == N * 1.0f
    for(int t = 0; t < R; ++t)
        REQUIRE( abs(sums[t] - static_cast<float>(N)) < 0.01 );
}
TEST_CASE("Concurrent entity creation and destruction", "[concurrency][write]") {
    ECS_PROFILE();
    ecs::registry registry;
    constexpr int T = 4;
    constexpr int M = 500;

    // each thread will create M entities of type Health and record them
    std::array<std::vector<ecs::entity>, T> created;
    std::vector<std::thread> creators;
    for(int t = 0; t < T; ++t) {
        creators.emplace_back([t, &registry, &created]() {
            for(int i = 0; i < M; ++i) {
                auto e = registry.create<Health>(Health{100});
                created[t].push_back(e);
            }
        });
    }
    for(auto &th : creators) th.join();

    // total entities == T * M
    auto all_entities = registry.getEntities();
    REQUIRE( static_cast<int>(all_entities.size()) == T * M );

    // now destroy them concurrently in the same thread groups
    std::vector<std::thread> destroyers;
    for(int t = 0; t < T; ++t) {
        destroyers.emplace_back([t, &registry, &created]() {
            for(auto e : created[t]) {
                registry.destroy(e);
            }
        });
    }
    for(auto &th : destroyers) th.join();

    // registry should be empty
    REQUIRE( registry.getEntities().empty() );
}
TEST_CASE("System basic usage", "[system]") {
    ECS_PROFILE();
    ecs::registry registry;

    struct Input
    {
        Velocity movementDir{0, 0};
    };
    struct NoMove {};
    
    ecs::system inputSystem = {
        .update = [](ecs::registry &reg){
            for(auto entity : reg.view<Input>())
                reg.destroy(entity);
            reg.create(Input{
                .movementDir = {0, 1}
            });
        },
        .write = registry.makeSignature<Input>()
    };
    ecs::system movementSystem = {
        .update = [](ecs::registry &reg){
            for(auto einput : reg.view<Input>())
            {
                auto &input = reg.get<Input>(einput);
                for(auto e : reg.view<Position>(ecs::exclude_t<NoMove>{}))
                {
                    auto &pos = reg.get<Position>(e);
                    pos.x += input.movementDir.dx;
                    pos.y += input.movementDir.dy;
                }
            }
        },
        .read = registry.makeSignature<Input, NoMove>(),
        .write = registry.makeSignature<Position>()
    };
    ecs::system rendererSystem = {
        .update = [](ecs::registry &reg){
            std::this_thread::sleep_for(std::chrono::milliseconds{10});
        }
    };

    auto e0 = registry.create<Position, NoMove>({-3, 4.5}, {});
    auto e1 = registry.create<Position>({-1, -3});
    auto e2 = registry.create<Position>();


    registry.systems().push_back(inputSystem);
    registry.systems().push_back(movementSystem);
    registry.systems().push_back(rendererSystem);

    registry.update();
    registry.update();
    registry.update();

    REQUIRE(registry.get<Position>(e0) == Position{-3, 4.5});
    REQUIRE(registry.get<Position>(e1) == Position{-1, 0});
    REQUIRE(registry.get<Position>(e2) == Position{ 0, 3});
}
*/

/** \endcond */