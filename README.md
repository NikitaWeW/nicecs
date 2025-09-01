# ECS - My entity component system implementation.

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
};
struct tag {};

int main()
{
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
```

## Older development and ai usage

When i asked for a small code review, people was skeptical about the ai usage in this project. "AI bullshit", they said.

I'll be honest, AI code disgusts me. Nonetheless, i did use it to speed up the testing, nothing more. Seeing people criticize me on using it for such an unrelated tasks really upsets me, since in my opinion i did nothing wrong. 

LLM did not touch a line of the actual project nor the documentation. However, i kept the tests, because there is no point in rewriting those.

The first commit has a lot of contents, because i was moving the code from my main project to the standalone repo.
Here are places, where i developed it in the chronological order:

- https://github.com/NikitaWeW/breakout/blob/45cb3f0df4f6f5712acfa4df22b055edc37b8200/src/utils/ECS.hpp
- https://github.com/NikitaWeW/breakout/tree/locked-ecs
- https://github.com/NikitaWeW/ecs (this repo)

## Tests
Compile the cmake project in the tests directory.

## Further Reading

- Austin Morlan’s ECS design: https://austinmorlan.com/posts/entity_component_system  
- EnTT library: https://github.com/skypjack/entt
