#pragma once

/************************************************************************************************
 * The BTreeOLC in this file is based on the paper:                                            *
 *                                                                                              *
 *  Optimistic Lock Coupling: A Scalable and Efficient General-Purpose Synchronization Method.  *
 *  V Leis, M Haubenschild, T Neumann. IEEE Data Eng. Bull., 2019                               *
 *                                                                                              *
 *  and taken from the following GitHub repository:                                             *
 *                                                                                              *
 *  https://github.com/wangziqi2016/index-microbench                                            *
 *                                                                                              *
 ***********************************************************************************************/

#include <cstdint>
#include <atomic>
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <sched.h>
#ifdef __x86_64__
#include <immintrin.h>
#endif
#include "prefetch.h"
#include <perfcpp/analyzer/memory_access.h>
#include "coroutine/coroutine.h"


enum class PageType : uint8_t {
    BTreeInner = 1,
    BTreeLeaf = 2
};

static const uint64_t cacheLineSize = 64U;

struct OptLock {
    std::atomic<uint64_t> type_version_lock_obsolete{0b100};

    static bool is_locked(uint64_t version) { return ((version & 0b10) == 0b10); }

    std::uint64_t read_lock_or_restart(bool &needRestart) {
        const auto version = type_version_lock_obsolete.load();
        if (is_locked(version) || is_obsolete(version)) {
#ifdef __x86_64__
            _mm_pause();
#endif
            needRestart = true;
        }
        return version;
    }

    void write_lock_or_restart(bool &needRestart) {
        auto version = read_lock_or_restart(needRestart);
        if (needRestart)
            return;

        upgrade_to_write_lock_or_restart(version, needRestart);
        if (needRestart)
            return;
    }

    void upgrade_to_write_lock_or_restart(std::uint64_t &version, bool &needRestart) {
        if (type_version_lock_obsolete.compare_exchange_strong(version, version + 0b10)) {
            version = version + 0b10;
        } else {
#ifdef __x86_64__
            _mm_pause();
#endif
            needRestart = true;
        }
    }

    void write_unlock() { type_version_lock_obsolete.fetch_add(0b10); }

    static bool is_obsolete(const std::uint64_t version) { return (version & 1) == 1; }

    void check_or_restart(const std::uint64_t startRead, bool &needRestart) const {
        read_unlock_or_restart(startRead, needRestart);
    }

    void read_unlock_or_restart(const std::uint64_t startRead, bool &needRestart) const {
        needRestart = (startRead != type_version_lock_obsolete.load());
    }

    void writeUnlockObsolete() { type_version_lock_obsolete.fetch_add(0b11); }
};

struct NodeBase : public OptLock {
    PageType type;
    std::uint16_t count{0U};

    /**
     * Prefetches the entire node with the given size.
     * @tparam PageSize Size of the node.
     */
    template<std::size_t PageSize>
    void prefetch() {
        SWPrefetcher::prefetch<0U, PageSize / cacheLineSize, SWPrefetcher::Target::ALL>(this);
    }

    virtual ~NodeBase() = default;
};

struct BTreeLeafBase : public NodeBase {
    static const PageType typeMarker = PageType::BTreeLeaf;

    ~BTreeLeafBase() override = default;
};

template<class Key, class Payload, std::size_t PageSize>
struct alignas(PageSize) BTreeLeaf : public BTreeLeafBase {
    static const std::uint64_t maxEntries = (PageSize - sizeof(NodeBase)) / (sizeof(Key) + sizeof(Payload));

    Key keys[maxEntries];
    Payload payloads[maxEntries];

    BTreeLeaf() { type = typeMarker; }

    ~BTreeLeaf() override = default;

    bool isFull() { return count == maxEntries; };

    unsigned lowerBound(Key k) {
        unsigned lower = 0;
        unsigned upper = count;
        do {
            unsigned mid = ((upper - lower) / 2) + lower;
            if (k < keys[mid]) {
                upper = mid;
            } else if (k > keys[mid]) {
                lower = mid + 1;
            } else {
                return mid;
            }
        } while (lower < upper);
        return lower;
    }

    void insert(const Key key, const Payload payload) {
        assert(count < maxEntries);
        if (count) {
            const auto pos = lowerBound(key);
            if ((pos < count) && (keys[pos] == key)) {
                // Upsert
                payloads[pos] = payload;
                return;
            }
            std::memmove(keys + pos + 1, keys + pos, sizeof(Key) * (count - pos));
            std::memmove(payloads + pos + 1, payloads + pos, sizeof(Payload) * (count - pos));
            keys[pos] = key;
            payloads[pos] = payload;
        } else {
            keys[0] = key;
            payloads[0] = payload;
        }
        ++count;
    }

    BTreeLeaf *split(Key &sep) {
        void *align_ptr = std::aligned_alloc(PageSize, sizeof(BTreeLeaf));
        auto *new_leaf = new(align_ptr) BTreeLeaf();
        new_leaf->count = count - (count / 2);
        count = count - new_leaf->count;
        std::memcpy(new_leaf->keys, keys + count, sizeof(Key) * new_leaf->count);
        std::memcpy(new_leaf->payloads, payloads + count, sizeof(Payload) * new_leaf->count);
        sep = keys[count - 1];
        return new_leaf;
    }
};

struct BTreeInnerBase : public NodeBase {
    static const PageType typeMarker = PageType::BTreeInner;

    ~BTreeInnerBase() override = default;
};

template<class Key, std::size_t PageSize>
struct alignas(PageSize) BTreeInner : public BTreeInnerBase {
    static const uint64_t maxEntries = (PageSize - sizeof(NodeBase)) / (sizeof(Key) + sizeof(NodeBase *));

    Key keys[maxEntries]{};
    NodeBase *children[maxEntries]{};

    BTreeInner() {
        count = 0;
        type = typeMarker;
    }

    ~BTreeInner() override {
        for (auto i = 0u; i <= count; i++) {
            if (children[i] != nullptr) {
                delete children[i];
            }
        }
    }

    bool isFull() { return count == (maxEntries - 1); };

    unsigned lowerBound(Key k) {
        unsigned lower = 0;
        unsigned upper = count;
        do {
            unsigned mid = ((upper - lower) / 2) + lower;
            if (k < keys[mid]) {
                upper = mid;
            } else if (k > keys[mid]) {
                lower = mid + 1;
            } else {
                return mid;
            }
        } while (lower < upper);
        return lower;
    }

    BTreeInner *split(Key &sep) {
        void *align_ptr = std::aligned_alloc(PageSize, sizeof(BTreeInner));
        auto *newInner = new(align_ptr) BTreeInner();
        newInner->count = count - (count / 2);
        count = count - newInner->count - 1;
        sep = keys[count];
        std::memcpy(newInner->keys, keys + count + 1, sizeof(Key) * (newInner->count + 1));
        std::memcpy(newInner->children, children + count + 1, sizeof(NodeBase *) * (newInner->count + 1));
        return newInner;
    }

    void insert(const Key key, NodeBase *child) {
        assert(count < maxEntries - 1);
        const auto pos = lowerBound(key);
        std::memmove(keys + pos + 1, keys + pos, sizeof(Key) * (count - pos + 1));
        std::memmove(children + pos + 1, children + pos, sizeof(NodeBase *) * (count - pos + 1));
        keys[pos] = key;
        children[pos] = child;
        std::swap(children[pos], children[pos + 1]);
        count++;
    }
};

template<class Key, class Value, std::size_t PageSize = 256U>
struct BTree {
    using task_type = Coroutine;

    std::atomic<NodeBase *> root;

    BTree() {
        void *root_ptr = std::aligned_alloc(PageSize, sizeof(BTreeLeaf<Key, Value, PageSize>));
        root = new(root_ptr) BTreeLeaf<Key, Value, PageSize>();
    }

    ~BTree() { delete root.load(); }

    /**
     * Coroutinized insert_requests method that yields control-flow for prefetching.
     */
    Coroutine insert(const Key key, const Value value) {
        auto restart_count = 0U;
        restart:
        if (restart_count++)
            yield(restart_count);
        auto is_need_restart = false;
        auto tree_level = 0U;

        // Current node
        auto *node = root.load();
        auto version_node = node->read_lock_or_restart(is_need_restart);
        if (is_need_restart || (node != root))
            goto restart;

        // Parent of current node
        BTreeInner<Key, PageSize> *parent = nullptr;
        std::uint64_t version_parent;

        while (node->type == PageType::BTreeInner) {
            auto *inner = static_cast<BTreeInner<Key, PageSize> *>(node);

            // Split eagerly if full
            if (inner->isFull()) {
                // Lock
                if (parent) {
                    parent->upgrade_to_write_lock_or_restart(version_parent, is_need_restart);
                    if (is_need_restart)
                        goto restart;
                }
                node->upgrade_to_write_lock_or_restart(version_node, is_need_restart);
                if (is_need_restart) {
                    if (parent)
                        parent->write_unlock();
                    goto restart;
                }
                if (!parent && (node != root)) { // there's a new parent
                    node->write_unlock();
                    goto restart;
                }
                // Split
                Key sep;
                auto *new_inner = inner->split(sep);
                if (parent)
                    parent->insert(sep, new_inner);
                else
                    makeRoot(sep, inner, new_inner);
                // Unlock and restart
                node->write_unlock();
                if (parent)
                    parent->write_unlock();
                goto restart;
            }

            if (parent) {
                parent->read_unlock_or_restart(version_parent, is_need_restart);
                if (is_need_restart)
                    goto restart;
            }

            parent = inner;
            version_parent = version_node;
            const auto pos = inner->lowerBound(key);

            node = inner->children[pos];
            inner->check_or_restart(version_node, is_need_restart);
            if (is_need_restart)
                goto restart;

            /**
             * Accessing the follow up node => Prefetch complete node
             */
            node->prefetch<PageSize>();
            co_await Annotation{};

            version_node = node->read_lock_or_restart(is_need_restart);
            if (is_need_restart)
                goto restart;
            ++tree_level;
        }

        auto *leaf = static_cast<BTreeLeaf<Key, Value, PageSize> *>(node);

        // Split leaf if full
        if (leaf->count == leaf->maxEntries) {
            // Lock
            if (parent) {
                parent->upgrade_to_write_lock_or_restart(version_parent, is_need_restart);
                if (is_need_restart)
                    goto restart;
            }
            node->upgrade_to_write_lock_or_restart(version_node, is_need_restart);
            if (is_need_restart) {
                if (parent)
                    parent->write_unlock();
                goto restart;
            }
            if (!parent && (node != root)) { // there's a new parent
                node->write_unlock();
                goto restart;
            }
            // Split
            Key sep;
            auto *new_leaf = leaf->split(sep);
            if (parent)
                parent->insert(sep, new_leaf);
            else
                makeRoot(sep, leaf, new_leaf);
            // Unlock and restart
            node->write_unlock();
            if (parent)
                parent->write_unlock();
            goto restart;
        } else {
            // only lock leaf node
            node->upgrade_to_write_lock_or_restart(version_node, is_need_restart);
            if (is_need_restart)
                goto restart;
            if (parent) {
                parent->read_unlock_or_restart(version_parent, is_need_restart);
                if (is_need_restart) {
                    node->write_unlock();
                    goto restart;
                }
            }

            leaf->insert(key, value);

            node->write_unlock();

            co_return Annotation{}; // success
        }
    }

    Coroutine lookup(const Key key, Value &result) {
        auto restart_count = 0U;
        restart:
        if (restart_count++)
            yield(restart_count);
        auto is_need_restart = false;
        auto tree_level = 0U;

        /**
         * Prefetch the root node => Prefetch complete node
         * Could be also left out
         */
        auto *node = root.load();

        auto version_node = node->read_lock_or_restart(is_need_restart);
        if (is_need_restart || (node != root))
            goto restart;

        // Parent of current node
        BTreeInner<Key, PageSize> *parent = nullptr;
        std::uint64_t version_parent;

        while (node->type == PageType::BTreeInner) {
            auto *inner = static_cast<BTreeInner<Key, PageSize> *>(node);

            if (parent) {
                parent->read_unlock_or_restart(version_parent, is_need_restart);
                if (is_need_restart)
                    goto restart;
            }

            parent = inner;
            version_parent = version_node;

            const auto pos = inner->lowerBound(key);
            node = inner->children[pos];

            inner->check_or_restart(version_node, is_need_restart);
            if (is_need_restart)
                goto restart;

            /**
             * Accessing the follow up node => Prefetch complete node
             */
            node->prefetch<PageSize>();
            co_await Annotation{};

            version_node = node->read_lock_or_restart(is_need_restart);
            if (is_need_restart)
                goto restart;

            ++tree_level;
        }

        auto *leaf = static_cast<BTreeLeaf<Key, Value, PageSize> *>(node);

        const auto pos = leaf->lowerBound(key);
        if ((pos < leaf->count) && (leaf->keys[pos] == key)) {
            result = leaf->payloads[pos];
        }
        if (parent) {
            parent->read_unlock_or_restart(version_parent, is_need_restart);
            if (is_need_restart)
                goto restart;
        }
        node->read_unlock_or_restart(version_node, is_need_restart);
        if (is_need_restart)
            goto restart;

        co_return Annotation{};
    }

    void makeRoot(Key k, NodeBase *leftChild, NodeBase *rightChild) {
        void *align_ptr = std::aligned_alloc(PageSize, sizeof(BTreeInner<Key, PageSize>));
        auto inner = new(align_ptr) BTreeInner<Key, PageSize>();
        inner->count = 1;
        inner->keys[0] = k;
        inner->children[0] = leftChild;
        inner->children[1] = rightChild;
        root = inner;
    }

    void yield(int count) {
        if (count > 3) {
            sched_yield();
        } else {
#ifdef __x86_64__
            _mm_pause();
#endif
        }
    }

    /**
     * @return A description of inner and leaf nodes structures.
     */
    std::pair<perf::analyzer::DataType, perf::analyzer::DataType> get_node_structures() {
        auto inner_node = perf::analyzer::DataType{"InnerNode", PageSize};
        inner_node.add("vptr", 8U);
        inner_node.add("latch", 8U);
        inner_node.add("page_type", 2U);
        inner_node.add("count", 2U);
        inner_node.add("--padding--", 4U);
        inner_node.add("keys", 112U);
        inner_node.add("children", 112U);
        inner_node.add("--padding--", 8U);

        auto leaf_node = perf::analyzer::DataType{"LeafNode", PageSize};
        leaf_node.add("vptr", 8U);
        leaf_node.add("latch", 8U);
        leaf_node.add("page_type", 2U);
        leaf_node.add("count", 2U);
        leaf_node.add("--padding--", 4U);
        leaf_node.add("keys", 112U);
        leaf_node.add("payloads", 112U);
        leaf_node.add("--padding--", 8U);

        return std::make_pair(std::move(inner_node), std::move(leaf_node));
    }

    /**
     * Traverses the tree and adds all nodes to the memory access analyzer.
     * @param memory_access_analyzer Analyzer to add nodes to.
     */
    void traverse_tree_and_add_nodes(perf::analyzer::MemoryAccess &memory_access_analyzer) {
        this->traverse_tree_and_add_node(memory_access_analyzer, 0U, root.load());
    }

    /**
     * Traverses the tree and adds all nodes.
     *
     * @param memory_access_analyzer The access analyzer the nodes are added to.
     * @param level The current level of the node, needed to tag inner nodes.
     * @param node The node itself.
     */
    void traverse_tree_and_add_node(perf::analyzer::MemoryAccess &memory_access_analyzer, const std::uint8_t level, NodeBase *node) {
        if (node->type == PageType::BTreeInner) {
            /// Create a tag to distinct different inner node levels.
            auto tag = std::string{"lvl-"}.append(std::to_string(level));

            /// Add the node along with the tag.
            memory_access_analyzer.annotate("InnerNode", node, std::move(tag));

            /// Traverse child nodes.
            auto *inner = reinterpret_cast<BTreeInner<Key, PageSize> *>(node);
            for (auto i = 0U; i <= inner->count; ++i) {
                this->traverse_tree_and_add_node(memory_access_analyzer, level + 1U, inner->children[i]);
            }
        } else {
            /// Add the node.
            memory_access_analyzer.annotate("LeafNode", node);
        }
    }
};
