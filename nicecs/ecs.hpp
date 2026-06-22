/*
      ___  ___ ___ 
     / _ \/ __/ __|        Copyright (c) 2024 Nikita Martynau 
    |  __/ (__\__ \        https://opensource.org/license/mit 
     \___|\___|___/ v1.5.8 https://github.com/nikitawew/nicecs

Thanks to this article: https://austinmorlan.com/posts/entity_component_system.
Took a very little bit of inspiration from https://github.com/skypjack/entt.
*/

/*
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once
#include <cstdint>
#include <bitset>
#include <memory>
#include <vector>
#include <type_traits>
#include <unordered_map>
#include <algorithm>

/*! \cond Doxygen_Suppress */
// Config section 

#include <cassert>

// Example of using tracy to profile:
// #define ECS_PROFILE ZoneScoped
#ifndef ECS_PROFILE
#define ECS_PROFILE
#endif

// Example of using nicecs with exceptions:
// struct EcsException : std::logic_error { 
//     EcsException() = delete;
//     EcsException(char const *) = delete;
//     inline EcsException(char const *msg, char const *file, int line) : logic_error(file + (":" + std::to_string(line)) + " " + msg) {}
// };
// #define ECS_ASSERT(x, msg) if(!static_cast<bool>(x)) { throw EcsException(msg, __FILE__, __LINE__); }
#ifndef ECS_ASSERT
#define ECS_ASSERT(x, msg) assert((x) && msg)
#endif

#ifndef ECS_MAX_COMPONENTS
#define ECS_MAX_COMPONENTS 1024
#endif

#include "sparse_set.hpp"

/*! \endcond */

namespace ecs
{
    /// @brief Entity ID. Entity 0 is invalid.
    using entity = std::uint32_t;
    /// @brief Component ID. Used with signature.
    /// TODO: Runtime components. Maybe https://github.com/skypjack/entt/issues/23
    using component_id = std::uint32_t;

    /// @brief Controls the maximum number of components allowed to be registered.
    constexpr component_id MAX_COMPONENTS = ECS_MAX_COMPONENTS;

    /// @brief Used to track which components entity has. 
    /// As an example, if Transform has type 0, RigidBody has type 1, and Gravity has type 2, an entity that “has” those three components would have a signature of 0b111 (bits 0, 1, and 2 are set).
    /// Important: signatures are uniform across registries.
    /// TODO: If MAX_COMPONENTS becomes an issue, use std::vector<bool>
    using signature = std::bitset<MAX_COMPONENTS>;

namespace impl
{

    /// @brief Manages entities (create, destroy) and their signatures (set, get).
    /// Any entity supplied to the manager must be created by the same manager object.
    class EntityManager
    {
    private:
        std::vector<entity> mAvailableEntityIDs;
        std::unordered_map<signature, sparse_set<entity>> mEntityGroups;
        std::uint32_t mLivingEntitiesCount = 0;
        sparse_set<signature> mSignatures;
        entity mNextID = 1;
    public:
        explicit EntityManager();
        ~EntityManager() = default;

        /// @brief Creates entity with an optional signature.
        /// @param signature A signature representing components the entity has (optional).
        /// @return Unique entity id.
        entity createEntity(signature signature = {});

        /// @brief Destroys entity.
        /// @param entity A valid entity identifier.
        void destroyEntity(entity const &entity);
        
        /// @brief Sets the signature of the entity.
        /// @param entity A valid entity identifier.
        /// @param signature A new signature,
        void setSignature(entity const &entity, signature signature);

        /// @brief Gets the signature of a valid entity.
        /// @param entity A valid entity identifier.
        /// @return A const reference to a signature, describing the components an entity has.
        signature const &getSignature(entity const &entity) const;

        /// @brief Get entities of this manager.
        /// @return Get a map of the sparse sets of all the valid entities of this registry with their signatures as its key.
        std::unordered_map<signature, sparse_set<entity>> const &getEntityGroups() const;

        /// @brief Checks if an identifier refers to a valid entity.
        /// @param entity An identifier, either valid or not.
        /// @return True if the identifier is valid, false otherwise.
        bool valid(entity const &entity) const;

        /// @brief Get the number of entities alive in a manager.
        std::size_t size() const;
    };

    /// @brief Every instanced component_array is derived from this polymorphic class.
    class IComponentArray 
    {
    public:
        IComponentArray() = default;
        virtual ~IComponentArray() = default;

        /// @brief Notify the array that the entity is destroyed.
        /// @param entity A destroyed entity identifier.
        virtual void onEntityDestroyed(entity const &entity) = 0;

        /// @brief Copy the component of an entity from another component array.
        /// @param other The other component array.
        /// @param to The entity to copy to.
        /// @param from The entity to copy from.
        virtual void copyEntityFrom(IComponentArray const *other, entity const &to, entity const &from) = 0;

        /// @brief Add an entity to the component array.
        /// @param entity A valid entity identifier.
        virtual void addEntity(entity const &entity) = 0;

        /// @brief Create an empty copy of the component array.
        virtual std::unique_ptr<IComponentArray> cloneEmpty() const = 0;

        /// @brief Create a copy of the component array.
        virtual std::unique_ptr<IComponentArray> clone() const = 0;
    };

    /// @brief Stores components of entities of a specific type as a sparse set.
    /// @tparam component_t The type of stored components.
    template <typename component_t>
    class ComponentArray : public IComponentArray, public sparse_set<component_t>
    {
    public:
        /// @brief The size of a component page in bytes.
        static constexpr std::size_t PAGE_SIZE = 4096;

        explicit ComponentArray();

        /// @copydoc ecs::impl::IComponentArray::onEntityDestroyed
        void onEntityDestroyed(entity const &entity) override;
        
        /// @copydoc ecs::impl::IComponentArray::copyEntityFrom
        void copyEntityFrom(IComponentArray const *other, entity const &to, entity const &from) override;

        /// @copydoc ecs::impl::IComponentArray::addEntity
        void addEntity(entity const &entity) override;

        /// @copydoc ecs::impl::IComponentArray::cloneEmpty
        std::unique_ptr<IComponentArray> cloneEmpty() const override;

        /// @copydoc ecs::impl::IComponentArray::clone
        std::unique_ptr<IComponentArray> clone() const override;
    };

    /// @brief Manages components and their arrays. All components are destroyed automatically.
    class ComponentManager
    {
    private:
        sparse_set<std::unique_ptr<IComponentArray>> mComponentArrays;
        inline static component_id mNextID = 0;
    public:
        /// @brief Get unique component ID used to index the signature bitset.
        /// @tparam component_t The component type.
        /// The id is the same between the managers.
        template <typename component_t> 
        static component_id getComponentID();

        /// @brief Get the next component id.
        static std::size_t getNextID();
    public:
        ComponentManager() = default;
        ComponentManager(ComponentManager const &other);
        ComponentManager &operator=(ComponentManager const &other);
        ~ComponentManager() = default;

        /// @brief Registers component.
        /// @param array An optional component array.
        /// @tparam component_t The component type.
        /// This should be called for every component used. Multiple calls for the same component_t will do nothing.
        /// FIXME: Registering a component mutates mComponentArrays.
        /// Lazy component initialization destroys const correctness and parallelizing per component storage access.
        template <typename component_t> 
        void registerComponent(std::unique_ptr<ecs::impl::IComponentArray> &&array = nullptr);

        /// @brief Notify component arrays that the entity is destroyed.
        /// @param entity A deleted entity identifier.
        void entityDestroyed(entity const &entity) const;

        /// @brief Get the component array associated with the given component type.
        /// @tparam component_t The component type.
        template <typename component_t> 
        impl::ComponentArray<component_t> *getComponentArray();
        /// @copydoc getComponentArray
        template <typename component_t>
        impl::ComponentArray<component_t> const *getComponentArray() const;

        sparse_set<std::unique_ptr<IComponentArray>> &getComponentArrays();
        sparse_set<std::unique_ptr<IComponentArray>> const &getComponentArrays() const;
    };

}; // namespace impl

    /// @brief Exclusion type list.
    /// A class to push around lists of types.
    /// @tparam Type List of types.
    template<typename... Type>
    struct exclude {};

    /// @brief An ECS interface.
    /// Contains entities and their components.
    class registry
    {
    private:
        impl::EntityManager mEntityManager;
        // ugly fix for lazy component registration.
        mutable impl::ComponentManager mComponentManager;
    public:
        registry() = default;
        ~registry() = default;
        registry(registry const &other);
        registry(registry &&other) noexcept;
        registry &operator=(registry const &other);
        registry &operator=(registry &&other) noexcept;

        /// @copydoc impl::EntityManager::valid
        bool valid(entity const &entity) const;

        /// @brief Checks if a valid entity has a component.
        /// @param entity A valid entity identifier.
        /// @tparam component_t The component type.
        /// @throws std::invalid_argument if the entity is not a valid identifier.
        /// @return True if the entity has the component, false otherwise.
        template <typename component_t> 
        bool has(entity const &entity) const;

        /// @brief Gets a component from a a valid entity.
        /// @param entity A valid entity identifier.
        /// @tparam component_t The component type.
        /// @throws std::invalid_argument if the entity is not a valid identifier.
        /// @throws std::out_of_range if the component is not added.
        /// @return The component lvalue reference.
        template <typename component_t> 
        component_t &get(entity const &entity);
        /// @copydoc get
        template <typename component_t> 
        component_t const &get(entity const &entity) const;

        /// @brief Removes a component from a valid entity.
        /// @param entity A valid entity identifier.
        /// @throws std::invalid_argument if the entity is not a valid identifier.
        /// @throws std::out_of_range if the component is not added.
        /// @tparam component_t The component type.
        template <typename component_t> 
        void remove(entity const &entity);
        
        /// @copydoc impl::component_manager::emplace
        /// @throws std::invalid_argument if the entity is not a valid identifier.
        /// @throws std::invalid_argument if the component is already added.
        template <typename component_t, class... Args>
        void emplace(entity const &entity, Args&&... args);

        /// @brief Create an entity.
        /// @tparam Components_t Components (optional).
        /// @return Unique valid entity id.
        /// @throws std::invalid_argument If the same component is added more than once.
        template <typename... Components_t> 
        entity create();

        /// @brief Create an entity.
        /// @tparam Components_t Components (optional).
        /// @param components The components to move in.
        /// @return Unique valid entity id.
        /// @throws std::invalid_argument If the same component is added more than once.
        template <typename... Components_t> 
        entity create(Components_t&&... components);

        /// @brief Destroys an entity and its components.
        /// @param entity A valid entity identifier.
        /// @throws std::invalid_argument if the entity is not a valid identifier.
        void destroy(entity const &entity);

        /// @brief Check if an entity has no components.
        /// @throws std::invalid_argument if the entity is not a valid identifier.
        /// @return True if the entity is empty, false otherwise.
        bool empty(entity const &entity) const;

        /// @brief Clear the registry.
        /// Destroys all the entities in the registry.
        void clear();

        /// @brief Get the number of components in an entity.
        /// @throws std::invalid_argument if the entity is not a valid identifier.
        std::size_t size(entity const &entity) const;

        /// @brief Get the number of entities in a registry.
        std::size_t size() const;

        /// @brief Returns a view for the given elements.
        /// @tparam Include Types of included elements used to construct the view.
        /// @tparam Exclude Types of elements used to filter the view.
        /// @param toExclude The type list used to deduce Exclude variadic template argument.
        /// @return A view on entities that are valid at the time of calling this member function and contain given included components and do not contain excluded ones at the time of calling this member function.
        /// view<>() returns all the entities in the registry.
        template<typename... Include, typename... Exclude>
        std::vector<entity> view(exclude<Exclude...> toExclude = exclude{}) const;

        /// @brief Returns a view for the given elements.
        /// @tparam May Types of elements that entity may have used to construct the view.
        /// @tparam Exclude Types of elements used to filter the view.
        /// @param toExclude The type list used to deduce Exclude variadic template argument.
        /// @return A view on entities that are valid at the time of calling this member function and contain given included components and do not contain excluded ones at the time of calling this member function.
        /// viewAny<>() returns none of the entities in the registry.
        template<typename... May, typename... Exclude>
        std::vector<entity> viewAny(exclude<Exclude...> toExclude = exclude{}) const;

        /// @brief Copy an entity from the other registry.
        /// @param otherEntity The entity from @p other registry to copy.
        /// @param other The registry to copy from.
        /// @return The copied entity.
        /// Adds the entity and its components from the other registry to this registry.
        ecs::entity copy(entity const &otherEntity, registry const &other);

        /// @brief Get the component manager.
        /// Use at your own risk.
        impl::ComponentManager const &getComponentManager() const;
        /// @copydoc getComponentManager
        impl::ComponentManager &getComponentManager();

        /// @brief Get the component manager.
        /// Use at your own risk.
        impl::EntityManager const &getEntityManager() const;
        /// @copydoc getEntityManager
        impl::EntityManager &getEntityManager();
    };
} // namespace ecs

/*! \cond Doxygen_Suppress */

inline ecs::impl::EntityManager::EntityManager()
{
    ECS_PROFILE;
    mAvailableEntityIDs.reserve(1000);
    mSignatures.reserve(1000);
}
inline ecs::entity ecs::impl::EntityManager::createEntity(signature signature)
{
    ECS_PROFILE;
    entity entity = 0;
    if(mAvailableEntityIDs.empty())
    {
        entity = mNextID++;
    } else {
        entity = mAvailableEntityIDs.back();
        mAvailableEntityIDs.pop_back();
    }
    ++mLivingEntitiesCount;
    mEntityGroups[signature].emplace(entity, entity);

    mSignatures.emplace(entity, signature);

    return entity;
}
inline void ecs::impl::EntityManager::destroyEntity(entity const &entity)
{
    ECS_PROFILE;
    ECS_ASSERT(valid(entity), "Invalid entity identifier");
    --mLivingEntitiesCount;
    mAvailableEntityIDs.push_back(entity);

    auto &group = mEntityGroups[getSignature(entity)];
    group.erase(entity);
    if(group.empty())
        mEntityGroups.erase(getSignature(entity));

    mSignatures.erase(entity);
}
inline void ecs::impl::EntityManager::setSignature(entity const &entity, signature signature)
{
    ECS_PROFILE;
    ECS_ASSERT(valid(entity), "Invalid entity identifier");

    auto &group = mEntityGroups[getSignature(entity)];
    group.erase(entity);
    if(group.empty())
        mEntityGroups.erase(getSignature(entity));

    mSignatures[entity] = signature;

    mEntityGroups[signature].emplace(entity, entity);

}
inline ecs::signature const &ecs::impl::EntityManager::getSignature(entity const &entity) const
{
    ECS_ASSERT(valid(entity), "Invalid entity identifier");
    
    return mSignatures.get(entity);
}
inline bool ecs::impl::EntityManager::valid(entity const &entity) const
{
    return 1 <= entity && entity < mNextID && mSignatures.contains(entity);
}
inline std::size_t ecs::impl::EntityManager::size() const
{
    return mLivingEntitiesCount;
}
inline std::unordered_map<ecs::signature, ecs::sparse_set<ecs::entity>> const &ecs::impl::EntityManager::getEntityGroups() const
{
    return mEntityGroups;
} 

template <typename component_t>
ecs::impl::ComponentArray<component_t>::ComponentArray() : sparse_set<component_t>(10, (PAGE_SIZE + sizeof(component_t) - 1) / sizeof(component_t)) {}
template <typename component_t>
inline void ecs::impl::ComponentArray<component_t>::onEntityDestroyed(entity const &entity)
{
    ECS_PROFILE;
    if(this->contains(entity))
        this->erase(entity);
}
template <typename component_t>
inline void ecs::impl::ComponentArray<component_t>::copyEntityFrom(impl::IComponentArray const *other, entity const &to, entity const &from)
{
    ECS_PROFILE;
    ECS_ASSERT(other, "Internal logic error");
    ecs::impl::ComponentArray<component_t> const *otherArray = static_cast<ecs::impl::ComponentArray<component_t> const *>(other);
    ECS_ASSERT(this->contains(to) && otherArray->contains(from), "Internal logic error");
    this->get(to) = otherArray->get(from);
}
template <typename component_t>
inline void ecs::impl::ComponentArray<component_t>::addEntity(entity const &entity)
{
    ECS_PROFILE;
    ECS_ASSERT(!this->contains(entity), "Component array already has the entity");
    this->emplace(entity);
}
template <typename component_t>
inline std::unique_ptr<ecs::impl::IComponentArray> ecs::impl::ComponentArray<component_t>::cloneEmpty() const
{
    ECS_PROFILE;
    return std::unique_ptr<ecs::impl::IComponentArray>{new ecs::impl::ComponentArray<component_t>{}};
}
template <typename component_t>
inline std::unique_ptr<ecs::impl::IComponentArray> ecs::impl::ComponentArray<component_t>::clone() const
{
    ECS_PROFILE;
    return std::unique_ptr<ecs::impl::IComponentArray>{new ecs::impl::ComponentArray<component_t>{*this}};
}

template <typename component_t>
inline void ecs::impl::ComponentManager::registerComponent(std::unique_ptr<ecs::impl::IComponentArray> &&array)
{
    ECS_PROFILE;
    auto id = getComponentID<component_t>();
    if(!mComponentArrays.contains(id))
        mComponentArrays.emplace(id, array ? std::move(array) : std::make_unique<impl::ComponentArray<component_t>>());
}
template <typename component_t>
inline ecs::component_id ecs::impl::ComponentManager::getComponentID()
{
    ECS_ASSERT(mNextID < MAX_COMPONENTS, "Too many components registered");
    static const component_id id = mNextID++;
    return id;
}
template <typename component_t>
inline ecs::impl::ComponentArray<component_t> *ecs::impl::ComponentManager::getComponentArray()
{
    ECS_PROFILE;
    auto id = getComponentID<component_t>();
    ECS_ASSERT(mComponentArrays.contains(id), "Component not registered before use");
    if(!mComponentArrays.contains(id))
        return nullptr;
    return static_cast<impl::ComponentArray<component_t> *>(mComponentArrays.get(id).get());
}
template <typename component_t>
inline ecs::impl::ComponentArray<component_t> const *ecs::impl::ComponentManager::getComponentArray() const
{
    ECS_PROFILE;
    auto id = getComponentID<component_t>();
    ECS_ASSERT(mComponentArrays.contains(id), "Component not registered before use");
    if(!mComponentArrays.contains(id))
        return nullptr;
    return static_cast<impl::ComponentArray<component_t> const *>(mComponentArrays.get(id).get());
}
inline std::size_t ecs::impl::ComponentManager::getNextID()
{
    return mNextID;
}
inline ecs::impl::ComponentManager::ComponentManager(impl::ComponentManager const &other)
{
    this->operator=(other);
}
inline ecs::impl::ComponentManager &ecs::impl::ComponentManager::operator=(impl::ComponentManager const &other)
{
    ECS_PROFILE;
    for(auto [id, ptr] : other.mComponentArrays)
    {
        mComponentArrays.emplace(id, ptr->clone());
    }

    return *this;
}
inline void ecs::impl::ComponentManager::entityDestroyed(entity const &entity) const
{
    ECS_PROFILE;
    for(auto &componentArray : mComponentArrays.dense()) {
        componentArray->onEntityDestroyed(entity);
    }
}
inline ecs::sparse_set<std::unique_ptr<ecs::impl::IComponentArray>> &ecs::impl::ComponentManager::getComponentArrays()
{
    return mComponentArrays;
}
inline ecs::sparse_set<std::unique_ptr<ecs::impl::IComponentArray>> const &ecs::impl::ComponentManager::getComponentArrays() const
{
    return mComponentArrays;
}

template <typename component_t>
inline bool ecs::registry::has(entity const &entity) const
{ 
    ECS_PROFILE;
    ECS_ASSERT(valid(entity), "Invalid entity identifier");
    
    mComponentManager.registerComponent<component_t>();
    return mEntityManager.getSignature(entity).test(impl::ComponentManager::getComponentID<component_t>()); 
}
template <typename component_t>
inline component_t &ecs::registry::get(entity const &entity) 
{
    ECS_PROFILE;
    ECS_ASSERT(valid(entity), "Invalid entity identifier");
    ECS_ASSERT(has<component_t>(entity), "Component to get is not added");
    
    return mComponentManager.getComponentArray<component_t>()->get(entity);
}
template <typename component_t>
inline component_t const &ecs::registry::get(entity const &entity) const
{
    ECS_PROFILE;
    ECS_ASSERT(valid(entity), "Invalid entity identifier");
    ECS_ASSERT(has<component_t>(entity), "Component to get is not added");
    
    return mComponentManager.getComponentArray<component_t>()->get(entity);
}
template <typename... Components_t>
inline ecs::entity ecs::registry::create()
{
    ECS_PROFILE;
    
    (mComponentManager.registerComponent<Components_t>(), ...);
    entity entity = mEntityManager.createEntity({});

    (emplace<Components_t>(entity), ...);

    return entity;
}
template <typename... Components_t>
inline ecs::entity ecs::registry::create(Components_t &&...components)
{
    ECS_PROFILE;
    
    (mComponentManager.registerComponent<std::decay_t<Components_t>>(), ...);
    entity entity = mEntityManager.createEntity({});

    (emplace<std::decay_t<Components_t>>(entity, std::forward<Components_t>(components)), ...);

    return entity;
}
template <typename component_t>
inline void ecs::registry::remove(entity const &entity)
{
    ECS_PROFILE;
    ECS_ASSERT(valid(entity), "Invalid entity identifier");
    ECS_ASSERT(has<component_t>(entity), "Component to remove is not added");
    
    mEntityManager.setSignature(entity, signature{mEntityManager.getSignature(entity)}.set(impl::ComponentManager::getComponentID<component_t>(), false));
    mComponentManager.getComponentArray<component_t>()->erase(entity);
}
template <typename component_t, class... Args>
inline void ecs::registry::emplace(entity const &entity, Args&&... args)
{
    ECS_PROFILE;
    ECS_ASSERT(valid(entity), "Invalid entity identifier");
    ECS_ASSERT(!has<component_t>(entity), "Component to emplace already added");

    mComponentManager.getComponentArray<component_t>()->emplace(entity, std::forward<Args>(args)...);
    mEntityManager.setSignature(entity, signature{mEntityManager.getSignature(entity)}.set(impl::ComponentManager::getComponentID<component_t>(), true));
}
inline ecs::registry::registry(registry const &other)
{
    *this = other;
}
inline ecs::registry::registry(registry &&other) noexcept
{
    *this = std::move(other);
}
inline ecs::registry &ecs::registry::operator=(registry const &other)
{
    ECS_PROFILE;
    mEntityManager = other.mEntityManager;
    mComponentManager = other.mComponentManager;
    return *this;
}
inline ecs::registry &ecs::registry::operator=(registry &&other) noexcept
{
    ECS_PROFILE;
    std::swap(mEntityManager, other.mEntityManager);
    std::swap(mComponentManager, other.mComponentManager);
    return *this;
}
inline bool ecs::registry::valid(entity const &entity) const
{
    return mEntityManager.valid(entity);
}
inline void ecs::registry::destroy(ecs::entity const &entity)
{
    ECS_PROFILE;
    ECS_ASSERT(valid(entity), "Invalid entity identifier");

    mEntityManager.destroyEntity(entity);
    mComponentManager.entityDestroyed(entity);
}
inline bool ecs::registry::empty(entity const &entity) const
{
    ECS_PROFILE;
    ECS_ASSERT(valid(entity), "Invalid entity identifier");
    return mEntityManager.getSignature(entity).none();
}
inline void ecs::registry::clear() 
{
    ECS_PROFILE;
    for(entity const &e : view<>())
        destroy(e);
}
inline std::size_t ecs::registry::size(entity const &entity) const 
{
    ECS_PROFILE;
    ECS_ASSERT(valid(entity), "Invalid entity identifier");
    return mEntityManager.getSignature(entity).count();
}
inline std::size_t ecs::registry::size() const
{
    ECS_PROFILE;
    return mEntityManager.size();
}
inline ecs::entity ecs::registry::copy(entity const &otherEntity, registry const &other)
{
    ECS_PROFILE;
    auto signature = other.mEntityManager.getSignature(otherEntity);
    for(std::size_t id = 0; id < impl::ComponentManager::getNextID(); ++id) 
    {
        if(!signature.test(id)) 
            continue;
        if(!mComponentManager.getComponentArrays().contains(id))
            mComponentManager.getComponentArrays().emplace(id, other.mComponentManager.getComponentArrays().get(id)->cloneEmpty());
    }

    entity entity = mEntityManager.createEntity(signature);
    for(std::size_t id = 0; id < impl::ComponentManager::getNextID(); ++id)
    {
        if(signature.test(id))
        {
            ECS_ASSERT(mComponentManager.getComponentArrays().contains(id), "Unregistered component (internal logic error)");
            mComponentManager.getComponentArrays().get(id)->addEntity(entity);
        }
        else continue;

        mComponentManager.getComponentArrays().get(id)->copyEntityFrom(other.mComponentManager.getComponentArrays().get(id).get(), entity, otherEntity);
    }

    return entity;
}
template <typename... Include, typename... Exclude>
inline std::vector<ecs::entity> ecs::registry::view(exclude<Exclude...>) const
{
    ECS_PROFILE;
    ecs::signature required;
    ecs::signature excluded;
    (required.set(impl::ComponentManager::getComponentID<Include>()), ...);
    (excluded.set(impl::ComponentManager::getComponentID<Exclude>()), ...);

    std::vector<ecs::entity> result;
    result.reserve(10);

    for(auto const &[signature, group] : mEntityManager.getEntityGroups())
    {
        if((signature & required) == required && (signature & excluded).none())
            result.insert(result.end(), group.dense().begin(), group.dense().end());
    }

    return result;
}
template <typename... May, typename... Exclude>
inline std::vector<ecs::entity> ecs::registry::viewAny(exclude<Exclude...> toExclude) const 
{
    ECS_PROFILE;
    ecs::signature required;
    ecs::signature excluded;
    (required.set(impl::ComponentManager::getComponentID<May>()), ...);
    (excluded.set(impl::ComponentManager::getComponentID<Exclude>()), ...);

    std::vector<ecs::entity> result;
    result.reserve(10);

    for(auto const &[signature, group] : mEntityManager.getEntityGroups())
    {
        if((signature & required).any() && (signature & excluded).none())
            result.insert(result.end(), group.dense().begin(), group.dense().end());
    }

    return result;
}
inline ecs::impl::ComponentManager const &ecs::registry::getComponentManager() const { return mComponentManager; }
inline ecs::impl::ComponentManager &ecs::registry::getComponentManager() { return mComponentManager; }
inline ecs::impl::EntityManager const &ecs::registry::getEntityManager() const { return mEntityManager; }
inline ecs::impl::EntityManager &ecs::registry::getEntityManager() { return mEntityManager; }

/*! \endcond */
