#pragma once

#include <string>
#include <vector>
#include <set>

// Разделить строку на вектор слов
std::vector<std::string> SplitIntoWords(const std::string& text);

// Возвращает словарь уникальных слов из строки
template <typename StringContainer>
std::set<std::string> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    std::set<std::string> non_empty_strings;
    for (const std::string& str : strings) {
        if (!str.empty()) {
            non_empty_strings.insert(str);
        }
    }
    return non_empty_strings;
}
