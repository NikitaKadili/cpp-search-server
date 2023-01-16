#include "request_queue.h"

#include <algorithm>

using namespace std;

RequestQueue::RequestQueue(const SearchServer& search_server)
    : search_server_(search_server)
    , current_time_(0) {}

vector<Document> RequestQueue::AddFindRequest(const string& raw_query, DocumentStatus status) {
    const auto result = search_server_.FindTopDocuments(raw_query, status);
    RefillActualRequests(result);
    return result;
}

vector<Document> RequestQueue::AddFindRequest(const string& raw_query) {
    const auto result = search_server_.FindTopDocuments(raw_query);
    RefillActualRequests(result);
    return result;
}

int RequestQueue::GetNoResultRequests() const {
    const auto result = count_if(requests_.begin(), requests_.end(),
        [](const QueryResult& qr) {
            return qr.status == RequestStatus::UNSUCEED;
        });
    return static_cast<int>(result);
}

void RequestQueue::RefillActualRequests(const vector<Document>& result) {
    ++current_time_; // Обновляем время

    // Сохраняем текущий запрос в историю
    requests_.push_front({ (result.empty()
        ? RequestStatus::UNSUCEED
        : RequestStatus::SUCCEED)
        , current_time_ });

    // Если текущее время превышает сутки
    // убираем первый в очереди истории запросов
    if (current_time_ > min_in_day_) {
        requests_.pop_back();
    }
}
