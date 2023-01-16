#pragma once

#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>
#include "document.h"
#include "string_processing.h"

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double DOUBLE_ACCURACY = 1e-6; // Точность сравнения десятичных дробей

class SearchServer {
public:
    // Шаблонный контруктор проверяет и добавляет стоп-слова из шаблонного контейнера
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);

    // Конструктор преобразует строку в контейнер и вызывает шаблонный контруктор
    explicit SearchServer(const std::string& stop_words_text);

    // Добавление документа на сервер
    void AddDocument(int document_id, const std::string& document, DocumentStatus status,
        const std::vector<int>& ratings);

    // Шаблонный метод ищет документы по предикату
    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string& raw_query,
        DocumentPredicate document_predicate) const;

    // Поиск документов с заданным статусом
    std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentStatus status) const;

    // Поиск документов по умолчанию (только актуальные)
    std::vector<Document> FindTopDocuments(const std::string& raw_query) const;

    // Возвращает кол-во документов
    int GetDocumentCount() const;

    // Возвращает ID документа по порядковому номеру
    int GetDocumentId(int index) const;

    // Сверяет запрос с конкретным документом, возвращает совпавшие слова и статус документа
    std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(const std::string& raw_query,
        int document_id) const;

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    const std::set<std::string> stop_words_;
    std::map<std::string, std::map<int, double>> word_to_document_freqs_;
    std::map<int, DocumentData> documents_;
    std::vector<int> document_ids_;

    // Возвращает true, если строка является стоп-словом
    bool IsStopWord(const std::string& word) const;

    // Возвращает true, если строка не содержит спец-символы
    static bool IsValidWord(const std::string& word);

    // Возвращает вектор строк без стоп-слов
    std::vector<std::string> SplitIntoWordsNoStop(const std::string& text) const;

    // Расчитывает средний рейтинг
    static int ComputeAverageRating(const std::vector<int>& ratings);

    struct QueryWord {
        std::string data;
        bool is_minus;
        bool is_stop;
    };

    // Присваивает слову статус минус или плюс слова
    QueryWord ParseQueryWord(const std::string& text) const;

    struct Query {
        std::set<std::string> plus_words;
        std::set<std::string> minus_words;
    };

    // Возвращает структуру с словарями плюс и минус слов
    Query ParseQuery(const std::string& text) const;

    // Возвращает IDF
    double ComputeWordInverseDocumentFreq(const std::string& word) const;

    // Возвращает вектор всех найденных по запросу документов без стоп и минус слов
    // согласно условию функции-предиката
    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query& query,
        DocumentPredicate document_predicate) const;
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
std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query,
    DocumentPredicate document_predicate) const {
    const auto query = ParseQuery(raw_query);

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

// Возвращает вектор всех найденных по запросу документов без стоп и минус слов
// согласно условию функции-предиката
template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query,
    DocumentPredicate document_predicate) const {
    std::map<int, double> document_to_relevance;
    for (const std::string& word : query.plus_words) {
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

    for (const std::string& word : query.minus_words) {
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
