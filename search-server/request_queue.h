#pragma once

#include <deque>
#include <string>
#include <vector>
#include "document.h"
#include "search_server.h"

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);

    // сделаем "обёртки" для всех методов поиска, чтобы сохранять результаты для нашей статистики
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);

    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);

    std::vector<Document> AddFindRequest(const std::string& raw_query);

    int GetNoResultRequests() const;

private:
    // Статус результата поиска
    enum class RequestStatus {
        SUCCEED, // Успешный поиск - что-то нашлось
        UNSUCEED // Ничего не нашлось
    };

    // Структура содержит статус результата поиска и время, в которое был сделан запрос
    struct QueryResult {
        RequestStatus status;
        uint64_t timestamp;
    };

    std::deque<QueryResult> requests_; // История запросов за последние сутки
    const static int min_in_day_ = 1440; // Кол-во секунд в сутки
    const SearchServer& search_server_; // Ссылка на объект сервера
    uint64_t current_time_; // Текущее время

    void RefillActualRequests(const std::vector<Document>& result);
};

template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
    const auto result = search_server_.FindTopDocuments(raw_query, document_predicate);
    RefillActualRequests(result);
    return result;
}