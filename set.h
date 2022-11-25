#pragma once

#include <utility>

namespace intrusive {

struct set_element_base {
  set_element_base* left{nullptr};
  set_element_base* right{nullptr};
  set_element_base* parent{nullptr};
  std::size_t height = 1;
};

set_element_base* get_max_node_ptr(set_element_base* pointer) {
  while (pointer->right) {
    pointer = pointer->right;
  }
  return pointer;
}
set_element_base* get_min_node_ptr(set_element_base* pointer) {
  while (pointer->left) {
    pointer = pointer->left;
  }
  return pointer;
}

set_element_base* next(set_element_base* pointer) {
  if (pointer->right) {
    pointer = pointer->right;
    return get_min_node_ptr(pointer);
  }
  while (pointer->parent && pointer->parent->right == pointer) {
    pointer = pointer->parent;
  }
  return pointer->parent;
}
set_element_base* prev(set_element_base* pointer) {
  if (pointer->left) {
    return get_max_node_ptr(pointer->left);
  }
  while (pointer->parent && pointer->parent->left == pointer) {
    pointer = pointer->parent;
  }
  return pointer->parent;
}

template <class T, class type_T, class Tag,
          class Comparator = std::less<type_T>>
struct set : Comparator { /// AVL-tree

  set_element_base fictitious;
  set_element_base* root;

  explicit set(Comparator compare = Comparator()) : Comparator(compare) {
    root = &fictitious;
    root->parent = root;
  }

  set(set const& other) = delete;

  Comparator const& cmp() const {
    return static_cast<const Comparator&>(*this);
  }

  void upd(set_element_base* pointer) {
    pointer->height =
        std::max(height(pointer->left), height(pointer->right)) + 1;
  }

  void left_rotate(set_element_base* pointer) {
    set_element_base* ptr = pointer;
    pointer = pointer->left;

    ptr->left = pointer->right;
    if (ptr->left) {
      ptr->left->parent = ptr;
    }

    pointer->parent = ptr->parent;

    if (pointer->parent->left == ptr) {
      pointer->parent->left = pointer;
    } else {
      pointer->parent->right = pointer;
    }

    pointer->right = ptr;
    ptr->parent = pointer;

    upd(ptr);
    upd(pointer);
  }

  void right_rotate(set_element_base* pointer) {
    set_element_base* ptr = pointer;
    pointer = pointer->right;

    ptr->right = pointer->left;
    if (ptr->right) {
      ptr->right->parent = ptr;
    }

    pointer->parent = ptr->parent;
    if (pointer->parent->left == ptr) {
      pointer->parent->left = pointer;
    } else {
      pointer->parent->right = pointer;
    }

    pointer->left = ptr;
    ptr->parent = pointer;

    upd(ptr);
    upd(pointer);
  }

  std::size_t height(set_element_base* ptr) {
    return (ptr == nullptr ? 0 : ptr->height);
  }

  std::size_t balance(set_element_base* pointer) {
    if (height(pointer->left) > height(pointer->right)) {
      return height(pointer->left) - height(pointer->right);
    }
    return height(pointer->right) - height(pointer->left);
  }

  void correcter(set_element_base* pointer) {
    upd(pointer);
    if (balance(pointer) < 2) {
      return;
    }

    if (height(pointer->left) > height(pointer->right)) {
      auto ptr = pointer->left;
      if (height(ptr->left) > height(ptr->right)) {
        left_rotate(pointer);
      } else {
        right_rotate(ptr);
        left_rotate(pointer);
      }
    } else {
      auto ptr = pointer->right;
      if (height(ptr->left) < height(ptr->right)) {
        right_rotate(pointer);
      } else {
        left_rotate(ptr);
        right_rotate(pointer);
      }
    }
  }

  type_T const& get_value(set_element_base* pointer) const {
    return static_cast<T&>(*pointer).value;
  }

  set_element_base* lower_bound(const type_T& value,
                                set_element_base* pointer) const {
    if (pointer == nullptr) {
      return root;
    }
    if (get_value(pointer) == value) {
      return pointer;
    }
    if (cmp()(get_value(pointer), value)) {
      if (pointer->right) {
        return lower_bound(value, pointer->right);
      }
      return next(pointer);
    } else {
      if (pointer->left) {
        return lower_bound(value, pointer->left);
      }
      return pointer;
    }
  }

  set_element_base* lower_bound(const type_T& value) const {
    return lower_bound(value, root->left);
  }

  set_element_base* upper_bound(const type_T& value) const {
    set_element_base* tmp_pointer = lower_bound(value);
    if (tmp_pointer == root) {
      return root;
    }
    if (get_value(tmp_pointer) == value) {
      return next(tmp_pointer);
    }
    return tmp_pointer;
  }

  set_element_base* find_ptr(const type_T& value) const {
    set_element_base* pointer = lower_bound(value);

    if (pointer == root) {
      return root;
    }
    if (get_value(pointer) != value) {
      return root;
    }

    return pointer;
  }

  void insert(T& element, set_element_base* pointer) {
    auto& tree_value = get_value(pointer);
    if (cmp()(element.value, tree_value)) {
      if (pointer->left) {
        insert(element, pointer->left);
      } else {
        pointer->left = &static_cast<set_element_base&>(element);
        pointer->left->parent = pointer;
        upd(pointer);
        return;
      }
    } else {
      if (pointer->right) {
        insert(element, pointer->right);
      } else {
        pointer->right = &static_cast<set_element_base&>(element);
        pointer->right->parent = pointer;
        upd(pointer);
        return;
      }
    }
    correcter(pointer);
  }

  void insert(T& element) {
    if (root->left == nullptr) {
      root->left = &static_cast<set_element_base&>(element);
      root->left->parent = root;
      return;
    }
    insert(element, root->left);
  }

  void erase_one_child(set_element_base* pointer) {
    set_element_base* child_ptr;
    if (pointer->left) {
      child_ptr = pointer->left;
    } else {
      child_ptr = pointer->right;
    }

    child_ptr->parent = pointer->parent;
    if (pointer->parent->left == pointer) {
      pointer->parent->left = child_ptr;
    } else {
      pointer->parent->right = child_ptr;
    }
    upd(child_ptr->parent);
  }

  void swap_link_in_edge(set_element_base* up, set_element_base* down) {
    auto global_parent = up->parent;

    if (up->left == down) {
      auto up_right_child = up->right;

      up->left = down->left;
      up->right = down->right;
      up->parent = down;

      if (up->left) {
        up->left->parent = up;
      }
      if (up->right) {
        up->right->parent = up;
      }

      down->left = up;
      down->right = up_right_child;
    } else {
      auto up_left_child = up->left;

      up->left = down->left;
      up->right = down->right;
      up->parent = down;

      if (up->left) {
        up->left->parent = up;
      }
      if (up->right) {
        up->right->parent = up;
      }

      down->right = up;
      down->left = up_left_child;
    }

    if (down->left) {
      down->left->parent = down;
    }
    if (down->right) {
      down->right->parent = down;
    }

    if (global_parent->left == up) {
      global_parent->left = down;
    } else {
      global_parent->right = down;
    }

    down->parent = global_parent;

    upd(up);
    upd(down);
  }

  void swap_link(set_element_base* up, set_element_base* down) {
    if (up->left == down || up->right == down) {
      swap_link_in_edge(up, down);
      return;
    }

    if (up->left) {
      up->left->parent = down;
    }
    if (up->right) {
      up->right->parent = down;
    }
    if (down->left) {
      down->left->parent = up;
    }
    if (down->right) {
      down->right->parent = up;
    }

    std::swap(up->left, down->left);
    std::swap(up->right, down->right);
    std::swap(up->parent, down->parent);

    if (up->parent->left == down) {
      up->parent->left = up;
    } else {
      up->parent->right = up;
    }

    if (down->parent->left == up) {
      down->parent->left = down;
    } else {
      down->parent->right = down;
    }

    upd(up);
    upd(down);
  }

  set_element_base* erase(set_element_base* pointer) {
    if (!(pointer->left) && !(pointer->right)) {
      auto parent_ptr = pointer->parent;
      if (parent_ptr->left == pointer) {
        parent_ptr->left = nullptr;
      } else {
        parent_ptr->right = nullptr;
      }
      return parent_ptr;
    }
    if (!(pointer->left) || !(pointer->right)) {
      auto tmp = pointer->parent;
      erase_one_child(pointer);
      return tmp;
    }

    set_element_base* aim_node_ptr = get_max_node_ptr(pointer->left);
    swap_link(pointer, aim_node_ptr);
    return erase(pointer);
  }

  void erase(const type_T& value) {
    set_element_base* pointer = find_ptr(value);
    if (pointer == root) {
      return;
    }
    pointer = erase(pointer);
    while (pointer != root) {
      auto tmp_ptr = pointer->parent;
      correcter(pointer);
      pointer = tmp_ptr;
    }
  }

  set_element_base* begin_ptr() const {
    if (root->left) {
      return get_min_node_ptr(root->left);
    }
    return root;
  }
  set_element_base* end_ptr() const {
    return root;
  }
};
} // namespace intrusive
