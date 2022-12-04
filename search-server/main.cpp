#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <numeric>

//#include "search_server.h"

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double INACCURACY_VALUE = 1e-6; // Accuracy of rating

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

struct Document {
    int id;
    double relevance;
    int rating;
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status,
        const vector<int>& ratings) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    vector<Document> FindTopDocuments(const string& raw_query,
        const DocumentStatus& doc_status) const {
        return FindTopDocuments(raw_query, [doc_status](int document_id, DocumentStatus status, int rating) { return status == doc_status; });
    }

    template <typename DocStatFunc>
    vector<Document> FindTopDocuments(const string& raw_query,
        DocStatFunc doc_stat_func) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, doc_stat_func);

        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                if (abs(lhs.relevance - rhs.relevance) < INACCURACY_VALUE) {
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

    int GetDocumentCount() const {
        return static_cast<int>(documents_.size());
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query,
        int document_id) const {
        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }
        return { matched_words, documents_.at(document_id).status };
    }

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }

        return accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return { text, is_minus, IsStopWord(text) };
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                }
                else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }

    // Existence required
    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template <typename DocStatFunc>
    vector<Document> FindAllDocuments(const Query& query, DocStatFunc doc_stat_func) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                if (doc_stat_func(document_id, documents_.at(document_id).status, documents_.at(document_id).rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back(
                { document_id, relevance, documents_.at(document_id).rating });
        }
        return matched_documents;
    }
};

template <typename T>
ostream& operator<<(ostream& os, const vector<T>& container) {
    os << "["s;
    bool is_first = true;
    for (const T& element : container) {
        if (!is_first) {
            os << ", "s;
        }
        is_first = false;
        os << element;
    }
    os << "]"s;

    return os;
}

template <typename T>
ostream& operator<<(ostream& os, const set<T>& container) {
    os << "{"s;
    bool is_first = true;
    for (const T& element : container) {
        if (!is_first) {
            os << ", "s;
        }
        is_first = false;
        os << element;
    }
    os << "}"s;

    return os;
}

template <typename T, typename U>
ostream& operator<<(ostream& os, const map<T, U>& container) {
    os << "{"s;
    bool is_first = true;
    for (const auto& [key, value] : container) {
        if (!is_first) {
            os << ", "s;
        }
        is_first = false;
        os << key << ": " << value;
    }
    os << "}"s;

    return os;
}

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
    const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cout << boolalpha;
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cout << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
    const string& hint) {
    if (!value) {
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

// -------- Начало модульных тестов поисковой системы ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(),
            "Stop words must be excluded from documents"s);
    }
}

/*
Разместите код остальных тестов здесь
*/

// Тест проверяет, что документы, содержащие минус-слова, исключаются из результатов поиска
void TestExcludeDocumentsWithMinusWords() {
    const vector<int> doc_id = { 23, 25 };
    const vector<string> content = {"wolf in the underground big grey"s, "big grey parrot found"s};
    const vector<vector<int>> ratings = { { 1, 2, 3 }, { 3, 4, 5 } };

    SearchServer server;
    server.SetStopWords("in the"s);
    server.AddDocument(doc_id.at(0), content.at(0), DocumentStatus::ACTUAL, ratings.at(0));
    server.AddDocument(doc_id.at(1), content.at(1), DocumentStatus::ACTUAL, ratings.at(1));

    const auto result = server.FindTopDocuments("big grey -wolf"s);
    ASSERT_EQUAL_HINT(result.size(), 1, 
        "Documents with minus-words must be excluded from result vector"s);
    ASSERT_EQUAL(result.at(0).id, 25);
}

// Тест проверяет соответствие документов поисковому запросу
void TestResultOutputWithMinusWords() {
    const int doc_id = 25;
    const string content = "white elaphant in the big city"s;
    const vector<int> ratings = { 1, 2, 3 };

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto [result, status] = server.MatchDocument("big white mouse lost"s, 25);
        ASSERT(!result.empty());
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto [result_vector, status] = server.MatchDocument("big white mouse lost -elaphant"s, 25);
        ASSERT_HINT(result_vector.empty(),
            "Document containing minus-words must return empty vector<string>"s);
    }
}

// Тест на правильность сортировки результата по релевонтности
void TestRelevanceSortingAndRatingCount() {
    const vector<int> doc_id = { 23, 25, 26 };
    const vector<string> content = { "wolf in the underground big grey"s, "big yellow parrot found"s, "small grey wolf seen"s};
    const vector<vector<int>> ratings = { { 1, 2, 3 }, { 3, 4, 5 }, { 6, 7, 8, 9 } };

    SearchServer server;
    server.SetStopWords("in the"s);
    server.AddDocument(doc_id.at(0), content.at(0), DocumentStatus::ACTUAL, ratings.at(0));
    server.AddDocument(doc_id.at(1), content.at(1), DocumentStatus::ACTUAL, ratings.at(1));
    server.AddDocument(doc_id.at(2), content.at(2), DocumentStatus::ACTUAL, ratings.at(2));

    const auto result = server.FindTopDocuments("big grey wolf"s);
    ASSERT_EQUAL(result.size(), 3);
    ASSERT_EQUAL_HINT(result.at(0).relevance > result.at(1).relevance, result.at(1).relevance > result.at(2).relevance,
        "Documents must be sorted by descending relevance"s);

    ASSERT_EQUAL(result.at(0).rating, 2);
    ASSERT_EQUAL(result.at(1).rating, 7);
    ASSERT_EQUAL(result.at(2).rating, 4);
}

// Тест фильтрации результатов поиска по предикату, заданному пользователем, и по заданному статусу
void TestFiltrationOfFoundDocumentsByStatusAndPredicate() {
    const vector<int> doc_id = { 23, 25, 26 };
    const vector<string> content = { "wolf in the underground big grey"s, "big yellow parrot found"s, "big grey wolf seen"s };
    const vector<vector<int>> ratings = { { 1, 2, 3 }, { 3, 4, 5 }, { 6, 7, 8, 9 } };

    SearchServer server;
    server.SetStopWords("in the"s);
    server.AddDocument(doc_id.at(0), content.at(0), DocumentStatus::ACTUAL, ratings.at(0));
    server.AddDocument(doc_id.at(1), content.at(1), DocumentStatus::IRRELEVANT, ratings.at(1));
    server.AddDocument(doc_id.at(2), content.at(2), DocumentStatus::BANNED, ratings.at(2));

    {
        const auto result = server.FindTopDocuments("big grey wolf"s, DocumentStatus::IRRELEVANT);
        ASSERT_EQUAL_HINT(result.size(), 1,
            "Must return only Documents with status DocumentStatus::IRRELEVANT"s);
        ASSERT_EQUAL_HINT(result.at(0).id, 25,
            "Must return only Documents with status DocumentStatus::IRRELEVANT"s);
    }

    {
        const auto result = server.FindTopDocuments("big grey wolf"s,
            [](int document_id, const DocumentStatus& document_status, int document_rating) {
                return document_id >= 25;
            });
        ASSERT_EQUAL_HINT(result.size(), 2,
            "Must return only Documents with ID more then 24"s);
        ASSERT_EQUAL(result.at(0).id, 26);
        ASSERT_EQUAL(result.at(1).id, 25);
    }
}

bool AreDoublesEquel(double d1, double d2) {
    if (abs(d1 - d2) > INACCURACY_VALUE) {
        return false;
    }
    return true;
}

// Тест правильности подсчёта релевантности
void TestRelevanceCount() {
    const vector<int> doc_id = { 23, 25, 26 };
    const vector<string> content = { "white cat modern collar"s, "furry cat furry tail"s, "handsome dog expressive eyes"s };
    const vector<vector<int>> ratings = { { 1, 2, 3 }, { 3, 4, 5 }, { 6, 7, 8, 9 } };

    SearchServer server;
    server.SetStopWords("in the"s);
    server.AddDocument(doc_id.at(0), content.at(0), DocumentStatus::ACTUAL, ratings.at(0));
    server.AddDocument(doc_id.at(1), content.at(1), DocumentStatus::ACTUAL, ratings.at(1));
    server.AddDocument(doc_id.at(2), content.at(2), DocumentStatus::ACTUAL, ratings.at(2));

    const auto result = server.FindTopDocuments("furry handsome cat"s);
    ASSERT_EQUAL(result.size(), 3);
    ASSERT_HINT(AreDoublesEquel(result.at(0).relevance, 0.650672), 
        "Relevance of this document counted wrong. Must be: 0.650672"s);
    ASSERT_HINT(AreDoublesEquel(result.at(1).relevance, 0.274653),
        "Relevance of this document counted wrong. Must be: 0.274653"s);
    ASSERT_HINT(AreDoublesEquel(result.at(2).relevance, 0.101366),
        "Relevance of this document counted wrong. Must be: 0.101366"s);
}

#define RUN_TEST(func) func()

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    // Не забудьте вызывать остальные тесты здесь
    RUN_TEST(TestExcludeDocumentsWithMinusWords);
    RUN_TEST(TestResultOutputWithMinusWords);
    RUN_TEST(TestRelevanceSortingAndRatingCount);
    RUN_TEST(TestFiltrationOfFoundDocumentsByStatusAndPredicate);
    RUN_TEST(TestRelevanceCount);
}

// --------- Окончание модульных тестов поисковой системы -----------

int main() {
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;
}