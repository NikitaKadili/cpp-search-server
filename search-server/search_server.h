#pragma once

#include <algorithm>
#include <cmath>
#include <deque>
#include <execution>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <vector>

#include "concurrent_map.h"
#include "document.h"
#include "string_processing.h"
#include "log_duration.h"

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double DOUBLE_ACCURACY = 1e-6; // Точность сравнения десятичных дробей

class SearchServer {
public:
    // Сокращенное наименование кортежа с результатом поиска метода MatchResult
    using MatchResult = std::tuple<std::vector<std::string_view>, DocumentStatus>;

    // Конструктор преобразует строку const std::string& в контейнер и вызывает шаблонный контруктор
    explicit SearchServer(const std::string& stop_words_text);

    // Конструктор преобразует строку std::string_view в контейнер и вызывает шаблонный контруктор
    explicit SearchServer(std::string_view stop_words_text);

    // Шаблонный контруктор проверяет и добавляет стоп-слова из шаблонного контейнера
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);

    // Добавление документа на сервер
    void AddDocument(int document_id, std::string_view document, DocumentStatus status,
        const std::vector<int>& ratings);

    // Шаблонный метод ищет документы по предикату
    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(std::string_view raw_query,
        DocumentPredicate document_predicate) const;

    // Поиск документов с заданным статусом
    std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentStatus status) const;

    // Поиск документов по умолчанию (только актуальные)
    std::vector<Document> FindTopDocuments(std::string_view raw_query) const;

    // Шаблонный метод ищет документы по предикату с заданной политикой исполнения
    template <typename DocumentPredicate, typename Policy>
    std::vector<Document> FindTopDocuments(Policy& policy, std::string_view raw_query,
        DocumentPredicate document_predicate) const;

    // Поиск документов с заданным статусом с заданной политикой исполнения
    template <typename Policy>
    std::vector<Document> FindTopDocuments(Policy& policy, std::string_view raw_query, DocumentStatus status) const;

    // Поиск документов по умолчанию (только актуальные) с заданной политикой исполнения
    template <typename Policy>
    std::vector<Document> FindTopDocuments(Policy& policy, std::string_view raw_query) const;

    // Возвращает кол-во документов
    int GetDocumentCount() const;

    // Возвращает итератор на начало document_ids_
    std::set<int>::iterator begin();

    // Возвращает итератор на конец document_ids_
    std::set<int>::iterator end();

    // Сверяет запрос с конкретным документом, возвращает совпавшие слова и статус документа
    MatchResult MatchDocument(std::string_view raw_query,
        int document_id) const;

    // Сверяет запрос с конкретным документом с заданной политикой исполнения (последовательной),
    // возвращает совпавшие слова и статус документа
    MatchResult MatchDocument(const std::execution::sequenced_policy&,
        std::string_view raw_query, int document_id) const;

    // Сверяет запрос с конкретным документом с заданной политикой исполнения (параллельной),
    // возвращает совпавшие слова и статус документа
    MatchResult MatchDocument(const std::execution::parallel_policy&,
        std::string_view raw_query, int document_id) const;

    // Возвращает частоту слов в документе по его id
    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;

    // Удаление документа по его id
    void RemoveDocument(int document_id);

    // Удаление документа по его id по заданной политике выполнения - последовательной
    void RemoveDocument(const std::execution::sequenced_policy&, int document_id);

    // Удаление документа по его id по заданной политике выполнения - параллельной
    void RemoveDocument(const std::execution::parallel_policy&, int document_id);

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    const std::set<std::string, std::less<>> stop_words_;
    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
    std::map<int, DocumentData> documents_;
    std::set<int> document_ids_;

    // Дэк для хранения строк добавляемых документов
    std::deque<std::string> document_;

    // Ключ - int id
    // Значение - map<string, double> (слово, частота в док-те)
    std::map<int, std::map<std::string_view, double>> id_to_words_and_freqs_;

    // Возвращает true, если строка является стоп-словом
    bool IsStopWord(std::string_view word) const;

    // Возвращает true, если строка не содержит спец-символы
    static bool IsValidWord(std::string_view word);

    // Возвращает вектор строк без стоп-слов
    std::vector<std::string_view> SplitIntoWordsNoStop(std::string_view text) const;

    // Расчитывает средний рейтинг
    static int ComputeAverageRating(const std::vector<int>& ratings);

    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    // Присваивает слову статус минус или плюс слова
    QueryWord ParseQueryWord(std::string_view text) const;

    struct Query {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };

    // Возвращает структуру с словарями плюс и минус слов
    Query ParseQuery(std::string_view text, bool skip_sorting = false) const;

    // Возвращает IDF
    double ComputeWordInverseDocumentFreq(std::string_view word) const;

    // Возвращает вектор всех найденных по запросу документов без стоп и минус слов
    // согласно условию функции-предиката
    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query& query,
        DocumentPredicate document_predicate) const;

    // Возвращает вектор всех найденных по запросу документов без стоп и минус слов
    // согласно условию функции-предиката с параллельной политикой исполнения
    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const std::execution::parallel_policy&, 
        const Query& query, DocumentPredicate document_predicate) const;
};

// Шаблонный контруктор проверяет и добавляет стоп-слова из шаблонного контейнера
template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
    : stop_words_(MakeUniqueNonEmptyStrings(stop_words))  // Extract non-empty stop words
{
    // Проверка слов на наличие спец-символов
    if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
        throw std::invalid_argument("Some of stop words are invalid");
    }
}

// Шаблонный метод ищет документы по предикату
template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query,
    DocumentPredicate document_predicate) const {
    std::string string_raw_query{ raw_query };
    const auto query = ParseQuery(string_raw_query);

    auto matched_documents = FindAllDocuments(query, document_predicate);

    sort(matched_documents.begin(), matched_documents.end(),
        [](const Document& lhs, const Document& rhs) {
            if (std::abs(lhs.relevance - rhs.relevance) < DOUBLE_ACCURACY) {
                return lhs.rating > rhs.rating;
            }
            else {
                return lhs.relevance > rhs.relevance;
            }
        });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }

    return matched_documents;
}

// Шаблонный метод ищет документы по предикату с заданной политикой исполнения
template <typename DocumentPredicate, typename Policy>
std::vector<Document> SearchServer::FindTopDocuments(Policy& policy,
    std::string_view raw_query, DocumentPredicate document_predicate) const {
    if (!std::is_same_v<std::decay_t<Policy>, std::execution::parallel_policy>) {
        // Ветка для последовательного исполнения
        return SearchServer::FindTopDocuments(raw_query, document_predicate);
    }
    
    // Ветка для параллельного исполнения
    std::string string_raw_query{ raw_query };
    const auto query = ParseQuery(string_raw_query);

    auto matched_documents = FindAllDocuments(std::execution::par, query, document_predicate);

    sort(matched_documents.begin(), matched_documents.end(),
        [](const Document& lhs, const Document& rhs) {
            if (std::abs(lhs.relevance - rhs.relevance) < DOUBLE_ACCURACY) {
                return lhs.rating > rhs.rating;
            }
            else {
                return lhs.relevance > rhs.relevance;
            }
        });

    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }

    return matched_documents;
}

// Поиск документов с заданным статусом с заданной политикой исполнения (последовательной)
template <typename Policy>
std::vector<Document> SearchServer::FindTopDocuments(Policy& policy,
    std::string_view raw_query, DocumentStatus status) const {
    if (!std::is_same_v<std::decay_t<Policy>, std::execution::parallel_policy>) {
        // Ветка для последовательного исполнения
        return SearchServer::FindTopDocuments(raw_query, status);
    }

    // Ветка для параллельного исполнения
    return SearchServer::FindTopDocuments(std::execution::par,
        raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}

// Поиск документов по умолчанию (только актуальные) с заданной политикой исполнения (последовательной)
template <typename Policy>
std::vector<Document> SearchServer::FindTopDocuments(Policy& policy,
    std::string_view raw_query) const {
    if (!std::is_same_v<std::decay_t<Policy>, std::execution::parallel_policy>) {
        // Ветка для последовательного исполнения
        return SearchServer::FindTopDocuments(raw_query);
    }

    // Ветка для параллельного исполнения
    return FindTopDocuments(std::execution::par, raw_query, DocumentStatus::ACTUAL);
}

// Возвращает вектор всех найденных по запросу документов без стоп и минус слов
// согласно условию функции-предиката
template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query,
    DocumentPredicate document_predicate) const {
    std::map<int, double> document_to_relevance;
    for (std::string_view word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            const auto& document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }

    for (std::string_view word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
            document_to_relevance.erase(document_id);
        }
    }

    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back(
            { document_id, relevance, documents_.at(document_id).rating });
    }
    return matched_documents;
}

// Возвращает вектор всех найденных по запросу документов без стоп и минус слов
// согласно условию функции-предиката с параллельной политикой исполнения
template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const std::execution::parallel_policy&,
    const Query& query, DocumentPredicate document_predicate) const {
    ConcurrentMap<int, double> document_to_relevance(document_ids_.size() / 4);

    for_each(std::execution::par, query.plus_words.begin(), query.plus_words.end(),
        [&](std::string_view word) {
            if (word_to_document_freqs_.find(word) == word_to_document_freqs_.end()) {
                return;
            }

            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto& [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                const auto& document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating)) {
                    document_to_relevance[document_id].ref_to_value += term_freq * inverse_document_freq;
                }
            }
        });

    for_each(std::execution::par, query.minus_words.begin(), query.minus_words.end(),
        [&](std::string_view word) {
            if (word_to_document_freqs_.find(word) == word_to_document_freqs_.end()) {
                return;
            }

            for (const auto& [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.Delete(document_id);
            }
        });


    std::vector<Document> matched_documents;
    for (const auto& [document_id, relevance] : document_to_relevance.BuildOrdinaryMap()) {
        matched_documents.push_back(
            { document_id, relevance, documents_.at(document_id).rating });
    }

    return matched_documents;
}
