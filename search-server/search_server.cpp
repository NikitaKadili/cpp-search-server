#include "search_server.h"

#include <iterator>

using namespace std;

// Конструктор преобразует строку string_view в контейнер и вызывает шаблонный контруктор
SearchServer::SearchServer(string_view stop_words_text)
    : SearchServer(
        SplitIntoWords(stop_words_text))  // Вызов шаблонного контруктора с контейнером
{}

// Конструктор преобразует строку const std::string& в контейнер и вызывает шаблонный контруктор
SearchServer::SearchServer(const string& stop_words_text)
    : SearchServer(
        SplitIntoWords(stop_words_text))  // Вызов шаблонного контруктора с контейнером
{}

// Добавление документа на сервер
void SearchServer::AddDocument(int document_id, string_view document, DocumentStatus status,
    const vector<int>& ratings) {
    document_.push_back(string{ document });

    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw invalid_argument("Invalid document_id"s);
    }
    const auto words = SplitIntoWordsNoStop(document_.back());

    const double inv_word_count = 1.0 / words.size();
    for (string_view word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        id_to_words_and_freqs_[document_id][word] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    document_ids_.insert(document_id);
}

// Поиск документов с заданным статусом
// Вызывает метод FindTopDocuments с предикатом
vector<Document> SearchServer::FindTopDocuments(string_view raw_query, DocumentStatus status) const {
    return SearchServer::FindTopDocuments(
        raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}

// Поиск документов по умолчанию (только актуальные)
// Вызывает метод FindTopDocuments с заданным статусом ACTUAL
vector<Document> SearchServer::FindTopDocuments(string_view raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

// Поиск документов с заданным статусом с заданной политикой исполнения (последовательной)
vector<Document> SearchServer::FindTopDocuments(const execution::sequenced_policy&, 
    string_view raw_query, DocumentStatus status) const {
    return SearchServer::FindTopDocuments(raw_query, status);
}

// Поиск документов по умолчанию (только актуальные) с заданной политикой исполнения (последовательной)
vector<Document> SearchServer::FindTopDocuments(const execution::sequenced_policy&, 
    string_view raw_query) const {
    return SearchServer::FindTopDocuments(raw_query);
}

// Поиск документов с заданным статусом с заданной политикой исполнения (параллельной)
vector<Document> SearchServer::FindTopDocuments(const execution::parallel_policy& policy,
    string_view raw_query, DocumentStatus status) const {
    return SearchServer::FindTopDocuments(policy,
        raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}

// Поиск документов по умолчанию (только актуальные) с заданной политикой исполнения (параллельной)
vector<Document> SearchServer::FindTopDocuments(const execution::parallel_policy& policy,
    string_view raw_query) const {
    return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}

// Возвращает кол-во документов
int SearchServer::GetDocumentCount() const {
    return static_cast<int>(documents_.size());
}

// Возвращает итератор на начало document_ids_
set<int>::iterator SearchServer::begin() {
    return document_ids_.begin();
}

// Возвращает итератор на конец document_ids_
set<int>::iterator SearchServer::end() {
    return document_ids_.end();
}

// Сверяет запрос с конкретным документом, возвращает совпавшие слова и статус документа
tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(string_view raw_query,
    int document_id) const {
    const auto query = ParseQuery(raw_query);

    for (string_view word : query.minus_words) {
        if (id_to_words_and_freqs_.at(document_id).count(word) != 0) {
            return { {}, documents_.at(document_id).status };
        }
    }

    vector<string_view> matched_words;
    for (string_view word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id) > 0) {
            matched_words.push_back(word);
        }
    }

    return { matched_words, documents_.at(document_id).status };
}

// Сверяет запрос с конкретным документом с заданной политикой исполнения (последовательной),
// возвращает совпавшие слова и статус документа
tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(const execution::sequenced_policy&,
    string_view raw_query, int document_id) const {
    return MatchDocument(raw_query, document_id);
}

// Сверяет запрос с конкретным документом с заданной политикой исполнения (параллельной),
// возвращает совпавшие слова и статус документа
tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(const execution::parallel_policy&,
    string_view raw_query, int document_id) const {
    const auto query = ParseQuery(raw_query, true);
    const auto& status = documents_.at(document_id).status;
    const auto& doc = id_to_words_and_freqs_.at(document_id);

    if (any_of(execution::par, query.minus_words.begin(), query.minus_words.end(),
        [&](string_view minus_word) {
            return doc.find(minus_word) != doc.end();
        })) {
        return { {}, status };
    }

    vector<string_view> matched_words(query.plus_words.size(), ""sv);

    transform(execution::par, query.plus_words.begin(), query.plus_words.end(),
        matched_words.begin(), [&](string_view plus_word) {
            return doc.find(plus_word) != doc.end() ? plus_word : ""sv;
        });

    sort(execution::par, matched_words.begin(), matched_words.end());
    matched_words.erase(unique(execution::par, matched_words.begin(), matched_words.end()), matched_words.end());
    
    // Удаляем первое пустое слово при наличии
    if (matched_words[0].empty()) {
        matched_words.erase(matched_words.begin());
    }
    
    return { matched_words, status };
}

// Возвращает частоту слов в документе по его id
const map<string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
    static map<string_view, double> empty_container{}; // Пустой контейнер

    auto it = id_to_words_and_freqs_.find(document_id);
    return (it != id_to_words_and_freqs_.end()
        ? it->second
        : empty_container);
}

// Удаление документа по его id
void SearchServer::RemoveDocument(int document_id) {
    const auto list_iterator_to_remove = find(document_ids_.begin(), document_ids_.end(), document_id);

    // Если такого id не существует, find вернет итератор на конец списка
    if (list_iterator_to_remove == document_ids_.end()) {
        return;
    }

    document_ids_.erase(list_iterator_to_remove); // Удаление из списка id
    documents_.erase(document_id); // Удаление из документов
    const auto words_from_doc = id_to_words_and_freqs_.at(document_id); // Создаем контейнер слов из удаляемого документа
    id_to_words_and_freqs_.erase(document_id);

    // Итерируемся по словам из удаляемого документа
    for (const auto& [word, _] : words_from_doc) {
        word_to_document_freqs_.at(word).erase(document_id);
    }
}

// Удаление документа по его id по заданной политике выполнения - последовательной
void SearchServer::RemoveDocument(const execution::sequenced_policy&, int document_id) {
    RemoveDocument(document_id);
}

// Удаление документа по его id по заданной политике выполнения - последовательной
void SearchServer::RemoveDocument(const execution::parallel_policy&, int document_id) {
    // Если такого id не существует, find вернет итератор на конец списка
    const auto list_iterator_to_remove = document_ids_.find(document_id);
    if (list_iterator_to_remove == document_ids_.end()) {
        return;
    }
    document_ids_.erase(list_iterator_to_remove); // Удаление из списка id
    
    // Для быстрой работы параллельных алгоритмов, создадим вектор указателей на слова, 
    // содержащиеся в документе с номером document_id
    vector<string_view> svs(id_to_words_and_freqs_.at(document_id).size());
    transform(execution::par,
        id_to_words_and_freqs_.at(document_id).begin(), id_to_words_and_freqs_.at(document_id).end(),
        svs.begin(), [](const auto& word_freqs) {
            return word_freqs.first;
        });

    for_each(execution::par,
        svs.begin(), svs.end(),
        [&](string_view ptr) {
            word_to_document_freqs_.at(ptr).erase(document_id);
            return;
        });
    
    documents_.erase(document_id); // Удаление из документов

    id_to_words_and_freqs_.erase(document_id);
}

// Возвращает true, если строка является стоп-словом
bool SearchServer::IsStopWord(string_view word) const {
    return stop_words_.count(word) > 0;
}

// Возвращает true, если строка не содержит спец-символы
bool SearchServer::IsValidWord(string_view word) {
    // A valid word must not contain special characters
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
        });
}

// Возвращает вектор строк без стоп-слов
vector<string_view> SearchServer::SplitIntoWordsNoStop(string_view text) const {
    vector<string_view> words;
    for (string_view word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw invalid_argument("Word "s + string{ word } + " is invalid"s);
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

// Расчитывает средний рейтинг
int SearchServer::ComputeAverageRating(const vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = 0;
    for (const int rating : ratings) {
        rating_sum += rating;
    }
    return rating_sum / static_cast<int>(ratings.size());
}

// Присваивает слову статус минус или плюс слова
SearchServer::QueryWord SearchServer::ParseQueryWord(string_view word) const {
    if (word.empty()) {
        throw invalid_argument("Query word is empty"s);
    }
    // word = text;
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        throw invalid_argument("Query word "s + string{ word } + " is invalid");
    }

    return {word, is_minus, IsStopWord(word) };
}

// Возвращает структуру с словарями плюс и минус слов
SearchServer::Query SearchServer::ParseQuery(string_view text, bool skip_sorting) const {
    Query result;

    for (string_view word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            }
            else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }

    if (!skip_sorting) {
        sort(result.plus_words.begin(), result.plus_words.end());
        sort(result.minus_words.begin(), result.minus_words.end());

        auto last = unique(result.minus_words.begin(), result.minus_words.end());
        result.minus_words.erase(last, result.minus_words.end());
        last = unique(result.plus_words.begin(), result.plus_words.end());
        result.plus_words.erase(last, result.plus_words.end());
    }

    return result;
}

// Возвращает IDF
double SearchServer::ComputeWordInverseDocumentFreq(string_view word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}
