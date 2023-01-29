#include "remove_duplicates.h"

#include <algorithm>
#include <set>
#include <string>
#include <vector>

void RemoveDuplicates(SearchServer& search_server) {
    using namespace std;

    map<set<string>, int> words_to_ids; // Храним уже проверенные слова и id
    vector<int> ids_to_remove; // id дубликатов
    for (int id : search_server) {
        set<string> words;
        const auto word_frequencies = search_server.GetWordFrequencies(id);
        for (const auto& [word, _] : word_frequencies) {
            words.insert(word);
        }
        if (words_to_ids.count(words) != 0) {
            cout << "Found duplicate document id "s << id << endl;
            ids_to_remove.push_back(id);
        }
        else {
            words_to_ids[words] = id;
        }
    }

    for (int id : ids_to_remove) {
        search_server.RemoveDocument(id);
    }
}
