#pragma once

#include <iostream>

// Структура документа для поискового сервера
struct Document {
    Document();
    Document(int id, double relevance, int rating);

    int id = 0;
    double relevance = 0.0;
    int rating = 0;
};

// Статус документа
enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

std::ostream& operator<<(std::ostream& os, const Document& doc);