// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <concepts>
#include <type_traits>
#include "common/types.h"

namespace Common {

template <typename TickType = u64>
struct LRUNode {
    LRUNode* prev{};
    LRUNode* next{};
    TickType tick{};
    bool linked{};
};

template <typename ObjectType, typename TickType = u64>
    requires std::derived_from<ObjectType, LRUNode<TickType>>
class LRUCache {
    using Node = LRUNode<TickType>;

    static Node& FromObject(ObjectType& object) {
        return static_cast<Node&>(object);
    }

    static ObjectType& ToObject(Node& node) {
        return static_cast<ObjectType&>(node);
    }

public:
    void Insert(ObjectType& obj, TickType tick) {
        Node& node = FromObject(obj);
        node.tick = tick;
        Attach(node);
    }

    void Touch(ObjectType& obj, TickType tick) {
        Node& node = FromObject(obj);
        if (node.tick >= tick) {
            return;
        }
        node.tick = tick;
        if (&node == last) {
            return;
        }
        Detach(node);
        Attach(node);
    }

    void Free(ObjectType& obj) {
        Node& node = FromObject(obj);
        if (node.linked) {
            Detach(node);
        }
    }

    template <typename Func>
    void ForEachItemBelow(TickType tick, Func&& func) {
        static constexpr bool RETURNS_BOOL =
            std::is_same_v<std::invoke_result<Func, ObjectType>, bool>;
        Node* it = first;
        while (it) {
            if (static_cast<s64>(tick) - static_cast<s64>(it->tick) < 0) {
                return;
            }
            Node* next = it->next;
            auto& obj = ToObject(*it);
            if constexpr (RETURNS_BOOL) {
                if (func(obj)) {
                    return;
                }
            } else {
                func(obj);
            }
            it = next;
        }
    }

private:
    void Attach(Node& node) {
        node.prev = last;
        node.next = nullptr;
        if (last) {
            last->next = &node;
        } else {
            first = &node;
        }
        last = &node;
        node.linked = true;
    }

    void Detach(Node& node) {
        if (node.prev) {
            node.prev->next = node.next;
        } else {
            first = node.next;
        }
        if (node.next) {
            node.next->prev = node.prev;
        } else {
            last = node.prev;
        }
        node.prev = node.next = nullptr;
        node.linked = false;
    }

    Node* first{};
    Node* last{};
};

} // namespace Common
