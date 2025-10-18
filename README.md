Actually, its just an EC (entity component) as the library provides no systems api, because it is really easy to implement them and each project needs a slightly different approach. Nonetheless, i used the ecs term as it is more common.

## Features

- Single header (`ecs.hpp`), pretty small.
- C++17, STL-only.
- Simple API.
- Sparse set storage (ecs::sparse_set available for use).
- Type safe component manipulation.

## Integration

`ecs.hpp` contains the entire library. The `ecs::registry` contains the api.
The library works with [cpm.cmake](https://github.com/cpm-cmake/CPM.cmake).

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
## Latest Benchmarks
```
-------------------------------------------------------------------------------
ecs::registry benchmarks
-------------------------------------------------------------------------------
benchmark name                       samples       iterations    est run time
                                     mean          low mean      high mean
                                     std dev       low std dev   high std dev
-------------------------------------------------------------------------------
view                                           100           178     1.3172 ms 
                                        78.0993 ns    76.0046 ns    85.6976 ns 
                                        18.1363 ns    4.97324 ns    41.7022 ns 
                                                                               
merged                                         100             1    88.5168 ms 
                                        880.299 us    874.434 us    888.826 us 
                                        35.8386 us    27.2969 us    45.8779 us 
                                                                               
create and emplace                             100             3     1.8255 ms 
                                        9.43451 us    9.25802 us    9.81493 us 
                                        1.26295 us    725.996 ns     2.4677 us 
```

## Older development

- https://github.com/NikitaWeW/breakout/blob/45cb3f0df4f6f5712acfa4df22b055edc37b8200/src/utils/ECS.hpp
- https://github.com/NikitaWeW/breakout/tree/locked-ecs
- https://github.com/NikitaWeW/ecs (this repo)

## Further Reading

- Austin Morlan’s ECS design: https://austinmorlan.com/posts/entity_component_system  
- EnTT library: https://github.com/skypjack/entt
