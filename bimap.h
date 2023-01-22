#pragma once

#include <stdexcept>
#include <type_traits>

#include "set.h"

template <typename Left, typename Right, typename CompareLeft = std::less<Left>,
          typename CompareRight = std::less<Right>>
struct bimap {

private:
  struct LEFT_TAG;
  struct RIGHT_TAG;

  using left_t = Left;
  using right_t = Right;

  struct node : intrusive::set_element<Left, LEFT_TAG>,
                intrusive::set_element<Right, RIGHT_TAG> {
    node(Left&& left, Right&& right)
        : intrusive::set_element<Left, LEFT_TAG>(std::move(left)),
          intrusive::set_element<Right, RIGHT_TAG>(std::move(right)) {}

    node(Left const& left, Right&& right)
        : intrusive::set_element<Left, LEFT_TAG>(left),
          intrusive::set_element<Right, RIGHT_TAG>(std::move(right)) {}

    node(Left&& left, Right const& right)
        : intrusive::set_element<Left, LEFT_TAG>(std::move(left)),
          intrusive::set_element<Right, RIGHT_TAG>(right) {}

    node(Left const& left, Right const& right)
        : intrusive::set_element<Left, LEFT_TAG>(left),
          intrusive::set_element<Right, RIGHT_TAG>(right) {}
  };

  using node_t = node;

  std::size_t bimap_size = 0;
  intrusive::set<Left, LEFT_TAG, CompareLeft> left_set;
  intrusive::set<Right, RIGHT_TAG, CompareRight> right_set;

public:
  template <class iterator_value, class iterator_tag,
            class other_iterator_value, class other_iterator_tag>
  struct base_iterator {
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = iterator_value;
    using pointer = iterator_value*;
    using reference = iterator_value&;

    base_iterator() = default;

    base_iterator(intrusive::set_element_base* ptr) : ptr(ptr) {}

    base_iterator& operator++() {
      ptr = ptr->next();
      return *this;
    }
    base_iterator operator++(int) {
      auto tmp = *this;
      ptr = ptr->next();
      return tmp;
    }

    base_iterator& operator--() {
      ptr = ptr->prev();
      return *this;
    }
    base_iterator operator--(int) {
      auto tmp = *this;
      ptr = ptr->prev();
      return tmp;
    }

    reference operator*() const {
      return static_cast<intrusive::set_element<iterator_value, iterator_tag>&>(
                 *ptr)
          .value;
    }
    pointer operator->() const {
      return &operator*();
    }

    using other_type_iterator =
        base_iterator<other_iterator_value, other_iterator_tag, iterator_value,
                      iterator_tag>;
    other_type_iterator flip() const {
      if (ptr->parent->parent == ptr) {
        return other_type_iterator(ptr->parent);
      }

      auto* tmp_node =
          static_cast<intrusive::set_element<iterator_value, iterator_tag>*>(
              ptr);
      auto* tmp_lca_node = static_cast<node*>(tmp_node);
      auto* other_tmp_node = static_cast<
          intrusive::set_element<other_iterator_value, other_iterator_tag>*>(
          tmp_lca_node);

      return other_type_iterator(
          static_cast<intrusive::set_element_base*>(other_tmp_node));
    }

    node_t* get_ptr_node_t() const {
      auto* tmp_node =
          static_cast<intrusive::set_element<iterator_value, iterator_tag>*>(
              ptr);
      return static_cast<node_t*>(tmp_node);
    }

    bool operator==(base_iterator const& other) const {
      return other.ptr == ptr;
    }
    bool operator!=(base_iterator const& other) const {
      return other.ptr != ptr;
    }

  private:
    intrusive::set_element_base* ptr = nullptr;
  };

  using left_iterator = base_iterator<left_t, LEFT_TAG, right_t, RIGHT_TAG>;
  using right_iterator = base_iterator<right_t, RIGHT_TAG, left_t, LEFT_TAG>;

  // Создает bimap не содержащий ни одной пары.
  bimap(CompareLeft compare_left = CompareLeft(),
        CompareRight compare_right = CompareRight())
      : left_set(std::move(compare_left)), right_set(std::move(compare_right)) {
    left_set.root->parent = right_set.root;
    right_set.root->parent = left_set.root;
  }

  // Конструкторы от других и присваивания
  bimap(bimap const& other) {
    left_set.root->parent = right_set.root;
    right_set.root->parent = left_set.root;

    try {
      for (auto it = other.begin_left(); it != other.end_left(); ++it) {
        insert(*it, *it.flip());
      }
    } catch (...) {
      while (size() > 0) {
        erase_left(begin_left());
      }
      throw;
    }
  }

  bimap(bimap&& other) noexcept = default;

  bimap& operator=(bimap const& other) {
    if (*this != other) {
      bimap(other).swap(*this);
    }
    return *this;
  }

  bimap& operator=(bimap&& other) noexcept {
    if (*this != other) {
      bimap(std::move(other)).swap(*this);
    }
    return *this;
  }

  // Деструктор. Вызывается при удалении объектов bimap.
  // Инвалидирует все итераторы ссылающиеся на элементы этого bimap
  // (включая итераторы ссылающиеся на элементы следующие за последними).
  ~bimap() noexcept {
    erase_left(begin_left(), end_left());
  }

  // Вставка пары (left, right), возвращает итератор на left.
  // Если такой left или такой right уже присутствуют в bimap, вставка не
  // производится и возвращается end_left().

  left_iterator insert(left_t&& left, right_t&& right) {
    return perfect_insert(std::forward<left_t>(left),
                          std::forward<right_t>(right));
  }
  left_iterator insert(left_t const& left, right_t&& right) {
    return perfect_insert(left, std::move(right));
  }
  left_iterator insert(left_t&& left, right_t const& right) {
    return perfect_insert(std::forward<left_t>(left), right);
  }
  left_iterator insert(left_t const& left, right_t const& right) {
    return perfect_insert(left, right);
  }

  // Удаляет элемент и соответствующий ему парный.
  // erase невалидного итератора неопределен.
  // erase(end_left()) и erase(end_right()) неопределены.
  // Пусть it ссылается на некоторый элемент e.
  // erase инвалидирует все итераторы ссылающиеся на e и на элемент парный к e.
  left_iterator erase_left(left_iterator it) {
    auto tmp = it++;
    remove(tmp);
    return it;
  }

  // Аналогично erase, но по ключу, удаляет элемент если он присутствует, иначе
  // не делает ничего Возвращает была ли пара удалена
  bool erase_left(left_t const& left) {
    auto it = find_left(left);
    if (it == end_left()) {
      return false;
    }
    remove(it);
    return true;
  }

  right_iterator erase_right(right_iterator it) {
    auto tmp = it++;
    remove(tmp);
    return it;
  }

  bool erase_right(right_t const& right) {
    auto it = find_right(right);
    if (it == end_right()) {
      return false;
    }
    remove(it);
    return true;
  }

  // erase от ренжа, удаляет [first, last), возвращает итератор на последний
  // элемент за удаленной последовательностью
  left_iterator erase_left(left_iterator first, left_iterator last) {
    while (first != last) {
      auto tmp_it = first++;
      remove(tmp_it);
    }
    return first;
  }
  right_iterator erase_right(right_iterator first, right_iterator last) {
    while (first != last) {
      auto tmp_it = first++;
      remove(tmp_it);
    }
    return first;
  }

  // Возвращает итератор по элементу. Если не найден - соответствующий end()
  left_iterator find_left(left_t const& left) const {
    return left_iterator(left_set.find_ptr(left));
  }
  right_iterator find_right(right_t const& right) const {
    return right_iterator(right_set.find_ptr(right));
  }

  // Возвращает противоположный элемент по элементу
  // Если элемента не существует -- бросает std::out_of_range
  Right const& at_left(left_t const& key) const {
    auto it = find_left(key);
    if (it == end_left()) {
      throw std::out_of_range(
          "left element wasn't found at 'at_left' function");
    }
    return *it.flip();
  }
  Left const& at_right(right_t const& key) const {
    auto it = find_right(key);
    if (it == end_right()) {
      throw std::out_of_range(
          "right element wasn't found at 'at_right' function");
    }
    return *it.flip();
  }

  // Возвращает противоположный элемент по элементу
  // Если элемента не существует, добавляет его в bimap и на противоположную
  // сторону кладет дефолтный элемент, ссылку на который и возвращает
  // Если дефолтный элемент уже лежит в противоположной паре - должен поменять
  // соответствующий ему элемент на запрашиваемый (смотри тесты)

  template <typename T = Right,
            std::enable_if_t<std::is_default_constructible_v<T>, int> = 0>
  right_t const& at_left_or_default(left_t const& key) {
    auto it = find_left(key);
    if (it == end_left()) {
      auto tmp_default_value = right_t();
      if (find_right(tmp_default_value) != end_right()) {
        erase_right(tmp_default_value);
      }
      it = insert(key, tmp_default_value);
    }
    return *it.flip();
  }

  template <typename T = Left,
            std::enable_if_t<std::is_default_constructible_v<T>, int> = 0>
  left_t const& at_right_or_default(right_t const& key) {
    auto it = find_right(key);
    if (it == end_right()) {
      auto tmp_default_value = left_t();
      if (find_left(tmp_default_value) != end_left()) {
        erase_left(tmp_default_value);
      }
      insert(tmp_default_value, key);
      it = find_right(key);
    }
    return *it.flip();
  }

  // lower и upper bound'ы по каждой стороне
  // Возвращают итераторы на соответствующие элементы
  // Смотри std::lower_bound, std::upper_bound.
  left_iterator lower_bound_left(const left_t& left) const {
    return left_iterator(left_set.lower_bound(left));
  }
  left_iterator upper_bound_left(const left_t& left) const {
    return left_iterator(left_set.upper_bound(left));
  }
  right_iterator lower_bound_right(const right_t& right) const {
    return right_iterator(right_set.lower_bound(right));
  }
  right_iterator upper_bound_right(const right_t& right) const {
    return right_iterator(right_set.upper_bound(right));
  }

  // Возващает итератор на минимальный по порядку left.
  left_iterator begin_left() const {
    return left_iterator(left_set.begin_ptr());
  }
  // Возващает итератор на следующий за последним по порядку left.
  left_iterator end_left() const {
    return left_iterator(left_set.end_ptr());
  }

  // Возващает итератор на минимальный по порядку right.
  right_iterator begin_right() const {
    return right_iterator(right_set.begin_ptr());
  }
  // Возващает итератор на следующий за последним по порядку right.
  right_iterator end_right() const {
    return right_iterator(right_set.end_ptr());
  }

  // Проверка на пустоту
  bool empty() const {
    return bimap_size == 0;
  }

  // Возвращает размер бимапы (кол-во пар)
  std::size_t size() const {
    return bimap_size;
  }

  bool operator==(bimap const& other) const {
    if (bimap_size != other.bimap_size) {
      return false;
    }
    for (auto it = begin_left(), other_it = other.begin_left();
         it != end_left(); it++, other_it++) {
      if (left_set.cmp()(*it, *other_it) != left_set.cmp()(*other_it, *it)) {
        return false;
      }

      if (right_set.cmp()(*it.flip(), *other_it.flip()) !=
          right_set.cmp()(*other_it.flip(), *it.flip())) {
        return false;
      }

      return true;
    }
  }

  bool operator!=(bimap const& other) const {
    return !(*this == other);
  }

  void swap(bimap& other) {
    std::swap(left_set.root, other.left_set.root);
    std::swap(right_set.root, other.right_set.root);
    std::swap(bimap_size, other.bimap_size);
  }

private:
  template <class iterator_type>
  void remove(iterator_type it) {
    bimap_size--;

    auto* ptr_node = it.get_ptr_node_t();

    auto& left_value =
        static_cast<intrusive::set_element<Left, LEFT_TAG>&>(*ptr_node).value;
    auto& right_value =
        static_cast<intrusive::set_element<Right, RIGHT_TAG>&>(*ptr_node).value;

    left_set.erase(left_value);
    right_set.erase(right_value);

    delete ptr_node;
  }

  template <class left_type = left_t, class right_type = right_t>
  left_iterator perfect_insert(left_type&& left, right_type&& right) {
    if (find_left(left) != end_left() || find_right(right) != end_right()) {
      return end_left();
    }

    node_t* pointer = new node_t(std::forward<left_type>(left),
                                 std::forward<right_type>(right));

    auto& l_node =
        static_cast<intrusive::set_element<Left, LEFT_TAG>&>(*pointer);
    auto& r_node =
        static_cast<intrusive::set_element<Right, RIGHT_TAG>&>(*pointer);

    right_set.insert(r_node);
    left_set.insert(l_node);
    bimap_size++;

    return left_iterator(left_set.find_ptr(l_node.value));
  }
};
