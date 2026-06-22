Actually, its just an EC (entity component) as the library provides no systems api. Nonetheless, i use the ecs term as it is more common.

## Features

- Header only.
- C++17, STL-only.
- Sparse set storage (ecs::sparse_set available for use).

## Documentation
Documentation is generated using doxygen. Simply run

```sh
doxygen
```
in the root folder.

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

    for(auto i = 0u; i < 10u; ++i) 
    {
        // Make an entity
        const auto entity = registry.create();
        // Add a component to an entity
        registry.emplace<position>(entity, i * 1.0f, i * 2.5f);
        if(i % 2 == 0) 
        { 
            registry.emplace<velocity>(entity, i * 0.6f, i * -0.3f); 
        }
        if(i == 8)
        {
            registry.emplace<tag>(entity);
        }
    }

    // Get a list of entities that satisfy requirements
    auto view = registry.view<position, velocity>(ecs::exclude_t<tag>{});

    for(auto const &e : view) 
    {
        registry.get<position>(e).x += registry.get<velocity>(e).dx;
        registry.get<position>(e).y += registry.get<velocity>(e).dy;
    }

    // You can also use sparse set containers separately
    ecs::sparse_set<position> positions; 
    positions.emplace(10, 3.1f, 19.4f);
    assert(positions.contains(10));
    auto targetPosition = positions.get(10);
    positions.erase(10);
}
```

## TODO

- Better component management
  - Runtime components. Maybe https://github.com/skypjack/entt/issues/23
  - Maybe change `ecs::signature` to `std::vector<bool>`?
  - Maybe separate `ecs::sparse_set` into its own header?
  - Fix: registering a component mutates mComponentArrays (destroys const correctness and parallel access to different component arrays).

## Tests and benchmarks
Build the cmake project in the tests directory:

```
cmake -S tests -B build/tests -D CMAKE_BUILD_TYPE=Release
build/tests/tests
# or
cd build/tests
ctest .
```

## Thread safety
The library is not synchronized. Even if it was, it still would be unusable in multithreaded applications, because of stale data issues.

You will need to manually synchronize the usage the library.

## Older development

- https://github.com/NikitaWeW/breakout/blob/45cb3f0df4f6f5712acfa4df22b055edc37b8200/src/utils/ECS.hpp
- https://github.com/NikitaWeW/breakout/tree/locked-ecs
- https://github.com/NikitaWeW/ecs (this repo)

---

- Austin Morlan’s ECS design: https://austinmorlan.com/posts/entity_component_system  
- EnTT library: https://github.com/skypjack/entt
