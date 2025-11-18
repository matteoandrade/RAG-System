#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <set>
#include <nlohmann/json.hpp>
#include <faiss/IndexFlat.h>

using json = nlohmann::json;

struct Document {
    int id;
    std::string text;
    std::vector<float> embedding;
};

struct SearchResult {
    int doc_id;
    float distance;
    std::string text;
};

class VectorDB {
private:
    std::vector<Document> documents;
    faiss::IndexFlatL2* index;
    int dim;
    
public:
    VectorDB() : index(nullptr), dim(768) {}
    
    ~VectorDB() {
        if (index) {
            delete index;
        }
    }
    
    void load(const std::string& filepath) {
        std::ifstream file(filepath);
        json j;
        file >> j;
        documents.clear();
        documents.reserve(j.size());
        
        for (const auto& item : j) {
            Document doc;
            doc.id = item["id"];
            doc.text = item["text"];
            
            for (const auto& val : item["embedding"]) {
                doc.embedding.push_back(val);
            }
            
            documents.push_back(doc);
        }

        if (!documents.empty()) {
            dim = documents[0].embedding.size();
        }
    }
    
    void build_index() {
        index = new faiss::IndexFlatL2(dim);
        std::vector<float> embeddings_matrix;
        embeddings_matrix.reserve(documents.size() * dim);
        
        for (const auto& doc : documents) {
            embeddings_matrix.insert(embeddings_matrix.end(), doc.embedding.begin(), doc.embedding.end());
        }
        
        index->add(documents.size(), embeddings_matrix.data());
    }
    
    std::vector<SearchResult> search(const std::vector<float>& query, int k, bool deduplicate = true) {
        std::vector<float> distances(k);
        std::vector<int64_t> indices(k);

        index->search(1, query.data(), k, distances.data(), indices.data());
        
        std::vector<SearchResult> results;
        std::set<int> seen_ids;
        
        for (int i = 0; i < k; ++i) {
            int64_t idx = indices[i];
            
            if (idx < 0 || idx >= documents.size()) {
                continue;
            }
            
            int doc_id = documents[idx].id;
            
            if (deduplicate && seen_ids.count(doc_id) > 0) {
                continue;
            }
            
            SearchResult result;
            result.doc_id = doc_id;
            result.distance = distances[i];
            result.text = documents[idx].text;
            
            results.push_back(result);
            seen_ids.insert(doc_id);
        }
        
        return results;
    }
    
    const Document& get_document(size_t index) const {
        return documents[index];
    }
};

void self_search(VectorDB& db, int test_doc_index) {    
    const Document& test_doc = db.get_document(test_doc_index);
    auto results = db.search(test_doc.embedding, 5, false);
    
    std::cout << "\nTop 5 results:" << std::endl;
    for (size_t i = 0; i < results.size(); ++i) {
        std::cout << "  " << (i + 1) << ". Doc ID: " << results[i].doc_id << ", Distance: " << results[i].distance << std::endl;
    }
}

int main(int argc, char* argv[]) {
    std::string input_file = "../preprocessed_documents.json";
    
    VectorDB db;
    db.load(input_file);
    db.build_index();

    int index = 42; //std::stoi(argv[1]);
    self_search(db, index);
    return 1;
}