#pragma once
#include <string>
#include <stdexcept>

// operators == are for testing only.

struct Position {
    float x = 0, y = 0;
    bool operator==(Position const& o) const { return x == o.x && y == o.y; }
};

struct Velocity {
    float dx = 0, dy = 0;
    bool operator==(Velocity const& o) const { return dx == o.dx && dy == o.dy; }
};

struct Tag {
    std::string s;
};

struct Health { 
    unsigned hp; 
};

struct EcsException : std::logic_error { 
    EcsException() = delete;
    EcsException(char const *) = delete;
    inline EcsException(char const *msg, char const *file, int line) : logic_error(file + (":" + std::to_string(line)) + " " + msg) {}
};
#define ECS_ASSERT(x, msg) if(!static_cast<bool>(x)) { throw EcsException(msg, __FILE__, __LINE__); }