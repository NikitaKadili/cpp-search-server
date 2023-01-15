#include "read_input_functions.h"

#include <iostream>

using namespace std;

// Считывание строки
string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

// Считывание числа
int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}
