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
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <memory>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>

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

#include "sparse_set.hpp"

/*! \endcond */

namespace ecs {

/// @brief Entity ID. Entity 0 is invalid.
using entity = std::uint32_t;
/// @brief Component ID.
using component_id = std::uint32_t;

/// @brief Get an id for each component type.
template <typename T>
component_id get_component_id() noexcept;

/// @brief Combine hashes.
/// Can be used for ecs::registry::storage<>().
component_id combine_component_ids(component_id const &lhs, component_id const &rhs) noexcept;

/// @brief Get a combined id of component type and name hash.
/// Can be used for ecs::registry::storage<>().
template <typename T>
component_id get_component_id(std::string_view name) noexcept;

template <typename T>
using storage_t = ecs::sparse_set<T>;

/// @brief Exclusion type list.
/// A class to push around lists of types.
/// @tparam Type List of types.
// template <typename... Type>
// struct exclude { };

template <typename... T>
using exclude = std::tuple<T...>;

template <typename, typename>
class view;

template <typename... Type>
view(Type const *...storage) -> view<std::tuple<Type...>, std::tuple<>>;

template <typename... Include, typename... Exclude>
view(std::tuple<Include const *...>, std::tuple<Exclude const *...> = { }) -> view<std::tuple<Include...>, std::tuple<Exclude...>>;

template <typename... Include, typename... Exclude>
class view<std::tuple<Include...>, std::tuple<Exclude...>> {
    static_assert(sizeof...(Include) != 0u);
private:
    std::tuple<Include const *...> mInclude;
    std::tuple<Exclude const *...> mExclude;
public:
    constexpr view() noexcept = default;
    constexpr view(std::tuple<Include const *...> include, std::tuple<Exclude const *...> exclude = { }) noexcept;

    /// @brief View iterator.
    /// The view iterators 
    class iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = entity;
        using difference_type = std::ptrdiff_t;
        using pointer = void;
        using reference = value_type;
    private:
        std::tuple<Include const *...> mInclude;
        std::tuple<Exclude const *...> mExclude;
    public:

        
    };
};

namespace impl {

    /// @brief Manages entities duh.
    class EntityManager {
    private:
        sparse_set<std::size_t> mEntities;
        std::vector<entity> mAvailableEntities;
        entity mNextID = 1;
    public:
        using iterator = std::remove_reference_t<decltype(mEntities.sparse())>::const_iterator;
        explicit EntityManager();
        ~EntityManager() = default;

        /// @brief Creates a unique entity.
        /// @return Unique entity id.
        entity emplace();

        /// @brief Destroys entity.
        /// @param entity A valid entity identifier.
        void erase(entity const &entity);

        /// @brief Checks if an identifier refers to a valid entity.
        /// @param entity An identifier, either valid or not.
        /// @return True if the identifier is valid, false otherwise.
        bool contains(entity const &entity) const;

        /// @brief Erases all entities.
        void clear();

        /// @brief Get the number of entities alive in a manager.
        std::size_t size() const;

        /// @brief Get the number of components in an entity.
        std::size_t size(entity const &entity) const;

        /// @brief Increment number of components in an entity.
        void incSize(entity const &entity);
        /// @brief Decrement number of components in an entity.
        void decSize(entity const &entity);

        /// @brief
        iterator begin() const;
        iterator end() const;
    };

    inline std::atomic<component_id> nextComponentId{ 0 };

    /// @brief Type erased sparse set.
    class IComponentArray {
    public:
        IComponentArray() = default;
        virtual ~IComponentArray() = default;

        /// @brief Erase an entity's component.
        /// @param entity An entity identifier.
        virtual void erase(entity const &entity) = 0;

        /// @brief Check if an entity has this component.
        /// @param entity An entity identifier.
        virtual bool contains(entity const &entity) const = 0;

        /// @brief Erases all elements.
        virtual void clear() = 0;

        /// @brief Create a copy of the component array.
        virtual std::unique_ptr<IComponentArray> clone() const = 0;
    };

    /// @brief Stores components of entities of a specific type as a sparse set.
    /// @tparam component_t The type of stored components.
    template <typename component_t>
    class ComponentArray : public IComponentArray, public sparse_set<component_t> {
    public:
        /// @brief The size of a component sparse page.
        static constexpr std::size_t PAGE_SIZE = 32;
        /// @brief The starting capacity to reserve in bytes.
        static constexpr std::size_t CAPACITY = 256;

        explicit ComponentArray();

        /// @copydoc ecs::impl::IComponentArray::erase
        void erase(entity const &entity) override;

        /// @copydoc ecs::impl::IComponentArray::contains
        bool contains(entity const &entity) const override;

        /// @copydoc ecs::impl::IComponentArray::clear
        void clear() override;

        /// @copydoc ecs::impl::IComponentArray::clone
        std::unique_ptr<IComponentArray> clone() const override;
    };

    /// @brief Manages components and their arrays. All components are destroyed automatically.
    class ComponentManager {
    private:
        sparse_set<std::unique_ptr<IComponentArray>> mComponentArrays;
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
        void erase(entity const &entity) const;

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

/// @brief An ECS interface.
/// Contains entities and their components.
class registry {
private:
    impl::EntityManager mEntityManager;
    // ugly fix for lazy component registration.
    mutable impl::ComponentManager mComponentManager;
public:
    using iterator = impl::EntityManager::iterator;

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
    /// @throws If the entity is not a valid identifier.
    /// @return True if the entity has the component, false otherwise.
    template <typename component_t>
    bool contains(entity const &entity) const;

    /// @brief Gets a component from a a valid entity.
    /// @param entity A valid entity identifier.
    /// @tparam component_t The component type.
    /// @throws If the entity is not a valid identifier.
    /// @throws If the component is not added.
    /// @return The component lvalue reference.
    template <typename component_t>
    component_t &get(entity const &entity);
    /// @copydoc get
    template <typename component_t>
    component_t const &get(entity const &entity) const;

    /// @brief Removes a component from a valid entity.
    /// @param entity A valid entity identifier.
    /// @throws If the entity is not a valid identifier.
    /// @throws If the component is not added.
    /// @tparam component_t The component type.
    template <typename component_t>
    void erase(entity const &entity);

    /// @copydoc impl::component_manager::emplace
    /// @throws If the entity is not a valid identifier.
    /// @throws If the component is already added.
    template <typename component_t, class... Args>
    void emplace(entity const &entity, Args &&...args);

    /// @brief Create an entity.
    /// @tparam Components_t Components (optional).
    /// @return Unique valid entity id.
    /// @throws If the same component is added more than once.
    template <typename... Components_t>
    entity create();

    /// @brief Create an entity.
    /// @tparam Components_t Components (optional).
    /// @param components The components to move in.
    /// @return Unique valid entity id.
    /// @throws If the same component is added more than once.
    template <typename... Components_t>
    entity create(Components_t &&...components);

    /// @brief Destroys an entity and its components.
    /// @param entity A valid entity identifier.
    /// @throws If the entity is not a valid identifier.
    void destroy(entity const &entity);

    /// @brief Clear the registry.
    /// Destroys all the entities in the registry.
    void clear();

    /// @brief Get the number of entities in a registry.
    std::size_t size() const;

    /// @brief Get the number of components in an entity.
    /// @throws If the entity is not a valid identifier.
    std::size_t size(entity const &entity) const;

    /// @brief Check if an entity has no components.
    /// @throws If the entity is not a valid identifier.
    /// @return True if the entity is empty, false otherwise.
    bool empty(entity const &entity) const;

    /// @brief Returns a view for the given elements.
    /// @tparam Include Types of included elements used to construct the view.
    /// @tparam Exclude Types of elements used to filter the view.
    /// @param toExclude The type list used to deduce Exclude variadic template argument.
    /// @return A view on entities that are valid at the time of calling this member function and contain given included components and do not contain excluded ones at the time of calling this member function.
    template <typename... Include, typename... Exclude>
    ecs::view<Include..., Exclude...> view(exclude<Exclude...> toExclude = exclude<>{ }) const;

    /// @brief Iterate all the valid entities in the registry.
    iterator begin() const;
    /// @copydoc begin
    iterator end() const;
};
} // namespace ecs

/*! \cond Doxygen_Suppress */

template <typename T>
inline ecs::component_id ecs::get_component_id() noexcept {
    static const component_id id = impl::nextComponentId.fetch_add(1);
    return id;
}
inline ecs::component_id ecs::combine_component_ids(component_id const &lhs, component_id const &rhs) noexcept {
    return lhs ^ rhs + 0x9e3779b9 + (lhs << 6) + (lhs >> 2);
}
template <typename T>
ecs::component_id ecs::get_component_id(std::string_view name) noexcept {
    combine_component_ids(get_component_id<T>(), std::hash<std::string_view>{ }(name));
}

inline ecs::impl::EntityManager::EntityManager() {
    ECS_PROFILE;
    mAvailableEntities.reserve(1000);
}
inline ecs::entity ecs::impl::EntityManager::emplace() {
    ECS_PROFILE;
    entity entity = 0;
    if(mAvailableEntities.empty()) {
        entity = mNextID++;
    } else {
        entity = mAvailableEntities.back();
        mAvailableEntities.pop_back();
    }
    mEntities.emplace(entity, 0);

    return entity;
}
inline void ecs::impl::EntityManager::erase(entity const &entity) {
    ECS_PROFILE;
    ECS_ASSERT(contains(entity), "Invalid entity identifier");
    mAvailableEntities.push_back(entity);
    mEntities.erase(entity);
}
inline bool ecs::impl::EntityManager::contains(entity const &entity) const {
    ECS_PROFILE;
    return 1 <= entity && entity < mNextID && mEntities.contains(entity);
}
inline void ecs::impl::EntityManager::clear() {
    mEntities.clear();
    mAvailableEntities.clear();
    mNextID = 1;
}
inline std::size_t ecs::impl::EntityManager::size() const {
    return mEntities.size();
}
inline std::size_t ecs::impl::EntityManager::size(entity const &entity) const {
    ECS_ASSERT(contains(entity), "Invalid entity!");
    return mEntities.get(entity);
}

inline void ecs::impl::EntityManager::incSize(entity const &entity) {
    ECS_ASSERT(contains(entity), "Invalid entity!");
    ++mEntities.get(entity);
}
inline void ecs::impl::EntityManager::decSize(entity const &entity) {
    ECS_ASSERT(contains(entity), "Invalid entity!");
    auto &entitySize = mEntities.get(entity);
    ECS_ASSERT(entitySize > 0, "Cannot decrement entity size!");
    --entitySize;
}
inline ecs::impl::EntityManager::iterator ecs::impl::EntityManager::begin() const {
    return mEntities.sparse().begin();
}
inline ecs::impl::EntityManager::iterator ecs::impl::EntityManager::end() const {
    return mEntities.sparse().end();
}

template <typename component_t>
ecs::impl::ComponentArray<component_t>::ComponentArray():
    sparse_set<component_t>((CAPACITY + sizeof(component_t) - 1) / sizeof(component_t), PAGE_SIZE) {
}
template <typename component_t>
inline void ecs::impl::ComponentArray<component_t>::erase(entity const &entity) {
    ECS_PROFILE;
    if(sparse_set<component_t>::contains(entity))
        sparse_set<component_t>::erase(entity);
}
template <typename component_t>
inline bool ecs::impl::ComponentArray<component_t>::contains(entity const &entity) const {
    return sparse_set<component_t>::contains(entity);
}
template <typename component_t>
inline void ecs::impl::ComponentArray<component_t>::clear() {
    sparse_set<component_t>::clear();
}
template <typename component_t>
inline std::unique_ptr<ecs::impl::IComponentArray> ecs::impl::ComponentArray<component_t>::clone() const {
    ECS_PROFILE;
    return std::unique_ptr<ecs::impl::IComponentArray>{ new ecs::impl::ComponentArray<component_t>{ *this } };
}

template <typename component_t>
inline void ecs::impl::ComponentManager::registerComponent(std::unique_ptr<ecs::impl::IComponentArray> &&array) {
    ECS_PROFILE;
    auto id = get_component_id<component_t>();
    if(!mComponentArrays.contains(id))
        mComponentArrays.emplace(id, array ? std::move(array) : std::make_unique<impl::ComponentArray<component_t>>());
}
template <typename component_t>
inline ecs::impl::ComponentArray<component_t> *ecs::impl::ComponentManager::getComponentArray() {
    ECS_PROFILE;
    auto id = get_component_id<component_t>();
    ECS_ASSERT(mComponentArrays.contains(id), "Component not registered before use");
    if(!mComponentArrays.contains(id))
        return nullptr;
    return static_cast<impl::ComponentArray<component_t> *>(mComponentArrays.get(id).get());
}
template <typename component_t>
inline ecs::impl::ComponentArray<component_t> const *ecs::impl::ComponentManager::getComponentArray() const {
    ECS_PROFILE;
    auto id = get_component_id<component_t>();
    ECS_ASSERT(mComponentArrays.contains(id), "Component not registered before use");
    if(!mComponentArrays.contains(id))
        return nullptr;
    return static_cast<impl::ComponentArray<component_t> const *>(mComponentArrays.get(id).get());
}
inline ecs::impl::ComponentManager::ComponentManager(impl::ComponentManager const &other) {
    this->operator=(other);
}
inline ecs::impl::ComponentManager &ecs::impl::ComponentManager::operator=(impl::ComponentManager const &other) {
    ECS_PROFILE;
    for(auto [id, ptr] : other.mComponentArrays) {
        mComponentArrays.emplace(id, ptr->clone());
    }

    return *this;
}
inline void ecs::impl::ComponentManager::erase(entity const &entity) const {
    ECS_PROFILE;
    for(auto &componentArray : mComponentArrays.dense()) {
        componentArray->erase(entity);
    }
}
inline ecs::sparse_set<std::unique_ptr<ecs::impl::IComponentArray>> &ecs::impl::ComponentManager::getComponentArrays() {
    return mComponentArrays;
}
inline ecs::sparse_set<std::unique_ptr<ecs::impl::IComponentArray>> const &ecs::impl::ComponentManager::getComponentArrays() const {
    return mComponentArrays;
}

template <typename component_t>
inline bool ecs::registry::contains(entity const &entity) const {
    ECS_PROFILE;
    ECS_ASSERT(valid(entity), "Invalid entity identifier");

    mComponentManager.registerComponent<component_t>();
    return mComponentManager.getComponentArray<component_t>()->contains(entity);
}
template <typename component_t>
inline component_t &ecs::registry::get(entity const &entity) {
    ECS_PROFILE;
    ECS_ASSERT(valid(entity), "Invalid entity identifier");
    ECS_ASSERT(contains<component_t>(entity), "Component to get is not added");

    return mComponentManager.getComponentArray<component_t>()->get(entity);
}
template <typename component_t>
inline component_t const &ecs::registry::get(entity const &entity) const {
    ECS_PROFILE;
    ECS_ASSERT(valid(entity), "Invalid entity identifier");
    ECS_ASSERT(contains<component_t>(entity), "Component to get is not added");

    return mComponentManager.getComponentArray<component_t>()->get(entity);
}
template <typename... Components_t>
inline ecs::entity ecs::registry::create() {
    ECS_PROFILE;

    (mComponentManager.registerComponent<Components_t>(), ...);
    entity entity = mEntityManager.emplace();

    (emplace<Components_t>(entity), ...);

    return entity;
}
template <typename... Components_t>
inline ecs::entity ecs::registry::create(Components_t &&...components) {
    ECS_PROFILE;

    (mComponentManager.registerComponent<std::decay_t<Components_t>>(), ...);
    entity entity = mEntityManager.emplace();

    (emplace<std::decay_t<Components_t>>(entity, std::forward<Components_t>(components)), ...);

    return entity;
}
template <typename component_t>
inline void ecs::registry::erase(entity const &entity) {
    ECS_PROFILE;
    ECS_ASSERT(valid(entity), "Invalid entity identifier");
    ECS_ASSERT(contains<component_t>(entity), "Component to remove is not added");

    mComponentManager.getComponentArray<component_t>()->erase(entity);
    mEntityManager.decSize(entity);
}
template <typename component_t, class... Args>
inline void ecs::registry::emplace(entity const &entity, Args &&...args) {
    ECS_PROFILE;
    ECS_ASSERT(valid(entity), "Invalid entity identifier");
    ECS_ASSERT(!contains<component_t>(entity), "Component to emplace already added");

    mComponentManager.getComponentArray<component_t>()->emplace(entity, std::forward<Args>(args)...);
    mEntityManager.incSize(entity);
}
inline ecs::registry::registry(registry const &other) {
    *this = other;
}
inline ecs::registry::registry(registry &&other) noexcept {
    *this = std::move(other);
}
inline ecs::registry &ecs::registry::operator=(registry const &other) {
    ECS_PROFILE;
    mEntityManager = other.mEntityManager;
    mComponentManager = other.mComponentManager;
    return *this;
}
inline ecs::registry &ecs::registry::operator=(registry &&other) noexcept {
    ECS_PROFILE;
    std::swap(mEntityManager, other.mEntityManager);
    std::swap(mComponentManager, other.mComponentManager);
    return *this;
}
inline bool ecs::registry::valid(entity const &entity) const {
    return mEntityManager.contains(entity);
}
inline void ecs::registry::destroy(ecs::entity const &entity) {
    ECS_PROFILE;
    ECS_ASSERT(valid(entity), "Invalid entity identifier");

    mEntityManager.erase(entity);
    mComponentManager.erase(entity);
}
inline bool ecs::registry::empty(entity const &entity) const {
    return size(entity) == 0;
}
inline void ecs::registry::clear() {
    ECS_PROFILE;
    mEntityManager.clear();
    for(std::size_t i = mComponentManager.getComponentArrays().size(); i; --i) {
        mComponentManager.getComponentArrays().begin()[i-1u].second->clear();
    }
}
inline std::size_t ecs::registry::size(entity const &entity) const {
    ECS_PROFILE;
    ECS_ASSERT(valid(entity), "Invalid entity identifier");
    return mEntityManager.size(entity);
}
inline std::size_t ecs::registry::size() const {
    ECS_PROFILE;
    return mEntityManager.size();
}
template <typename... Include, typename... Exclude>
inline ecs::view<Include..., Exclude...> ecs::registry::view(exclude<Exclude...>) const {
    ECS_PROFILE;
    return ecs::view(std::tuple<Include...>(mComponentManager.getComponentArray<Include>()...), std::tuple<Exclude...>(mComponentManager.getComponentArray<Exclude>()...));
}
inline ecs::registry::iterator ecs::registry::begin() const {
    return mEntityManager.begin();
}
inline ecs::registry::iterator ecs::registry::end() const {
    return mEntityManager.end();
}

/*! \endcond */
