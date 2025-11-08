Actually, its just an EC (entity component) as the library provides no systems api. Nonetheless, i use the ecs term as it is more common.

## Features

- Single header (`ecs.hpp`), pretty small.
- C++17, STL-only.
- Simple API.
- Sparse set storage (ecs::sparse_set available for use).
- Type safe component manipulation.
- Thread safe ([kinda](#thread-safety)).

## Integration

`nicecs/ecs.hpp` contains the entire library. The `ecs::registry` contains the api.
The library works well with [cpm.cmake](https://github.com/cpm-cmake/CPM.cmake).

## Documentation
Documentation is generated using doxygen. Simply run
```
doxygen
```
in the root folder.

## Code Example

```cpp
#include "ecs.hpp"

struct position {
    float x;
    float y;
};
struct velocity {
    float dx;
    float dy;

    velocity(float dx = 0, float dy = 0) : dx(dx), dy(dy) 
    {}
};
struct tag {};

int main()
{
    ecs::registry registry;

    for(auto i = 0u; i < 10u; ++i) 
    {
        const auto entity = registry.create();
        registry.emplace<position>(entity, i * 1.f, i * 1.f);
        if(i % 2 == 0) 
        { 
            registry.emplace<velocity>(entity, i * .1f, i * .1f); 
        }
        if(i == 8)
        {
            registry.emplace<tag>(entity);
        }
    }

    auto view = registry.view<position, velocity>(ecs::exclude_t<tag>{});

    for(auto const &e : view) 
    {
        registry.get<position>(e).x += registry.get<velocity>(e).dx;
        registry.get<position>(e).y += registry.get<velocity>(e).dy;
    }
}
```

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
You can redefine ECS_SYNCHRONIZE(...) to disable synchronization.
```c
#define ECS_SYNCHRONIZE(...)
```

`ecs::registry` is synchronized, however beware of getting references or pointers to components, 
because `ecs::sparse_set` maintains dense storage by moving components, which invalidates pointers / references. 
Also the components might be modified by another thread.

## Older development

- https://github.com/NikitaWeW/breakout/blob/45cb3f0df4f6f5712acfa4df22b055edc37b8200/src/utils/ECS.hpp
- https://github.com/NikitaWeW/breakout/tree/locked-ecs
- https://github.com/NikitaWeW/ecs (this repo)

---

- Austin Morlan’s ECS design: https://austinmorlan.com/posts/entity_component_system  
- EnTT library: https://github.com/skypjack/entt
