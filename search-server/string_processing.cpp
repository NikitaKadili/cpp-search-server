#include "string_processing.h"

using namespace std;

// Принимает текст в string_view, возвращает вектор слов
vector<string_view> SplitIntoWords(string_view text) {
    vector<string_view> result;

    // Находим и отсекаем все пробелы в начале строки
    size_t pos = text.find_first_not_of(' ');
    text.remove_prefix(pos == text.npos ? text.length() : pos);

    while (!text.empty()) {
        // Находим пробел, следующий за окончанием первого слова, добавляем слово в result
        pos = text.find_first_of(' ');
        result.push_back(text.substr(0, (pos == text.npos ? text.length() : pos)));

        // Удаляем текущее слова и следующий за ним пробел (при наличии)
        text.remove_prefix(pos == text.npos ? text.length() : ++pos);

        // Находим и удаляем оставшиеся за словом пробелы (при наличии)
        pos = text.find_first_not_of(' ');
        text.remove_prefix(pos == text.npos ? text.length() : pos);
    }

    return result;
}
