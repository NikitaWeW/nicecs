# ECS - A Single-Header, C++17 Thread-Safe Entity-Component System

An easy-to-integrate, header-only ECS library focused on simplicity, small footprint, and solid performance. Inspired by Austin Morlan’s article on ECS design and lightweight patterns from EnTT.

## Features

- Single header (`ecs.hpp`), ~1000 lines of code  
- C++17, STL-only  
- Simple API for creating/destroying entities and adding/removing components  
- Sparse-set storage per component type  
- Type-safe component registration and lookup  
- Convenient views (include/exclude filters)  

## Integration

ecs.hpp contains the entire library

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

## Documentation
Documentation is generated using doxygen. Simply run
```
doxygen
```
in the root folder.

## Further Reading

- Austin Morlan’s ECS design: https://austinmorlan.com/posts/entity_component_system  
- EnTT library: https://github.com/skypjack/entt
