# Упрощенная модель поискового сервера
## Краткое описание
Учебный проект упрощенной модели поискового сервера.
Поисковый сервер хранит заданные пользователем стоп-слова, документы и их id.
Поиск осуществляется как по конкретному документу, так и по всем документам, выводя результаты, отсортированные по заданным пользователем параметрам.
Вывод поиска может осуществляться как в упорядоченном, так и в параллельном исполнении, ранжирование по релевантности документов осуществляется на основе TF-IDF.
## Использование
Пример использования:
```
SearchServer search_server(stop_words); // stop_words - контейнер стоп слов. Также, можно передавать строкой
// Ниже в цикле представлен процесс добавления документов в поисковый сервер
for (int i = 0; i < static_cast<int>(documents.size()); ++i) {
    search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, { 1, 2, 3 });
}

cout << "ACTUAL by default:"s << endl;
// Вывод актуальных документов (по умолчанию), соответствующих запросу "curly nasty cat"
for (const Document& document : search_server.FindTopDocuments("curly nasty cat"s)) {
    PrintDocument(document);
}

cout << "BANNED:"s << endl;
// Вывод запрещенных документов (передачей желаемого статуса в качестве аргумента), соответствующих запросу "curly nasty cat"
for (const Document& document : search_server.FindTopDocuments(execution::seq, "curly nasty cat"s, DocumentStatus::BANNED)) {
    PrintDocument(document);
}

cout << "Even ids:"s << endl;
// Вывод документов с четным id (передачей лямбда-функции в качестве аргумента), соответствующих запросу "curly nasty cat"
for (const Document& document : search_server.FindTopDocuments(execution::par, "curly nasty cat"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
    PrintDocument(document);
}

```
Также, методом `MatchResult MatchDocument(std::string_view query, int id)` возможно сверять содержание документа под номером id с содержимым текста query. Метод вернет картеж, состоящий из: вектора совпавших слов, статуса документа. 
## Системные требования
* C++17 (STL)
* g++ с поддержкой 17-го стандарта (также, возможно применения иных компиляторов C++ с поддержкой необходимого стандарта)
## Планы по доработке
* Добавить сериализацию документов сервера (с применением Protobuf)
* Внедрить ввод-вывод данных в формате JSON
## Стек
* C++17
