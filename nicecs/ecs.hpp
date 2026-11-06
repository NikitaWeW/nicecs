/*
      ___  ___ ___ 
     / _ \/ __/ __|        Copyright (c) 2024 Nikita Martynau 
    |  __/ (__\__ \        https://opensource.org/license/mit 
     \___|\___|___/ v1.6.0 https://github.com/nikitawew/nicecs

No generative AI was used.

Thanks to: 
https://austinmorlan.com/posts/entity_component_system.
https://github.com/skypjack/entt.
Https://github.com/SanderMertens/ecs-faq

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>
#include <array>

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
/// @brief Entity version.
using version_t = std::uint32_t;

/// @brief Get an id for each component type.
template <typename T>
component_id get_component_id() noexcept;

/// @brief Syntax sugar for easy string hashing (for storage id's).
component_id operator""_hs(const char *str, ecs::size_t len);

/// @brief Exclusion type list.
template <typename... T>
using exclude = std::tuple<T...>;

class view_iterable {
public:
    using iterator = std::remove_reference_t<decltype(sparse_set<int>{ }.sparse())>::const_iterator;
    virtual bool contains(entity const &entity) const = 0;
    virtual ecs::size_t size() const = 0;
    virtual iterator begin() const = 0;
    virtual iterator end() const = 0;
};

class view_sentinel { };

/// @brief Lightweight view on the component pools.
/// Allows to efficiently iterate over entities containing required components, filtering excluded components.
/// Is constructed using static arrays of abstract component pools (practically free).
/// Provides begin() and end() to iterate over each entity.
/// When iterating, the view:
/// 1. Finds the smallest pool.
/// 2. Loops over every entity in the smallest pool.
/// 3. Performs O(1) contains check on every other pool.
/// This way there are no memory allocations and practically no overhead.
template <ecs::size_t IncludeSize, ecs::size_t ExcludeSize = 0>
class view {
    static_assert(IncludeSize != 0u);
private:
    // Last element is the smallest one
    std::array<view_iterable const *, IncludeSize> mInclude;
    std::array<view_iterable const *, ExcludeSize> mExclude;
public:
    /// @brief View iterator.
    class iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = entity;
        using difference_type = std::ptrdiff_t;
        using pointer = void;
        using reference = value_type;
    private:
        view_iterable const * const *mInclude;
        view_iterable const * const *mExclude;
        ecs::size_t mIt = 0;
        // In case entity is deleted from the pool
        reference mValue;
        view_iterable const *mSmallest;

        template <ecs::size_t... IncludeSeq, ecs::size_t... ExcludeSeq>
        bool valid(entity const &e, std::integer_sequence<ecs::size_t, IncludeSeq...>, std::integer_sequence<ecs::size_t, ExcludeSeq...>) const;
        void seek_next();
    public:
        iterator(ecs::size_t it, view_iterable const * const *include, view_iterable const * const *exclude);
        iterator &operator++() noexcept;
        iterator operator++(int) noexcept;

        reference operator*() const noexcept;

        constexpr bool operator==(iterator const &other) const noexcept;
        constexpr bool operator!=(iterator const &other) const noexcept;

        bool operator==(view_sentinel const &) const noexcept;
        bool operator!=(view_sentinel const &) const noexcept;
    };

    constexpr view(std::array<view_iterable const *, IncludeSize> &&mInclude,
        std::array<view_iterable const *, ExcludeSize> &&mExclude = { }) noexcept;

    iterator begin() const noexcept;
    iterator endIt() const noexcept;
    view_sentinel end() const noexcept;

    /// @brief Returns the size of the smallest pool.
    /// @warning May not represent the number of satisfying entities in the view!
    ecs::size_t size() const noexcept;
};

template <typename It>
bool operator==(view_sentinel const &s, It const &it) noexcept;
template <typename It>
bool operator!=(view_sentinel const &s, It const &it) noexcept;

template <typename T>
class component_array;

/// @brief Helper function to make views.
/// Takes types, casts them to view_iterable const * and makes a view
/// @example ecs::make_view(std::make_tuple(&reg.storage<Position>("name"_hs)), std::make_tuple(&reg.storage<Velocity>("name"_hs)))
template <typename... Include, typename... Exclude>
constexpr inline view<sizeof...(Include), sizeof...(Exclude)> make_view(std::tuple<Include...> include, std::tuple<Exclude...> exclude = { }) {
    // clang-format off
    return std::apply([&](auto const *... inc){
        return std::apply([&](auto const *... exc){
            return view(std::array<view_iterable const *, sizeof...(Include)>{inc...}, 
                        std::array<view_iterable const *, sizeof...(Exclude)>{exc...});
        }, exclude);
    }, include);
    // clang-format on
}

class component_storage : public view_iterable {
public:
    /// @brief Returns a pointer to the component data.
    virtual void *data(entity const &entity) = 0;

    /// @brief Returns a pointer to the component data.
    virtual void const *data(entity const &entity) const = 0;

    /// @brief Returns the size of stored component type in bytes.
    virtual ecs::size_t component_size() const = 0;

    /// @brief Erase an entity's component.
    /// @param entity An entity identifier.
    virtual void erase(entity const &entity) = 0;

    /// @brief Get the result of get_component_id<T>()
    virtual component_id id() const = 0;
};

namespace impl {

    /// @brief Manages entities duh.
    class entity_manager {
    private:
        std::vector<bool> mAliveEntities;
        std::vector<version_t> mVersions;
        std::vector<entity> mAvailableEntities;
        entity mNextID = 1;
        ecs::size_t mSize = 0;
    public:
        using iterator = std::remove_reference_t<decltype(mAliveEntities)>::const_iterator;
        explicit entity_manager();
        ~entity_manager() = default;

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
        ecs::size_t size() const;

        /// @brief Get the entity version (the number of times this id is used in emplace())
        version_t version(ecs::entity const &entity) const;

        /// @brief
        iterator begin() const;
        iterator end() const;
    };

    inline std::atomic<component_id> nextComponentId{ 0 };

    /// @brief Type erased sparse set.
    /// Is guaranteed to be a parent of ComponentArray
    class component_pool : public component_storage {
    public:
        component_pool() = default;
        virtual ~component_pool() = default;

        /// @brief Erases all elements.
        virtual void clear() = 0;

        /// @brief Create and returns a copy of the component array if the elements can be copied, nullptr otherwise.
        /// @warning Delete the pointer manually.
        virtual component_pool *clone() const = 0;
    };

    /// @brief idk anymore...
    template <typename storage_t>
    class iterable {
    private:
        storage_t *mStorage;
    public:
        inline iterable(storage_t *storage): mStorage(storage) {
        }

        auto begin() {
            return mStorage->begin();
        }
        auto end() {
            return mStorage->end();
        }
        auto begin() const {
            return mStorage->begin();
        }
        auto end() const {
            return mStorage->end();
        }
    };

    /// @brief Manages components and their arrays. All components are destroyed automatically.
    class component_manager {
    public:
        // All elements can be converted to component_pool's, but are stored like this for ease of iteration.
        using storage_type = std::unordered_map<component_id, component_storage *>;
    private:
        storage_type mComponentArrays;
    public:
        component_manager() = default;
        component_manager(component_manager const &other);
        component_manager &operator=(component_manager const &other);
        component_manager(component_manager &&other) = default;
        component_manager &operator=(component_manager &&other) = default;
        ~component_manager();

        /// @brief Creates a new component pool.
        /// @tparam T The type of the component.
        /// @param id The pool id.
        template <typename T>
        component_array<T> *emplace(component_id id);

        /// @brief If the pool does not exist, emplaces it, returns the existing pool otherwise.
        template <typename T>
        component_array<T> *try_emplace(component_id id);

        /// @brief Duh
        bool contains(component_id id) const;

        /// @brief Notify component arrays that the entity is destroyed.
        /// @param entity A deleted entity identifier.
        void erase(entity const &entity) const;

        template <typename T>
        component_array<T> *get(component_id id);
        template <typename T>
        component_array<T> const *get(component_id id) const;

        component_pool const *find(component_id id) const;
        component_pool *find(component_id id);

        storage_type &storage();
        storage_type const &storage() const;
    };
}; // namespace impl

/// @brief Stores components of entities of a specific type as a sparse set.
/// @tparam T The type of stored components.
template <typename T>
class component_array : public impl::component_pool /* , public component_storage, public view_iterable */, public sparse_set<T> {
public:
    /// @brief The size of a component sparse page.
    static constexpr ecs::size_t PAGE_SIZE = 32;
    /// @brief The starting capacity to reserve in bytes.
    static constexpr ecs::size_t CAPACITY = 256;

    explicit component_array();

    /// @copydoc ecs::component_storage::data
    void *data(entity const &entity) override;
    /// @copydoc ecs::component_storage::data
    void const *data(entity const &entity) const override;

    /// @copydoc ecs::component_storage::component_size
    ecs::size_t component_size() const override;

    /// @copydoc ecs::component_storage::erase
    void erase(entity const &entity) override;

    /// @copydoc ecs::view_iterareble::contains
    bool contains(entity const &entity) const override;

    /// @copydoc ecs::view_iterareble::size
    ecs::size_t size() const override;

    /// @copydoc ecs::component_storage::id
    component_id id() const override;

    /// @copydoc ecs::impl::component_pool::clear
    void clear() override;

    view_iterable::iterator begin() const noexcept override;
    view_iterable::iterator end() const noexcept override;

    /// @copydoc ecs::impl::component_pool::clone
    component_pool *clone() const override;
};

/// @brief An ECS interface.
/// Contains entities and their components.
class registry {
private:
    impl::entity_manager mEntityManager;
    // ugly fix for lazy component registration.
    mutable impl::component_manager mComponentManager;
public:
    using iterator = impl::entity_manager::iterator;

    registry() = default;
    ~registry() = default;
    registry(registry const &other);
    registry(registry &&other) noexcept;
    registry &operator=(registry const &other);
    registry &operator=(registry &&other) noexcept;

    /// @brief Get the component storage.
    /// @param id Arbitrary id or a string hash (not combined with the type id). Must be unique.
    template <typename T>
    component_array<T> const &storage(component_id id = get_component_id<T>()) const;
    /// @copydoc storage
    template <typename T>
    component_array<T> &storage(component_id id = get_component_id<T>());

    /// @brief Find the component storage. Returns nullptr if not found.
    component_storage *storage(component_id id);
    /// @copydoc storage
    component_storage const *storage(component_id id) const;

    /// @brief Get all component pools.
    impl::iterable<ecs::impl::component_manager::storage_type const> storage() const;
    /// @copydoc storage
    impl::iterable<ecs::impl::component_manager::storage_type> storage();

    /// @brief Create an entity.
    /// @return Unique valid entity id.
    /// @throws If the same component is added more than once.
    entity create();

    /// @copydoc impl::EntityManager::valid
    bool valid(entity const &entity) const;

    /// @brief Destroys an entity and its components.
    /// @param entity A valid entity identifier.
    /// @throws If the entity is not a valid identifier.
    void destroy(entity const &entity);

    /// @brief Clear the registry.
    /// Destroys all the entities in the registry.
    void clear();

    /// @brief Get the number of entities in a registry.
    ecs::size_t size() const;
    /// @brief Check if the registry has no entities.
    bool empty() const;

    /// @brief Get the number of components in an entity.
    /// @throws If the entity is not a valid identifier.
    /// @warning O(n) time (checks every pool).
    /// Keeping track of the entity's size in the entity manager is possible but would be unsafe,
    /// because the storage could be accessed without notifying the manager.
    ecs::size_t size(entity const &entity) const;

    /// @copydoc impl::entity_manager::version
    version_t version(entity const &entity) const;

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
    ecs::view<sizeof...(Include), sizeof...(Exclude)> view(exclude<Exclude...> toExclude = exclude<>{ }) const;

    /// @brief Iterate all the valid entities in the registry.
    iterator begin() const;
    /// @copydoc begin
    iterator end() const;

    // Syntax sugar below

    /// @brief Create an entity.
    /// @param components The components to move in.
    /// @return Unique valid entity id.
    /// @throws If the same component is added more than once.
    template <typename... Components_t>
    entity create(Components_t &&...components);

    template <typename... Components_t>
    entity create();

    /// @brief Checks if a valid entity has a component.
    /// @param entity A valid entity identifier.
    /// @tparam T The component type.
    /// @throws If the entity is not a valid identifier.
    /// @return True if the entity has the component, false otherwise.
    template <typename T>
    bool contains(entity const &entity) const;

    /// @brief Gets a component from a a valid entity.
    /// @param entity A valid entity identifier.
    /// @tparam T The component type.
    /// @throws If the entity is not a valid identifier.
    /// @throws If the component is not added.
    /// @return The component lvalue reference.
    template <typename T>
    T &get(entity const &entity);
    /// @copydoc get
    template <typename T>
    T const &get(entity const &entity) const;

    /// @brief Removes a component from a valid entity.
    /// @param entity A valid entity identifier.
    /// @throws If the entity is not a valid identifier.
    /// @throws If the component is not added.
    /// @tparam T The component type.
    template <typename T>
    void erase(entity const &entity);

    /// @copydoc impl::component_manager::emplace
    /// @throws If the entity is not a valid identifier.
    /// @throws If the component is already added.
    template <typename T, class... Args>
    T &emplace(entity const &entity, Args &&...args);
};

} // namespace ecs

/*! \cond Doxygen_Suppress */

template <typename T>
inline ecs::component_id ecs::get_component_id() noexcept {
    static const component_id id = impl::nextComponentId.fetch_add(1);
    return id;
}
inline ecs::component_id ecs::operator""_hs(const char *str, ecs::size_t len) {
    ECS_PROFILE;
    auto res = std::hash<std::string_view>{ }(std::string_view(str, len));
    if(res == 0) {
        return std::numeric_limits<component_id>::max();
    }
    return res;
}

inline ecs::impl::entity_manager::entity_manager() {
    ECS_PROFILE;
    mAvailableEntities.reserve(1000);
}
inline ecs::entity ecs::impl::entity_manager::emplace() {
    ECS_PROFILE;
    entity entity = 0;
    if(mAvailableEntities.empty()) {
        entity = mNextID++;
    } else {
        entity = mAvailableEntities.back();
        mAvailableEntities.pop_back();
    }
    if(entity > mAliveEntities.size()) {
        mAliveEntities.resize(std::max<ecs::size_t>(mAliveEntities.size() * 2, entity), 0);
        mVersions.resize(std::max<ecs::size_t>(mAliveEntities.size() * 2, entity), 0);
    }
    mAliveEntities[entity - 1u] = 1;
    ++mVersions[entity - 1u];
    ++mSize;

    return entity;
}
inline void ecs::impl::entity_manager::erase(entity const &entity) {
    ECS_PROFILE;
    ECS_ASSERT(contains(entity), "Invalid entity identifier");
    mAvailableEntities.push_back(entity);
    mAliveEntities[entity - 1u] = 0;
    --mSize;
}
inline bool ecs::impl::entity_manager::contains(entity const &entity) const {
    ECS_PROFILE;
    return 1 <= entity && entity < mNextID && (entity <= mAliveEntities.size() && mAliveEntities[entity - 1u] != 0);
}
inline void ecs::impl::entity_manager::clear() {
    ECS_PROFILE;
    mAliveEntities.clear();
    mVersions.clear();
    mAvailableEntities.clear();
    mNextID = 1;
    mSize = 0;
}
inline ecs::size_t ecs::impl::entity_manager::size() const {
    return mSize;
}
inline ecs::version_t ecs::impl::entity_manager::version(entity const &entity) const {
    ECS_ASSERT(contains(entity), "Invalid entity identifier");
    return mVersions[entity - 1u];
}
inline ecs::impl::entity_manager::iterator ecs::impl::entity_manager::begin() const {
    return mAliveEntities.begin();
}
inline ecs::impl::entity_manager::iterator ecs::impl::entity_manager::end() const {
    return mAliveEntities.end();
}

template <typename T>
inline ecs::component_array<T>::component_array(): sparse_set<T>((CAPACITY + sizeof(T) - 1) / sizeof(T), PAGE_SIZE) {
}
template <typename T>
void *ecs::component_array<T>::data(entity const &entity) {
    return &sparse_set<T>::get(entity);
}
template <typename T>
void const *ecs::component_array<T>::data(entity const &entity) const {
    return &sparse_set<T>::get(entity);
}
template <typename T>
ecs::size_t ecs::component_array<T>::component_size() const {
    return sizeof(T);
}
template <typename T>
inline void ecs::component_array<T>::erase(entity const &entity) {
    ECS_PROFILE;
    if(sparse_set<T>::contains(entity))
        sparse_set<T>::erase(entity);
}
template <typename T>
inline bool ecs::component_array<T>::contains(entity const &entity) const {
    ECS_PROFILE;
    return sparse_set<T>::contains(entity);
}
template <typename T>
inline ecs::size_t ecs::component_array<T>::size() const {
    ECS_PROFILE;
    return sparse_set<T>::size();
}
template <typename T>
inline ecs::component_id ecs::component_array<T>::id() const {
    return get_component_id<T>();
}
template <typename T>
inline ecs::view_iterable::iterator ecs::component_array<T>::begin() const noexcept {
    ECS_PROFILE;
    return sparse_set<T>::sparse().begin();
}
template <typename T>
inline ecs::view_iterable::iterator ecs::component_array<T>::end() const noexcept {
    ECS_PROFILE;
    return sparse_set<T>::sparse().end();
}
template <typename T>
inline void ecs::component_array<T>::clear() {
    ECS_PROFILE;
    sparse_set<T>::clear();
}
template <typename T>
inline ecs::impl::component_pool *ecs::component_array<T>::clone() const {
    ECS_PROFILE;
    if constexpr(std::is_copy_constructible_v<T>) {
        return new ecs::component_array<T>{ *this };
    }
    return nullptr;
}

template <typename T>
inline ecs::component_array<T> *ecs::impl::component_manager::emplace(component_id id) {
    ECS_PROFILE;
    ECS_ASSERT(mComponentArrays.find(id) == mComponentArrays.end(), "Component pool already exists!");
    return static_cast<ecs::component_array<T> *>(mComponentArrays.emplace(id, new component_array<T>()).first->second);
}
template <typename T>
inline ecs::component_array<T> *ecs::impl::component_manager::try_emplace(component_id id) {
    ECS_PROFILE;
    if(mComponentArrays.find(id) == mComponentArrays.end()) {
        return emplace<T>(id);
    }
    auto array = get<T>(id);
    ECS_ASSERT(array->id() == get_component_id<T>(), "Cannot use same id for different types!");
    return array;
}
template <typename T>
inline ecs::component_array<T> *ecs::impl::component_manager::get(component_id id) {
    ECS_PROFILE;
    if(mComponentArrays.find(id) != mComponentArrays.end()) {
        auto array = static_cast<component_array<T> *>(mComponentArrays.at(id));
        ECS_ASSERT(array->id() == get_component_id<T>(), "Incorrect id!");
        return array;
    }
    return nullptr;
}
template <typename T>
inline ecs::component_array<T> const *ecs::impl::component_manager::get(component_id id) const {
    ECS_PROFILE;
    if(mComponentArrays.find(id) != mComponentArrays.end()) {
        auto array = static_cast<component_array<T> const *>(mComponentArrays.at(id));
        ECS_ASSERT(array->id() == get_component_id<T>(), "Incorrect id!");
        return array;
    }
    return nullptr;
}
inline ecs::impl::component_pool const *ecs::impl::component_manager::find(component_id id) const {
    ECS_PROFILE;
    if(mComponentArrays.find(id) != mComponentArrays.end()) {
        return static_cast<ecs::impl::component_pool const *>(mComponentArrays.at(id));
    }
    return nullptr;
}
inline ecs::impl::component_pool *ecs::impl::component_manager::find(component_id id) {
    ECS_PROFILE;
    if(mComponentArrays.find(id) != mComponentArrays.end()) {
        return static_cast<ecs::impl::component_pool *>(mComponentArrays.at(id));
    }
    return nullptr;
}
inline ecs::impl::component_manager::component_manager(impl::component_manager const &other) {
    this->operator=(other);
}
inline ecs::impl::component_manager &ecs::impl::component_manager::operator=(impl::component_manager const &other) {
    ECS_PROFILE;
    for(auto &[id, array] : other.mComponentArrays) {
        if(component_pool *copy = static_cast<component_pool *>(array)->clone()) {
            mComponentArrays.emplace(id, copy);
        }
    }

    return *this;
}
inline ecs::impl::component_manager::~component_manager() {
    for(auto &[id, array] : mComponentArrays) {
        delete static_cast<component_pool *>(array);
    }
}
inline void ecs::impl::component_manager::erase(entity const &entity) const {
    ECS_PROFILE;
    for(auto &[id, array] : mComponentArrays) {
        array->erase(entity);
    }
}
inline ecs::impl::component_manager::storage_type &ecs::impl::component_manager::storage() {
    return mComponentArrays;
}
inline ecs::impl::component_manager::storage_type const &ecs::impl::component_manager::storage() const {
    return mComponentArrays;
}

template <typename T>
inline ecs::component_array<T> const &ecs::registry::storage(component_id id) const {
    return *mComponentManager.try_emplace<T>(id);
}
template <typename T>
inline ecs::component_array<T> &ecs::registry::storage(ecs::component_id id) {
    return *mComponentManager.try_emplace<T>(id);
}
inline ecs::component_storage *ecs::registry::storage(component_id id) {
    return mComponentManager.find(id);
}
inline ecs::component_storage const *ecs::registry::storage(component_id id) const {
    return mComponentManager.find(id);
}
inline ecs::impl::iterable<ecs::impl::component_manager::storage_type const> ecs::registry::storage() const {
    return &mComponentManager.storage();
}
inline ecs::impl::iterable<ecs::impl::component_manager::storage_type> ecs::registry::storage() {
    return &mComponentManager.storage();
}

inline ecs::entity ecs::registry::create() {
    ECS_PROFILE;
    return mEntityManager.emplace();
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
    for(auto &[id, array] : mComponentManager.storage()) {
        static_cast<impl::component_pool *>(array)->clear();
    }
}
inline ecs::size_t ecs::registry::size(entity const &entity) const {
    ECS_PROFILE;
    ECS_ASSERT(valid(entity), "Invalid entity identifier");

    ecs::size_t size = 0;
    for(auto const &pool : mComponentManager.storage()) {
        if(pool.second->contains(entity)) {
            ++size;
        }
    }
    return size;
}
inline ecs::version_t ecs::registry::version(entity const &entity) const {
    return mEntityManager.version(entity);
}
inline ecs::size_t ecs::registry::size() const {
    ECS_PROFILE;
    return mEntityManager.size();
}
inline bool ecs::registry::empty() const {
    return size() == 0;
}
inline ecs::registry::iterator ecs::registry::begin() const {
    return mEntityManager.begin();
}
inline ecs::registry::iterator ecs::registry::end() const {
    return mEntityManager.end();
}

template <ecs::size_t Include, ecs::size_t Exclude>
inline constexpr ecs::view<Include, Exclude>::view(std::array<view_iterable const *, Include> &&include,
    std::array<view_iterable const *, Exclude> &&exclude) noexcept: mInclude(std::move(include)), mExclude(std::move(exclude)) {
    ECS_PROFILE;
    if constexpr(Include > 1u) {
        std::sort(mInclude.begin(), mInclude.end(), [](auto const &lhs, auto const &rhs) { return lhs->size() < rhs->size(); });
    }
    if constexpr(Exclude > 1u) {
        std::sort(mExclude.begin(), mExclude.end(), [](auto const &lhs, auto const &rhs) { return lhs->size() > rhs->size(); });
    }
}
template <ecs::size_t Include, ecs::size_t Exclude>
inline ecs::view<Include, Exclude>::iterator::iterator(ecs::size_t it, view_iterable const * const *include, view_iterable const * const *exclude):
    mInclude(include), mExclude(exclude), mIt(it) {
    mSmallest = mInclude[0];
    seek_next();
}
template <ecs::size_t Include, ecs::size_t Exclude>
template <ecs::size_t... IncludeSeq, ecs::size_t... ExcludeSeq>
inline bool ecs::view<Include, Exclude>::iterator::valid(entity const &e, std::integer_sequence<ecs::size_t, IncludeSeq...>,
    std::integer_sequence<ecs::size_t, ExcludeSeq...>) const {
    ECS_PROFILE;
    return (!mExclude[ExcludeSeq]->contains(e) && ...) && (mInclude[IncludeSeq + 1u]->contains(e) && ...);
}
template <ecs::size_t Include, ecs::size_t Exclude>
inline void ecs::view<Include, Exclude>::iterator::seek_next() {
    ECS_PROFILE;
    while(mIt != mSmallest->size()) {
        mValue = mSmallest->begin()[mIt];

        if(valid(mValue, std::make_integer_sequence<ecs::size_t, Include - 1u>{ }, std::make_integer_sequence<ecs::size_t, Exclude>{ })) {
            return;
        }

        ++mIt;
    }
}
template <ecs::size_t Include, ecs::size_t Exclude>
inline typename ecs::view<Include, Exclude>::iterator &ecs::view<Include, Exclude>::iterator::operator++() noexcept {
    ECS_PROFILE;
    // If the entity at the iterator changed, the previous entity was removed.
    // Storage performed swap and pop and the current iterator contains new valid entity.
    // Do not increment in this case.
    // Ugly fix for range based for loops. The iterator is incremented before the bounds check.
    // ________________________
    if(mIt != mSmallest->size() && mValue == mSmallest->begin()[mIt]) {
        ++mIt;
    }
    seek_next();
    return *this;
}
template <ecs::size_t Include, ecs::size_t Exclude>
inline typename ecs::view<Include, Exclude>::iterator ecs::view<Include, Exclude>::iterator::operator++(int) noexcept {
    ECS_PROFILE;
    auto original = *this;
    this->operator++();
    return original;
}
template <ecs::size_t Include, ecs::size_t Exclude>
inline typename ecs::view<Include, Exclude>::iterator::reference ecs::view<Include, Exclude>::iterator::operator*() const noexcept {
    return mValue;
}
template <ecs::size_t Include, ecs::size_t Exclude>
inline constexpr bool ecs::view<Include, Exclude>::iterator::operator==(iterator const &other) const noexcept {
    return mIt == other.mIt;
}
template <ecs::size_t Include, ecs::size_t Exclude>
inline constexpr bool ecs::view<Include, Exclude>::iterator::operator!=(iterator const &other) const noexcept {
    return !operator==(other);
}
template <ecs::size_t Include, ecs::size_t Exclude>
inline bool ecs::view<Include, Exclude>::iterator::operator==(view_sentinel const &) const noexcept {
    return mIt >= mSmallest->size();
}
template <ecs::size_t Include, ecs::size_t Exclude>
inline bool ecs::view<Include, Exclude>::iterator::operator!=(view_sentinel const &o) const noexcept {
    return !operator==(o);
}
template <typename It>
inline bool ecs::operator==(view_sentinel const &s, It const &it) noexcept {
    return it == s;
}
template <typename It>
inline bool ecs::operator!=(view_sentinel const &s, It const &it) noexcept {
    return it != s;
}

template <ecs::size_t Include, ecs::size_t Exclude>
inline typename ecs::view<Include, Exclude>::iterator ecs::view<Include, Exclude>::begin() const noexcept {
    return iterator(0, mInclude.data(), mExclude.data());
}
template <ecs::size_t Include, ecs::size_t Exclude>
inline typename ecs::view<Include, Exclude>::iterator ecs::view<Include, Exclude>::endIt() const noexcept {
    return iterator(mInclude[0]->size(), mInclude.data(), mExclude.data());
}
template <ecs::size_t Include, ecs::size_t Exclude>
inline ecs::view_sentinel ecs::view<Include, Exclude>::end() const noexcept {
    return ecs::view_sentinel{ };
}
template <ecs::size_t Include, ecs::size_t Exclude>
inline ecs::size_t ecs::view<Include, Exclude>::size() const noexcept {
    return mInclude[0]->size();
}

// Syntax sugar

template <typename... Include, typename... Exclude>
inline ecs::view<sizeof...(Include), sizeof...(Exclude)> ecs::registry::view(exclude<Exclude...>) const {
    ECS_PROFILE;
    return ecs::make_view(std::tuple(&storage<Include>()...), std::tuple(&storage<Exclude>()...));
}
template <typename... Components_t>
inline ecs::entity ecs::registry::create(Components_t &&...components) {
    ECS_PROFILE;

    entity entity = create();

    (storage<std::decay_t<Components_t>>().emplace(entity, std::forward<Components_t>(components)), ...);

    return entity;
}
template <typename... Components_t>
inline ecs::entity ecs::registry::create() {
    return create(Components_t{ }...);
}
template <typename T>
inline bool ecs::registry::contains(entity const &entity) const {
    ECS_PROFILE;
    ECS_ASSERT(valid(entity), "Invalid entity identifier");

    return storage<T>().contains(entity);
}
template <typename T>
inline T &ecs::registry::get(entity const &entity) {
    ECS_PROFILE;
    ECS_ASSERT(valid(entity), "Invalid entity identifier");
    ECS_ASSERT(contains<T>(entity), "Component to get is not added");

    return storage<T>().get(entity);
}
template <typename T>
inline T const &ecs::registry::get(entity const &entity) const {
    ECS_PROFILE;
    ECS_ASSERT(valid(entity), "Invalid entity identifier");
    ECS_ASSERT(contains<T>(entity), "Component to get is not added");

    return storage<T>().get(entity);
}
template <typename T>
inline void ecs::registry::erase(entity const &entity) {
    ECS_PROFILE;
    ECS_ASSERT(valid(entity), "Invalid entity identifier");
    ECS_ASSERT(contains<T>(entity), "Component to remove is not added");

    storage<T>().erase(entity);
}
template <typename T, class... Args>
inline T &ecs::registry::emplace(entity const &entity, Args &&...args) {
    ECS_PROFILE;
    ECS_ASSERT(valid(entity), "Invalid entity identifier");
    ECS_ASSERT(!contains<T>(entity), "Component to emplace already added");

    return storage<T>().emplace(entity, std::forward<Args>(args)...);
}

/*! \endcond */
