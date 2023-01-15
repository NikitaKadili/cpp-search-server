#include "document.h"

using namespace std;

// Структура документа для поискового сервера
Document::Document() = default;
Document::Document(int id, double relevance, int rating) 
    : id(id)
    , relevance(relevance)
    , rating(rating) {
}

// Перегрузка оператора вывода документа
ostream& operator<<(ostream& os, const Document& doc) {
    os << "{ ";
    os << "document_id = " << doc.id;
    os << ", relevance = " << doc.relevance;
    os << ", rating = " << doc.rating;
    os << " }";
    return os;
}
