#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdio>
#include <string>
#include <nlohmann/json.hpp>
#include <faiss/IndexFlat.h>
#include "encode.cpp"
#include "llm_generation.cpp"

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

class RAGSystem {
private:
    std::vector<Document> documents;
    std::map<int, std::string> doc_id_to_text;
    faiss::IndexFlatL2* index;
    QueryEncoder* encoder;
    LLMGenerator* generator;
    int dim;
    
public:
    RAGSystem(const std::string& bge_model_path, const std::string& llm_model_path, const std::string& preprocessed_docs_path) {
        dim = 768;
        index = nullptr;
        encoder = new QueryEncoder();
        generator = new LLMGenerator();
        llama_backend_init();
        load_documents(preprocessed_docs_path);
        build_index();
    }
    
    ~RAGSystem() {
        if (index) delete index;
        if (encoder) delete encoder;
        if (generator) delete generator;
        llama_backend_free();
    }
    
    void load_documents(const std::string& filepath) {
        std::ifstream file(filepath);
        json j;
        file >> j;
        documents.clear();
        documents.reserve(j.size());
        
        for (const auto & item : j) {
            Document doc;
            doc.id = item["id"];
            doc.text = item["text"];
            
            for (const auto & val : item["embedding"]) {
                doc.embedding.push_back(val);
            }
            
            documents.push_back(doc);
            doc_id_to_text[doc.id] = doc.text;
        }
    }
    
    void build_index() {        
        dim = documents[0].embedding.size();
        index = new faiss::IndexFlatL2(dim);

        std::vector<float> embeddings_matrix;
        embeddings_matrix.reserve(documents.size() * dim);
        
        for (const auto& doc : documents) {
            embeddings_matrix.insert(embeddings_matrix.end(), doc.embedding.begin(), doc.embedding.end());
        }
        
        index->add(documents.size(), embeddings_matrix.data());
    }
    
    std::vector<SearchResult> retrieve_documents(const std::vector<float>& query_embedding, int k = 3) {
        std::vector<float> distances(k);
        std::vector<int64_t> indices(k);
        
        index->search(1, query_embedding.data(), k, distances.data(), indices.data());
        std::vector<SearchResult> retrieved_docs;

        for (int i = 0; i < k; i++) {
            SearchResult r;
            r.doc_id = indices[i];
            r.distance = distances[i];
            r.text = documents[r.doc_id].text;
            retrieved_docs.push_back(r);
        }
        
        return retrieved_docs;
    }
    
    std::string augment_prompt(const std::string& query, const std::vector<SearchResult>& retrieved_docs) {
        std::string augmented;
        augmented += query;

        for (int i = 0; i < retrieved_docs.size(); ++i) {
            augmented += " Top documents:" + retrieved_docs[i].text;
        }
        return augmented;
    }

    std::string answer_query(const std::string& query, int k = 3) {
        std::cout << "\n[Query]: " << query.c_str() << "\n";

        std::vector<float> query_embedding = encoder->encode(query);
        std::vector<SearchResult> retrieved_docs = retrieve_documents(query_embedding, k);
        
        std::cout << "Retrieved " << retrieved_docs.size() << " documents\n";

        for (size_t i = 0; i < retrieved_docs.size(); i++) {
            SearchResult preview = retrieved_docs[i];
            std::cout << "  Doc " << preview.doc_id << ": " << preview.text.substr(0, 60) << "... with distance: " << preview.distance << "\n";
        }
        
        std::string augmented_prompt = augment_prompt(query, retrieved_docs);

        std::cout << "Generating response...\n";

        std::string response = generator->generate(augmented_prompt, 256);
        return response;
    }
    
    void run_interactive() {
        std::cout << "\n========================================\n";
        std::cout << "RAG System - Interactive Mode\n";
        std::cout << "Type 'exit' or 'quit' to stop\n";
        std::cout << "========================================\n\n";
        
        std::string query;
        while (true) {
            std::cout << "\n> Enter your question: ";
            
            if (!std::getline(std::cin, query)) break;
            
            if (query.empty()) continue;
            
            if (query == "exit" || query == "quit") {
                std::cout << "Goodbye!\n";
                break;
            }
            
            std::string answer = answer_query(query);
            std::cout << "\n[Answer]" << answer.c_str() << "\n";
        }
    }
};

int main(int argc, char** argv) {
    std::string bge_model = "../bge-base-en-v1.5-f32.gguf";
    std::string llm_model = "../tinyllama-1.1b-chat-v0.3.Q4_K_M.gguf";
    std::string docs_file = "../preprocessed_documents.json";
    
    ggml_backend_load_all();
    llama_log_set([](enum ggml_log_level level, const char * text, void * user_data) {}, nullptr);

    RAGSystem rag(bge_model, llm_model, docs_file);
    rag.run_interactive();
    
    return 0;
}