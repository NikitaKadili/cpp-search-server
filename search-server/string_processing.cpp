#include "string_processing.h"

using namespace std;

// ��������� ����� � string_view, ���������� ������ ����
vector<string_view> SplitIntoWords(string_view text) {
    vector<string_view> result;

    // ������� � �������� ��� ������� � ������ ������
    size_t pos = text.find_first_not_of(' ');
    text.remove_prefix(pos == text.npos ? text.length() : pos);

    while (!text.empty()) {
        // ������� ������, ��������� �� ���������� ������� �����, ��������� ����� � result
        pos = text.find_first_of(' ');
        result.push_back(text.substr(0, (pos == text.npos ? text.length() : pos)));

        // ������� ������� ����� � ��������� �� ��� ������ (��� �������)
        text.remove_prefix(pos == text.npos ? text.length() : ++pos);

        // ������� � ������� ���������� �� ������ ������� (��� �������)
        pos = text.find_first_not_of(' ');
        text.remove_prefix(pos == text.npos ? text.length() : pos);
    }

    return result;
}
