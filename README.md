# ECS - My entity component system implementation.

Actually, its just an EC (entity component) as the library provides no systems api, because it is really easy to implement them and each project needs a slightly different approach. Nonetheless, i used the ecs term as it is more common.

An easy-to-integrate, header-only ECS library focused on simplicity and solid performance. Inspired by Austin Morlan’s article on ECS design and the nice api from EnTT.

## Features

- Single header (`ecs.hpp`), pretty small.
- C++17, STL-only.
- Simple API.
- Sparse set storage.
- Type safe lazy (implicit) component registration and lookup.

## Integration

`ecs.hpp` contains the entire library. The `ecs::registry` contains the api.

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

## Tests
Build the cmake project in the tests directory:

```
cmake -S tests -B build/tests
build/tests/tests
# or
cd build/tests
ctest .
```

## Further Reading

- Austin Morlan’s ECS design: https://austinmorlan.com/posts/entity_component_system  
- EnTT library: https://github.com/skypjack/entt

## Older development

When i asked for a small code review, people were skeptical about the ai usage in this project. "AI bullshit", they said.

LLM did not touch a line of the project nor the documentation. Everything is human-written.

The first commit has a lot of contents, because i was moving the code from my main project to the standalone repo.
Here are places, where i developed it in the chronological order:

- https://github.com/NikitaWeW/breakout/blob/45cb3f0df4f6f5712acfa4df22b055edc37b8200/src/utils/ECS.hpp
- https://github.com/NikitaWeW/breakout/tree/locked-ecs
- https://github.com/NikitaWeW/ecs (this repo)
