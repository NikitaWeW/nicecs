#include "catch2/catch_test_macros.hpp"
#include "common.hpp"

#include <vector>
#include <set>

TEST_CASE("Sparse set tests", "[ecs][ecs::sparse_set]") {
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

    REQUIRE_THROWS_AS(s.emplace(1), EcsException);

    s.erase(2);
    REQUIRE(s.size() == 5);
    REQUIRE(s.contains(1));
    REQUIRE_FALSE(s.contains(2));
    REQUIRE(s.contains(3));
    REQUIRE(s.contains(4));
    REQUIRE(s.contains(5));
    REQUIRE(s.contains(6));
    std::vector<std::string> expected1 = { "Position", "F", "C", "D", "E" };
    REQUIRE(s.dense() == expected1);

    s.erase(5);
    REQUIRE(s.size() == 4);
    REQUIRE_FALSE(s.contains(5));
    REQUIRE(s.dense().size() == 4);
    REQUIRE(std::find(s.dense().begin(), s.dense().end(), "Position") != s.dense().end());
    REQUIRE(std::find(s.dense().begin(), s.dense().end(), "D") != s.dense().end());

    {
        auto sparse = s.sparse();
        for(auto i : sparse)
            s.erase(i);
    }

    REQUIRE(s.dense().empty());
    REQUIRE(s.empty());
    REQUIRE(s.size() == 0);
    REQUIRE(s.sparse().empty());

    s.shrink_to_fit();

    REQUIRE(s.dense().capacity() == 0);
    REQUIRE(s.sparse().capacity() == 0);

    s.reserve(100);

    REQUIRE(s.dense().capacity() == 100);
    REQUIRE(s.sparse().capacity() == 100);

    REQUIRE_THROWS_AS(s.get(0), EcsException);
    REQUIRE_THROWS_AS(s.erase(0), EcsException);
    REQUIRE_NOTHROW(s[0]);
    s.erase(0);

    s.emplace(1, "Position");
    s.emplace(2, "Velocity");
    s.emplace(3, "C");
    s.emplace(4, "D");
    s.emplace(5, "E");
    s.emplace(6, "F");

    REQUIRE(s.dense().size() == 6);
    REQUIRE(s.sparse().size() == 6);

    s.clear();

    REQUIRE(s.dense().size() == 0);
    REQUIRE(s.sparse().size() == 0);
    REQUIRE(s.sparse().size() == 0);

    s.emplace(2, "Velocity");
    s.emplace(4, "D");
    s.emplace(6, "F");
    s.emplace(1, "Position");
    s.emplace(5, "E");
    s.emplace(3, "C");

    {
        std::vector<std::pair<std::size_t, std::string>> seen;
        for(auto it = s.begin(); it != s.end(); ++it) {
            auto [s, d] = *it;
            seen.emplace_back(s, d);
        }
        REQUIRE(seen.size() == 6);

        REQUIRE(seen[0].first == 2);
        REQUIRE(seen[0].second == "Velocity");
        REQUIRE(seen[1].first == 4);
        REQUIRE(seen[1].second == "D");
        REQUIRE(seen[2].first == 6);
        REQUIRE(seen[2].second == "F");
        REQUIRE(seen[3].first == 1);
        REQUIRE(seen[3].second == "Position");
        REQUIRE(seen[4].first == 5);
        REQUIRE(seen[4].second == "E");
        REQUIRE(seen[5].first == 3);
        REQUIRE(seen[5].second == "C");
    }

    {
        std::vector<std::string> seen;
        for(auto it = s.pDense(); it != s.pDense() + s.size(); ++it) {
            it->append(" Appended");
            seen.emplace_back(*it);
        }
        REQUIRE(seen.size() == 6);

        REQUIRE(seen[0] == "Velocity Appended");
        REQUIRE(seen[1] == "D Appended");
        REQUIRE(seen[2] == "F Appended");
        REQUIRE(seen[3] == "Position Appended");
        REQUIRE(seen[4] == "E Appended");
        REQUIRE(seen[5] == "C Appended");
    }

    s.clear();
    s.emplace(2, "Velocity");
    s.emplace(4, "D");
    s.emplace(6, "F");
    s.emplace(1, "Position");
    s.emplace(5, "E");
    s.emplace(3, "C");

    for(auto it = s.begin(); it != s.end(); ++it) {
        auto [s, d] = *it;
        if(s == 3)
            d = "Cucumber";
    }
    REQUIRE(s.get(3) == "Cucumber");

    auto it = s.begin();
    REQUIRE((it + 1) != it);
    auto pair1 = *(it + 1);
    REQUIRE(pair1.first == s.sparse()[1]);
    auto pr = it[2];
    REQUIRE(pr.second == "F");

    auto it2 = s.begin() + 3;
    REQUIRE((it2 - s.begin()) == 3);
}
TEST_CASE("Emplacement of a aggregate type in the sparse set", "[ecs][ecs::sparse_set]") {
    ecs::sparse_set<Position> s;
    s.emplace(0, 0.0f, 0.0f);
    s.emplace(1, 0.1f, 0.1f);
    s.emplace(2, 0.2f, 0.2f);

    REQUIRE(s.contains(0));
    REQUIRE(s.contains(1));
    REQUIRE(s.contains(2));
    REQUIRE(s.get(0) == Position{ 0.0f, 0.0f });
    REQUIRE(s.get(1) == Position{ 0.1f, 0.1f });
    REQUIRE(s.get(2) == Position{ 0.2f, 0.2f });
}
TEST_CASE("Move only types") {
    static_assert(!std::is_copy_constructible_v<MoveOnly>);

    ecs::sparse_set<MoveOnly> s;
    s.emplace(0, 4);
    s.emplace(1, 1);
    s.emplace(2, 5);

    ecs::registry r;
    auto e = r.create(MoveOnly{ });
    REQUIRE(r.contains<MoveOnly>(e));
    r.get<MoveOnly>(e);
    r.erase<MoveOnly>(e);
    r.emplace<MoveOnly>(e, 1);
    REQUIRE(r.contains<MoveOnly>(e));
    REQUIRE(r.get<MoveOnly>(e).val == 1);
    r.emplace<Position>(e);
}
TEST_CASE("Registry tests", "[ecs][ecs::registry]") {
    ecs::registry reg;

    SECTION("entity creation and destruction") {
        ecs::entity e0 = reg.create();
        ecs::entity e1 = reg.create<Position>();
        ecs::entity e2 = reg.create<Position, Velocity>({ 0.1f, 10 }, { 1, 0 });
        ecs::entity e3 = 0;
        Position copythis{ 4, 2 };
        ecs::entity e4 = reg.create(copythis);
        REQUIRE(reg.size() == 4);

        REQUIRE(reg.valid(e0));
        REQUIRE(reg.empty(e0));
        REQUIRE(reg.size(e0) == 0);
        REQUIRE_FALSE(reg.contains<Position>(e0));
        REQUIRE_FALSE(reg.contains<Velocity>(e0));

        REQUIRE(reg.valid(e1));
        REQUIRE_FALSE(reg.empty(e1));
        REQUIRE(reg.size(e1) == 1);
        REQUIRE(reg.contains<Position>(e1));
        REQUIRE_FALSE(reg.contains<Velocity>(e1));
        REQUIRE(reg.get<Position>(e1) == Position{ });

        REQUIRE(reg.valid(e2));
        REQUIRE_FALSE(reg.empty(e2));
        REQUIRE(reg.size(e2) == 2);
        REQUIRE(reg.contains<Position>(e2));
        REQUIRE(reg.contains<Velocity>(e2));
        REQUIRE(reg.get<Position>(e2) == Position{ 0.1f, 10 });
        REQUIRE(reg.get<Velocity>(e2) == Velocity{ 1, 0 });

        REQUIRE_FALSE(reg.valid(e3));
        REQUIRE_THROWS_AS(reg.contains<Position>(e3), EcsException);
        REQUIRE_THROWS_AS(reg.get<Position>(e3), EcsException);

        REQUIRE(reg.valid(e4));
        REQUIRE_FALSE(reg.empty(e4));
        REQUIRE(reg.size(e4) == 1);
        REQUIRE(reg.contains<Position>(e4));
        REQUIRE_FALSE(reg.contains<Velocity>(e4));
        REQUIRE(reg.get<Position>(e4) == Position{ 4, 2 });
        REQUIRE(copythis == Position{ 4, 2 });

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

    SECTION("component manipulation") {
        ecs::entity e = reg.create();
        REQUIRE(reg.valid(e));
        REQUIRE_FALSE(reg.contains<Position>(e));
        REQUIRE_FALSE(reg.contains<Velocity>(e));

        reg.emplace<Position>(e, 0.0f, 0.0f);
        REQUIRE(reg.valid(e));
        REQUIRE(reg.contains<Position>(e));
        REQUIRE(reg.get<Position>(e) == Position{ 0.0f, 0.0f });

        reg.erase<Position>(e);
        REQUIRE(reg.valid(e));
        REQUIRE_FALSE(reg.contains<Position>(e));
        REQUIRE(reg.empty(e));
        REQUIRE_THROWS_AS(reg.get<Position>(e), EcsException);
    }

    SECTION("registry component copy semantics") {
        auto e1 = reg.create<Health>(Health{ 42 });

        ecs::registry second = reg;

        REQUIRE(second.valid(e1));
        REQUIRE(second.contains<Health>(e1));
        CHECK(second.get<Health>(e1).hp == 42);

        second.get<Health>(e1).hp = 7;

        REQUIRE(reg.get<Health>(e1).hp != 7);
    }
    SECTION("registry entity copy semantics") {
        ecs::registry reg;

        std::vector<ecs::entity> original_ids;
        for(int i = 0; i < 10; ++i) {
            auto e = reg.create<Position, Health>(Position{ float(i), float(i) }, Health{ unsigned(i) });
            original_ids.push_back(e);
        }
        std::set<ecs::entity> all_ids(original_ids.begin(), original_ids.end());
        REQUIRE(all_ids.size() == original_ids.size());

        ecs::registry copy = reg;

        std::vector<ecs::entity> copy_ids;
        for(int i = 0; i < 10; ++i) {
            auto e = copy.create<Position, Health>(Position{ float(i + 100), float(i + 100) }, Health{ unsigned(i) + 100 });
            copy_ids.push_back(e);
        }

        REQUIRE(reg.size() == original_ids.size());
        REQUIRE(copy.size() == copy_ids.size() + original_ids.size());

        for(auto e : copy_ids) {
            REQUIRE(all_ids.find(e) == all_ids.end());
        }

        for(auto e : original_ids) {
            REQUIRE(reg.valid(e));
            REQUIRE(reg.contains<Position>(e));
            REQUIRE(reg.contains<Health>(e));
        }

        for(auto e : copy_ids) {
            REQUIRE(copy.valid(e));
            REQUIRE(copy.contains<Position>(e));
            REQUIRE(copy.contains<Health>(e));
        }
    }
}

TEST_CASE("Views", "[ecs][ecs::view]") {
    ecs::registry reg;
    addEntities(reg, 100'000);

    auto view1 = reg.view<Position, Velocity>(ecs::exclude<Health, Tag>{ });

    for(auto it = view1.begin(); it != view1.end(); ++it) {
        REQUIRE(reg.storage<Position>().contains(*it));
        REQUIRE(reg.storage<Velocity>().contains(*it));
        REQUIRE_FALSE(reg.storage<Health>().contains(*it));
        REQUIRE_FALSE(reg.storage<Tag>().contains(*it));
    }

    reg.clear();

    {
        auto e0 = reg.create();
        auto e1 = reg.create<Position>();
        auto e2 = reg.create<Position, Velocity>();
        auto e3 = reg.create<Position>();
        auto e4 = reg.create<Velocity>();

        auto posView = reg.view<Position>();
        REQUIRE(posView.size() == 3);
        auto it = posView.begin();
        REQUIRE(it != posView.end());
        REQUIRE(reg.valid(*it++));
        REQUIRE(reg.valid(*it));
        ++it;
        REQUIRE(reg.valid(*it));

        REQUIRE(std::find(posView.begin(), posView.end(), e0) == posView.end());
        REQUIRE(std::find(posView.begin(), posView.end(), e1) != posView.end());
        REQUIRE(std::find(posView.begin(), posView.end(), e2) != posView.end());
        REQUIRE(std::find(posView.begin(), posView.end(), e3) != posView.end());
        REQUIRE(std::find(posView.begin(), posView.end(), e4) == posView.end());

        auto posOnly = reg.view<Position>(ecs::exclude<Velocity>{ });
        REQUIRE(posOnly.size() >= 2);
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

    reg.clear();
    {
        auto e0 = reg.create();
        auto e1 = reg.create();
        auto e2 = reg.create();
        auto e3 = reg.create();

        reg.storage<Position>("client"_hs).emplace(e0);
        reg.storage<Position>("server"_hs).emplace(e1);
        reg.storage<Velocity>("client"_hs).emplace(e2);
        reg.storage<Velocity>("server"_hs).emplace(e3);

        auto viewPosClient = ecs::make_view(std::make_tuple(&reg.storage<Position>("client"_hs)));
        auto viewVelClient = ecs::make_view(std::make_tuple(&reg.storage<Velocity>("client"_hs)));
        auto viewPosServer = ecs::make_view(std::make_tuple(&reg.storage<Position>("server"_hs)));
        auto viewVelServer = ecs::make_view(std::make_tuple(&reg.storage<Velocity>("server"_hs)));
        // ...
    }
}

TEST_CASE("Registry example", "[ecs][ecs::registry]") {
    ecs::registry registry;

    for(auto i = 0u; i < 10u; ++i) {
        const auto entity = registry.create();
        registry.emplace<Position>(entity, i * 1.f, i * 1.f);
        if(i % 2 == 0) {
            registry.emplace<Velocity>(entity, i * .1f, i * .1f);
        }
        if(i == 8) {
            registry.emplace<Tag>(entity);
        }
    }

    auto view = registry.view<Position, Velocity>(ecs::exclude<Tag>{ });

    REQUIRE(view.size() == 4);

    for(auto const &e : view) {
        registry.get<Position>(e).x += registry.get<Velocity>(e).dx;
        registry.get<Position>(e).y += registry.get<Velocity>(e).dy;
    }
}

/** \endcond */
