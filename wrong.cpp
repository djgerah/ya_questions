#include <algorithm> 
#include <cmath>
#include <iostream>
#include <map>
#include <numeric>
#include <set>
#include <string>
#include <vector>
 
const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON = 1e-6;
using namespace std;
 
struct Document {
    int id;
    double relevance;
    int rating;
};
 
enum class DocumentStatus {
    ACTUAL, IRRELEVANT, BANNED, REMOVED 
    };
 
string ReadLine() {
    string str;
    getline(cin, str);
    return str;
}
 
int ReadLineWithNumber() {
    int number_of_lines;
    cin >> number_of_lines;
    ReadLine();
    return number_of_lines;
}
 
vector<int> Ratings() {
      int number_of_ratings = 0;
      vector<int> document_ratings = {};
      cin >> number_of_ratings ;
      if (number_of_ratings == 0) {
        return document_ratings;
      }
      for (int j=0; j<number_of_ratings; ++j) {
        int rating; 
        cin >> rating;
        document_ratings.push_back(rating);
      }
      ReadLine();
      return document_ratings;
}
 
vector<string> SplitIntoWords (const string& text) {
    vector<string> result{};
    if (text.empty()) {
        return result;
    }
    size_t size_s = text.size();
    string word = ""s;
    for (size_t i =0; i < size_s; ++i) {
        if (text[i] != ' ') {
            word += text[i];
        }
        else if (!word.empty()) {
            result.push_back(word); 
            word = ""s;
        }
    }
    if (!word.empty()) {
        result.push_back(word);
    }
    return result;
}
 
class SearchServer {
    public:
 
    void SetStopWords(const string& text) {
    for (const string& word : SplitIntoWords(text)) {
        stop_words_.insert(word);
    }
}
 
void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int> ratings) {
    vector<string> document_v = SplitIntoWordsNoStop(document);
                    if (!document_v.empty()) {
    ++document_count_;
    documents_status_.insert({document_id, status});
    int average_rating = ComputeAverageRating(ratings); 
    document_ratings_[document_id] = average_rating;
    for(const string& word : document_v) {
        if (stop_words_.find(word) == stop_words_.end()) {
            double word_freq = 1.0/document_v.size();
            word_to_document_freqs_[word][document_id] += word_freq;
        }
    }
    }
}
 
int GetDocumentCount() const {
    return document_count_;
}
 
tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
    vector<string> result{};
    Query query = ParseQuery(raw_query);
    for (const string& word: query.plus_words) {
        if (word_to_document_freqs_.find(word) == word_to_document_freqs_.end() 
        || word_to_document_freqs_.at(word).find(document_id) == word_to_document_freqs_.at(word).end()) {
            continue;
        }
        result.push_back(word);
    }
 
    for (const string& word: query.minus_words) {
        if (word_to_document_freqs_.find(word) == word_to_document_freqs_.end()
        || word_to_document_freqs_.at(word).find(document_id) == word_to_document_freqs_.at(word).end()) {
            continue;
        }
        result.clear();
    }
 
    return {result, documents_status_.at(document_id)};
}
 
// Возвращает самые релевантные документы в виде вектора пар {id, релевантность}
template <typename KeyMapper>
vector<Document> FindTopDocuments(const string& raw_query, KeyMapper key_mapper) const {
    vector<Document> result = FindAllDocuments(ParseQuery(raw_query), key_mapper);
    sort(result.begin(), result.end(), 
    [] (const Document& left, const Document& right) {
        return (abs(left.relevance - right.relevance) <= EPSILON && left.rating > right.rating) 
        || left.relevance > right.relevance;});
    if (result.size() > MAX_RESULT_DOCUMENT_COUNT) {
        result.resize(MAX_RESULT_DOCUMENT_COUNT);
    } 
    return result; 
}
 
vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus doc_status = DocumentStatus::ACTUAL) const {
    return FindTopDocuments (raw_query, [doc_status] (int documen_id, DocumentStatus status, int rating) {
        return status == doc_status;});
}
 
private:
 
struct Query {
    set<string> minus_words;
    set<string> plus_words;
};
 
set<string> stop_words_;
map<string, map<int, double>> word_to_document_freqs_;
int document_count_ = 0;
map<int, int> document_ratings_; // pair doc_id - rating
map<int, DocumentStatus> documents_status_;
 
vector<string> SplitIntoWordsNoStop(const string& text) const {
    vector<string> result{}; 
    for (const string& word : SplitIntoWords(text)) {
        if (stop_words_.count(word) == 0 ) {
            result.push_back(word); 
        }
    }
    return result; 
}
 
Query ParseQuery(const string& text) const {
    vector<string> result_v = SplitIntoWordsNoStop(text);
    if (result_v.empty()) {
        return {};
    }
    Query result;
    for(const string& word : result_v) {
       if (word[0] == '-' && word.size() > 1) {
        result.minus_words.insert(word.substr(1));
       }
       else {
        result.plus_words.insert(word);
       }   
    }
    return result;
}
 
static int ComputeAverageRating(const vector<int>& ratings) {
   return (ratings.size() == 0 ? 0 : accumulate(ratings.begin(), ratings.end(),0) 
   / static_cast<int>(ratings.size()));
} 
 
template <typename KeyMapper>
vector<Document> FindAllDocuments(const Query& query, KeyMapper key_mapper) const {
    
    map<int, double> docs_relevance;
    for (const string& word: query.plus_words) {
        if (word_to_document_freqs_.find(word) == word_to_document_freqs_.end()) {
            continue;
        }
        for (const auto& [document_id, TF] : word_to_document_freqs_.at(word)) {
            if (key_mapper(document_id, documents_status_.at(document_id), document_ratings_.at(document_id))) {
                double IDF = log(static_cast<double>(document_count_) 
                / static_cast<double>(word_to_document_freqs_.at(word).size()) );
                docs_relevance[document_id] += TF * IDF; 
            }
        }
    }
 
    for (const string& word: query.minus_words) {
        if (word_to_document_freqs_.find(word) == word_to_document_freqs_.end()) {
            continue;
        }
        for (const auto& [document_id, TF] : word_to_document_freqs_.at(word)) {
            docs_relevance.erase(document_id); 
        }
    }
 
    vector<Document> result;
    for (const auto& [document_id, relevance] : docs_relevance) {
        result.push_back({document_id, relevance, document_ratings_.at(document_id)});
    }
 
    return result;
}
 
};
 
void PrintDocument(const Document& document) {
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating
         << " }"s << endl;
}
int main() 
{
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);
    search_server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    search_server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});
    cout << "ACTUAL by default:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s)) {
        PrintDocument(document);
    }
    cout << "BANNED:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED)) {
        PrintDocument(document);
    }
    cout << "Even ids:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
        PrintDocument(document);
    }
    return 0;
}