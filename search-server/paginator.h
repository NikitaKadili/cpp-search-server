#pragma once

#include <iostream>

// Объект класса хранит заданный диапозон
template <typename RandomIt>
class IteratorRange {
public:
    IteratorRange(RandomIt first, RandomIt last)
        : first_(first)
        , last_(last)
        , size_(distance(first, last)) {}

    // Вернуть начало диапозона
    RandomIt begin() {
        return first_;
    }

    // Вернуть конец диапозона
    RandomIt end() {
        return last_;
    }

    // Вернуть размер диапозона
    size_t size() {
        return size_;
    }

private:
    RandomIt first_;
    RandomIt last_;
    size_t size_;
};

// Перезгрузка оператора вывода диапозона IteratorRange
template <typename RandomIt>
std::ostream& operator<<(std::ostream& os, IteratorRange<RandomIt> range) {
    for (auto it = range.begin(); it != range.end(); ++it) {
        os << *it;
    }
    return os;
}

template <typename RandomIt>
class Paginator {
public:
    Paginator(RandomIt first, RandomIt last, size_t page_size) {
        for (size_t left = distance(first, last); left > 0;) {
            // Минимум между оставшимся и заданным количеством элементов страницы
            // будет равен размеру текущей страницы
            const size_t current_page_size = std::min(left, page_size);
            const RandomIt current_page_end = next(first, current_page_size); // Сдвигаемся на размер текущей страницы

            pages_.push_back({ first, current_page_end });
            left -= current_page_size; // Вычитаем размер текущей страницы
            first = current_page_end; // Первый итератор след. страницы = последнему итератору предыдущей
        }
    }

    // Вернуть первый итератор вектора страниц
    auto begin() const {
        return pages_.begin();
    }

    // Вернуть последний итератор вектора страниц
    auto end() const {
        return pages_.end();
    }

    // Вернуть количество страниц
    size_t size() const {
        return pages_.size();
    }

private:
    std::vector<IteratorRange<RandomIt>> pages_;
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}
