Actually, its just an EC (entity component), as the library provides no systems utilities. Nonetheless, I use the term ecs as it is more common.

## Features

- Header only.
- C++17, STL-only.
- Sparse set storage (ecs::sparse_set available for use in nicecs/sparse_set.hpp).

---

<!-- *Note: the git tags / releases might be outdated, just use latest commit* -->

## Usage

```cpp
#include "ecs.hpp"

struct position {
    float x;
    float y;
};
struct velocity {
    float dx;
    float dy;
};
struct tag {};

int main()
{
    // Registry keeps track of all the entities and stores their component data
    ecs::registry registry;

    for(auto i = 0u; i < 10u; ++i) {
        // Make an entity
        ecs::entity entity = registry.create();
        // Add a component to an entity
        registry.emplace<Position>(entity, i * 1.f, i * 1.f);
        if(i % 2 == 0) {
            registry.emplace<Velocity>(entity, i * .1f, i * .1f);
        }
        if(i == 8) {
            registry.emplace<Tag>(entity);
        }
    }

    // Get the entities that satisfy requirements
    ecs::view view = registry.view<Position, Velocity>(ecs::exclude<Tag>{ });

    for(auto const &e : view) {
        registry.get<Position>(e).x += registry.get<Velocity>(e).dx;
        registry.get<Position>(e).y += registry.get<Velocity>(e).dy;
    }

    // You can also use sparse set containers separately
    ecs::sparse_set<position> positions; 
    positions.emplace(10, 3.1f, 19.4f);
    assert(positions.contains(10));
    auto targetPosition = positions.get(10);
    positions.erase(10);
}
```

## Documentation
There are doxygen comments I guess. 
The usage is pretty straight forward and the library is not that big.

## Tests and benchmarks
Build the cmake project in the tests directory:

```
cmake -S tests -B build -D CMAKE_BUILD_TYPE=Release
build/tests
build/benchmarks
```

## Thread safety
The library is not synchronized. Even if it was, it still would be unusable in multithreaded applications, because of stale data issues.
You will need to manually synchronize the usage the library.

<!-- ## Older development

- https://github.com/NikitaWeW/breakout/blob/45cb3f0df4f6f5712acfa4df22b055edc37b8200/src/utils/ECS.hpp
- https://github.com/NikitaWeW/breakout/tree/locked-ecs
- https://github.com/NikitaWeW/nicecs / https://codeberg.org/nikitam/nicecs (this repo) -->

## Some ECS resources:

- [Austin Morlan’s ECS design.](https://austinmorlan.com/posts/entity_component_system)
- [EnTT library.](https://github.com/skypjack/entt)
- [ECS faq.](Https://github.com/SanderMertens/ecs-faq)

---

*No generative AI was used.*
