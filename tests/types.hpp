#pragma once
#include <string>

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
    bool operator==(Tag const& o) const { return s == o.s; }
};

struct Health { 
    unsigned hp; 
};
