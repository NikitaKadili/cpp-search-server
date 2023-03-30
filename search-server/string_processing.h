#pragma once

#include <string>
#include <vector>
#include <set>
#include <string_view>

// Принимает текст в string_view, возвращает вектор слов
std::vector<std::string_view> SplitIntoWords(std::string_view text);

// Возвращает словарь уникальных слов из строки
template <typename StringContainer>
std::set<std::string, std::less<>> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    std::set<std::string, std::less<>> non_empty_strings;
    for (std::string_view str : strings) {
        if (!str.empty()) {
            non_empty_strings.insert(std::string{ str });
        }
    }
    return non_empty_strings;
}
