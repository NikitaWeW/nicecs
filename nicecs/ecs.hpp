/*
      ___  ___ ___ 
     / _ \/ __/ __|        Copyright (c) 2024 Nikita Martynau 
    |  __/ (__\__ \        https://opensource.org/license/mit 
     \___|\___|___/ v1.6   https://github.com/nikitawew/nicecs

Thanks to: 
https://austinmorlan.com/posts/entity_component_system.
https://github.com/skypjack/entt.
Https://github.com/SanderMertens/ecs-faq
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
/// @brief Size type.
using size_t = std::uint64_t;

/// @brief Get an id for each component type.
template <typename T>
component_id get_component_id() noexcept;

component_id operator""_hs(const char *str, ecs::size_t len);

/// @brief Combine hashes.
template <typename T>
T combine_hash(T const &lhs, T const &rhs) noexcept;

/// @brief Get a combined id of component type and name hash.
/// Can be used for ecs::registry::storage<>().
template <typename T>
component_id get_component_id(std::string_view name) noexcept;

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
        view_iterable const **mInclude;
        view_iterable const **mExclude;
        view_iterable::iterator mIt;
        view_iterable::iterator mEnd;

        bool valid(entity const &e) const;
    public:
        iterator(view_iterable::iterator it, view_iterable::iterator end, view_iterable const **include, view_iterable const **exclude);
        iterator &operator++() noexcept;
        iterator operator++(int) const noexcept;

        reference operator*() const noexcept;

        constexpr bool operator==(iterator const &other) const noexcept;
        constexpr bool operator!=(iterator const &other) const noexcept;
    };

    constexpr view(std::array<view_iterable const *, IncludeSize> &&mInclude,
        std::array<view_iterable const *, ExcludeSize> &&mExclude = { }) noexcept;

    iterator begin() const noexcept;
    iterator end() const noexcept;
    ecs::size_t size() const noexcept;
};

template <typename T>
class component_array;

template <typename... Include, typename... Exclude>
constexpr inline view<sizeof...(Include), sizeof...(Exclude)> make_view(std::tuple<Include...> include, std::tuple<Exclude...> exclude = {}) {
    // clang-format off
    return std::apply([&](auto const *... inc){
        return std::apply([&](auto const *... exc){
            return view(std::array<view_iterable const *, sizeof...(Include)>{inc...}, 
                        std::array<view_iterable const *, sizeof...(Exclude)>{exc...});
        }, exclude);
    }, include);
    // clang-format on
}

namespace impl {

    /// @brief Manages entities duh.
    class entity_manager {
    private:
        sparse_set<ecs::size_t> mEntities;
        std::vector<entity> mAvailableEntities;
        entity mNextID = 1;
    public:
        using iterator = std::remove_reference_t<decltype(mEntities.sparse())>::const_iterator;
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

        /// @brief
        iterator begin() const;
        iterator end() const;
    };

    inline std::atomic<component_id> nextComponentId{ 0 };

    /// @brief Type erased sparse set.
    /// Is guaranteed to be a parent of ComponentArray
    class component_storage : public view_iterable {
    public:
        component_storage() = default;
        virtual ~component_storage() = default;

        /// @brief Erase an entity's component.
        /// @param entity An entity identifier.
        virtual void erase(entity const &entity) = 0;

        /// @brief Erases all elements.
        virtual void clear() = 0;

        /// @brief Create a copy of the component array.
        virtual std::unique_ptr<component_storage> clone() const = 0;
    };

    /// @brief Manages components and their arrays. All components are destroyed automatically.
    class component_manager {
    private:
        sparse_set<std::unique_ptr<component_storage>> mComponentArrays;
    public:
        component_manager() = default;
        component_manager(component_manager const &other);
        component_manager &operator=(component_manager const &other);
        ~component_manager() = default;

        /// @brief Creates a new component pool.
        /// @tparam T The type of the component.
        /// @param id The pool id.
        template <typename T>
        component_array<T> *emplace(component_id id = get_component_id<T>());

        /// @brief If the pool does not exist, emplaces it, returns the existing pool otherwise.
        template <typename T>
        component_array<T> *try_emplace(component_id id = get_component_id<T>());

        bool contains(component_id id = 0) const;

        /// @brief Notify component arrays that the entity is destroyed.
        /// @param entity A deleted entity identifier.
        void erase(entity const &entity) const;

        /// @brief Get the component array associated with the given component type.
        template <typename T>
        component_array<T> *get(component_id id = 0);
        /// @copydoc get
        template <typename T>
        component_array<T> const *get(component_id id = 0) const;

        sparse_set<std::unique_ptr<component_storage>> &storage();
        sparse_set<std::unique_ptr<component_storage>> const &storage() const;
    };
}; // namespace impl

/// @brief Stores components of entities of a specific type as a sparse set.
/// @tparam T The type of stored components.
template <typename T>
class component_array : public impl::component_storage, public sparse_set<T> {
public:
    /// @brief The size of a component sparse page.
    static constexpr ecs::size_t PAGE_SIZE = 32;
    /// @brief The starting capacity to reserve in bytes.
    static constexpr ecs::size_t CAPACITY = 256;

    explicit component_array();

    /// @copydoc ecs::impl::component_storage::erase
    void erase(entity const &entity) override;

    /// @copydoc ecs::view_iterareble::contains
    bool contains(entity const &entity) const override;

    /// @copydoc ecs::view_iterareble::size
    ecs::size_t size() const override;

    /// @copydoc ecs::impl::component_storage::clear
    void clear() override;

    view_iterable::iterator begin() const noexcept override;
    view_iterable::iterator end() const noexcept override;

    /// @copydoc ecs::impl::component_storage::clone
    std::unique_ptr<component_storage> clone() const override;
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

    /// @brief Get the component pool.
    /// @param id Arbitrary id or a string hash (not combined with the type id)
    template <typename T>
    component_array<T> const &storage(component_id id = 0) const;
    /// @copydoc storage
    template <typename T>
    component_array<T> &storage(component_id id = 0);

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

    /// @brief Get the number of components in an entity.
    /// @throws If the entity is not a valid identifier.
    /// @warning O(n) time (checks every pool).
    /// Keeping track of the entity's size in the entity manager is possible but would be unsafe,
    /// because the storage could be accessed without notifying the manager.
    ecs::size_t size(entity const &entity) const;

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
template <typename T>
inline T ecs::combine_hash(T const &lhs, T const &rhs) noexcept {
    ECS_PROFILE;
    return lhs ^ rhs + 0x9e3779b9 + (lhs << 6) + (lhs >> 2);
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
    mEntities.emplace(entity, 0);

    return entity;
}
inline void ecs::impl::entity_manager::erase(entity const &entity) {
    ECS_PROFILE;
    ECS_ASSERT(contains(entity), "Invalid entity identifier");
    mAvailableEntities.push_back(entity);
    mEntities.erase(entity);
}
inline bool ecs::impl::entity_manager::contains(entity const &entity) const {
    ECS_PROFILE;
    return 1 <= entity && entity < mNextID && mEntities.contains(entity);
}
inline void ecs::impl::entity_manager::clear() {
    ECS_PROFILE;
    mEntities.clear();
    mAvailableEntities.clear();
    mNextID = 1;
}
inline ecs::size_t ecs::impl::entity_manager::size() const {
    return mEntities.size();
}
inline ecs::impl::entity_manager::iterator ecs::impl::entity_manager::begin() const {
    return mEntities.sparse().begin();
}
inline ecs::impl::entity_manager::iterator ecs::impl::entity_manager::end() const {
    return mEntities.sparse().end();
}

template <typename T>
ecs::component_array<T>::component_array(): sparse_set<T>((CAPACITY + sizeof(T) - 1) / sizeof(T), PAGE_SIZE) {
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
inline std::unique_ptr<ecs::impl::component_storage> ecs::component_array<T>::clone() const {
    ECS_PROFILE;
    return std::unique_ptr<ecs::impl::component_storage>{ new ecs::component_array<T>{ *this } };
}

template <typename T>
inline ecs::component_array<T> *ecs::impl::component_manager::emplace(component_id id) {
    ECS_PROFILE;
    return static_cast<ecs::component_array<T> *>(mComponentArrays.emplace(id, std::make_unique<component_array<T>>()).get());
}
template <typename T>
inline ecs::component_array<T> *ecs::impl::component_manager::try_emplace(component_id id) {
    ECS_PROFILE;
    if(!mComponentArrays.contains(id)) {
        return emplace<T>(id);
    }
    return get<T>(id);
}
template <typename T>
inline ecs::component_array<T> *ecs::impl::component_manager::get(component_id id) {
    ECS_PROFILE;
    ECS_ASSERT(mComponentArrays.contains(id), "Component not registered before use");
    if(!mComponentArrays.contains(id))
        return nullptr;
    return static_cast<component_array<T> *>(mComponentArrays.get(id).get());
}
template <typename T>
inline ecs::component_array<T> const *ecs::impl::component_manager::get(component_id id) const {
    ECS_PROFILE;
    ECS_ASSERT(mComponentArrays.contains(id), "Component not registered before use");
    if(!mComponentArrays.contains(id))
        return nullptr;
    return static_cast<component_array<T> const *>(mComponentArrays.get(id).get());
}
inline ecs::impl::component_manager::component_manager(impl::component_manager const &other) {
    this->operator=(other);
}
inline ecs::impl::component_manager &ecs::impl::component_manager::operator=(impl::component_manager const &other) {
    ECS_PROFILE;
    for(auto [id, ptr] : other.mComponentArrays) {
        mComponentArrays.emplace(id, ptr->clone());
    }

    return *this;
}
inline void ecs::impl::component_manager::erase(entity const &entity) const {
    ECS_PROFILE;
    for(auto &componentArray : mComponentArrays.dense()) {
        componentArray->erase(entity);
    }
}
inline ecs::sparse_set<std::unique_ptr<ecs::impl::component_storage>> &ecs::impl::component_manager::storage() {
    return mComponentArrays;
}
inline ecs::sparse_set<std::unique_ptr<ecs::impl::component_storage>> const &ecs::impl::component_manager::storage() const {
    return mComponentArrays;
}

template <typename T>
ecs::component_array<T> const &ecs::registry::storage(component_id id) const {
    return *mComponentManager.try_emplace<T>(id);
}
template <typename T>
ecs::component_array<T> &ecs::registry::storage(ecs::component_id id) {
    return *mComponentManager.try_emplace<T>(id);
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
    for(ecs::size_t i = mComponentManager.storage().size(); i; --i) {
        mComponentManager.storage().begin()[i - 1u].second->clear();
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
inline ecs::size_t ecs::registry::size() const {
    ECS_PROFILE;
    return mEntityManager.size();
}
template <typename... Include, typename... Exclude>
inline ecs::view<sizeof...(Include), sizeof...(Exclude)> ecs::registry::view(exclude<Exclude...>) const {
    ECS_PROFILE;
    return ecs::view({ storage<Include>(0)... }, { storage<Exclude>(0)... });
}
inline ecs::registry::iterator ecs::registry::begin() const {
    return mEntityManager.begin();
}
inline ecs::registry::iterator ecs::registry::end() const {
    return mEntityManager.end();
}

template <ecs::size_t Include, ecs::size_t Exclude>
constexpr ecs::view<Include, Exclude>::view(std::array<view_iterable const *, Include> &&include,
    std::array<view_iterable const *, Exclude> &&exclude) noexcept: mInclude(std::move(include)), mExclude(std::move(exclude)) {
    if constexpr(Include > 1u) {
        auto &smallest = *std::min_element(mInclude.begin(), mInclude.end(),
            [](auto const &lhs, auto const &rhs) { return lhs->size() < rhs->size(); });
        std::swap(smallest, mInclude.back());
    }
}

template <ecs::size_t Include, ecs::size_t Exclude>
ecs::view<Include, Exclude>::iterator::iterator(typename ecs::view_iterable::iterator it, typename ecs::view_iterable::iterator end,
    view_iterable const **include, view_iterable const **exclude): mIt(it), mEnd(end), mInclude(include), mExclude(exclude) {
}

template <ecs::size_t Include, ecs::size_t Exclude>
bool ecs::view<Include, Exclude>::iterator::valid(entity const &e) const {
    ECS_PROFILE;
    constexpr auto pred = [&e](auto const &pool) { return pool->contains(e); };
    return ((Include == 1u) || std::all_of(mInclude, mInclude + Include - 1u, pred)) &&
           ((Exclude == 0u) || std::none_of(mExclude, mExclude + Exclude, pred));
}
template <ecs::size_t Include, ecs::size_t Exclude>
typename ecs::view<Include, Exclude>::iterator &ecs::view<Include, Exclude>::iterator::operator++() noexcept {
    ECS_PROFILE;
    if(mIt == mEnd) {
        return *this;
    }
    ++mIt;
    for(; mIt != mEnd && !valid(*mIt); ++mIt) {
    }
    return *this;
}
template <ecs::size_t Include, ecs::size_t Exclude>
typename ecs::view<Include, Exclude>::iterator ecs::view<Include, Exclude>::iterator::operator++(int) const noexcept {
    return ++iterator(this);
}

template <ecs::size_t Include, ecs::size_t Exclude>
typename ecs::view<Include, Exclude>::iterator::reference ecs::view<Include, Exclude>::iterator::operator*() const noexcept {
    return *mIt;
}

template <ecs::size_t Include, ecs::size_t Exclude>
constexpr bool ecs::view<Include, Exclude>::iterator::operator==(iterator const &other) const noexcept {
    return mIt == other.mIt;
}
template <ecs::size_t Include, ecs::size_t Exclude>
constexpr bool ecs::view<Include, Exclude>::iterator::operator!=(iterator const &other) const noexcept {
    return !operator==(other);
}

template <ecs::size_t Include, ecs::size_t Exclude>
typename ecs::view<Include, Exclude>::iterator ecs::view<Include, Exclude>::begin() const noexcept {
    return iterator(mInclude[Include - 1u]->begin(), mInclude[Include - 1u]->end(), mInclude.data(), mExclude.data());
}
template <ecs::size_t Include, ecs::size_t Exclude>
typename ecs::view<Include, Exclude>::iterator ecs::view<Include, Exclude>::end() const noexcept {
    return iterator(mInclude[Include - 1u]->end(), mInclude[Include - 1u]->end(), mInclude.data(), mExclude.data());
}
template <ecs::size_t Include, ecs::size_t Exclude>
ecs::size_t ecs::view<Include, Exclude>::size() const noexcept {
    return mInclude[Include - 1u]->size();
}

// Syntax sugar

template <typename... Components_t>
inline ecs::entity ecs::registry::create(Components_t &&...components) {
    ECS_PROFILE;

    entity entity = create();

    (storage<std::decay_t<Components_t>>(0).emplace(entity, std::forward<Components_t>(components)), ...);

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

    return storage<T>(0).contains(entity);
}
template <typename T>
inline T &ecs::registry::get(entity const &entity) {
    ECS_PROFILE;
    ECS_ASSERT(valid(entity), "Invalid entity identifier");
    ECS_ASSERT(contains<T>(entity), "Component to get is not added");

    return storage<T>(0).get(entity);
}
template <typename T>
inline T const &ecs::registry::get(entity const &entity) const {
    ECS_PROFILE;
    ECS_ASSERT(valid(entity), "Invalid entity identifier");
    ECS_ASSERT(contains<T>(entity), "Component to get is not added");

    return storage<T>(0).get(entity);
}
template <typename T>
inline void ecs::registry::erase(entity const &entity) {
    ECS_PROFILE;
    ECS_ASSERT(valid(entity), "Invalid entity identifier");
    ECS_ASSERT(contains<T>(entity), "Component to remove is not added");

    storage<T>(0).erase(entity);
}
template <typename T, class... Args>
inline T &ecs::registry::emplace(entity const &entity, Args &&...args) {
    ECS_PROFILE;
    ECS_ASSERT(valid(entity), "Invalid entity identifier");
    ECS_ASSERT(!contains<T>(entity), "Component to emplace already added");

    return storage<T>(0).emplace(entity, std::forward<Args>(args)...);
}

/*! \endcond */
