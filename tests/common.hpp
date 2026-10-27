#pragma once
#include <random>
#include <stdexcept>
#include <string>

struct EcsException : std::logic_error {
    EcsException() = delete;
    EcsException(char const *) = delete;
    inline EcsException(char const *msg, char const *file, int line): logic_error(file + (":" + std::to_string(line)) + " " + msg) { }
};
#define ECS_ASSERT(x, msg) if(!static_cast<bool>(x)) { throw EcsException(msg, __FILE__, __LINE__); }
#include "nicecs/ecs.hpp"
#include "nicecs/ecs.hpp"

// operators == are for testing only.

struct Position {
    float x = 0, y = 0;
    bool operator==(Position const &o) const { return x == o.x && y == o.y; }
};

struct Velocity {
    float dx = 0, dy = 0;
    bool operator==(Velocity const &o) const { return dx == o.dx && dy == o.dy; }
};

struct Tag {
    std::string s;
};

struct Health {
    unsigned hp;
};

struct MoveOnly {
    int val = -1;
    MoveOnly(int _val = 0) : val(_val) {}
    MoveOnly(MoveOnly const &) = delete;
    MoveOnly(MoveOnly &&) = default;

    MoveOnly &operator=(MoveOnly const &) = delete;
    MoveOnly &operator=(MoveOnly &&) = default;
};

using ecs::operator""_hs;
inline void addEntities(ecs::registry &reg, ecs::size_t size = 10'000) {
    std::mt19937_64 rng(12345);
    std::uniform_int_distribution<int> dist(0, 1);

    for(std::size_t i = 0; i < size; ++i) {
        auto e = reg.create();

        if(dist(rng)) reg.emplace<Position>(e, float(i), float(i + 1));
        if(dist(rng)) reg.emplace<Velocity>(e, float(i) * 123, float(i) * 312);
        if(dist(rng)) reg.emplace<Health>(e, unsigned(i) % 100);
        if(dist(rng)) reg.storage<Position>("named"_hs).emplace(e, float(i*10.9-41), float(100 - i));
        if(dist(rng)) reg.storage<Velocity>("named"_hs).emplace(e, float(i*21.5-83), float(14 - 0.4*i));
        if(dist(rng)) reg.storage<Health>("named1"_hs).emplace(e, unsigned(i) % 99);
    }
}
