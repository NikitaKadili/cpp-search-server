#include "remove_duplicates.h"

#include <algorithm>
#include <set>
#include <string>
#include <vector>

void RemoveDuplicates(SearchServer& search_server) {
    using namespace std;

    map<set<string_view>, int> words_to_ids; // ������ ��� ����������� ����� � id
    vector<int> ids_to_remove; // id ����������
    for (int id : search_server) {
        set<string_view> words;
        const auto word_frequencies = search_server.GetWordFrequencies(id);
        for (const auto& word_to_freq : word_frequencies) {
            words.insert(word_to_freq.first);
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
