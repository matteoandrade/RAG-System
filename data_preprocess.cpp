#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <nlohmann/json.hpp>
#include "llama.h"

using json = nlohmann::json;

struct Document {
    int id;
    std::string text;
    std::vector<float> embedding;
};

std::vector<Document> load_documents(const std::string& filepath) {
    std::ifstream file(filepath);
    json j;
    file >> j;
    std::vector<Document> documents;
    documents.reserve(j.size());
    
    for (const auto& item : j) {
        Document doc;
        doc.id = item["id"];
        doc.text = item["text"];
        documents.push_back(doc);
    }
    return documents;
}

std::vector<Document> encode_documents(std::vector<Document>& documents, const std::string& model_path) {
    llama_backend_init();
    llama_model_params model_params = llama_model_default_params();
    llama_model* model = llama_load_model_from_file(model_path.c_str(), model_params);

    llama_context_params ctx_params = llama_context_default_params();
    ctx_params.embeddings = true;
    llama_context* ctx = llama_new_context_with_model(model, ctx_params);
    
    int n_embd = llama_n_embd(model);

    int successful_encodings = 0;
    int zero_embeddings = 0;
    
    for (size_t i = 0; i < documents.size(); ++i) {
        if ((i + 1) % 100 == 0) {
            std::cout << "Processed " << (i + 1) << "/" << documents.size() << " (success: " << successful_encodings << ")" << std::endl;
        }
        
        std::vector<llama_token> tokens(1024);
        const llama_vocab* vocab = llama_model_get_vocab(model);
        int n_tokens = llama_tokenize(vocab, documents[i].text.c_str(), documents[i].text.size(), tokens.data(), tokens.size(), true, false);

        tokens.resize(n_tokens);

        llama_batch batch = llama_batch_init(n_tokens, 0, 1);

        for (int t = 0; t < n_tokens; t++) {
            batch.token[t] = tokens[t];
            batch.pos[t] = t;
            batch.n_seq_id[t] = 1;
            batch.seq_id[t][0] = 0;
            batch.logits[t] = false;
        }

        batch.n_tokens = n_tokens;

        if (llama_encode(ctx, batch) != 0) {
            llama_batch_free(batch);
            continue;
        }

        const float* embd = llama_get_embeddings_seq(ctx, 0);

        if (!embd) {
            llama_batch_free(batch);
            continue;
        }

        documents[i].embedding.assign(embd, embd + n_embd);

        float sum = 0.0;
        for (int j = 0; j < n_embd; j++) {
            sum += std::abs(documents[i].embedding[j]);
            if (sum != 0.0) {
                break;
            }
        }

        if (sum < 0.0001) {
            zero_embeddings++;
        } else {
            successful_encodings++;
        }

        llama_batch_free(batch);
    }
    
    llama_free(ctx);
    llama_free_model(model);
    llama_backend_free();
    
    return documents;
}


void save_preprocessed_data(const std::vector<Document>& documents, const std::string& output_filepath) {
    json j = json::array();
    
    for (const auto& doc : documents) {
        json doc_json;
        doc_json["id"] = doc.id;
        doc_json["text"] = doc.text;
        doc_json["embedding"] = doc.embedding;
        j.push_back(doc_json);
    }
    
    std::ofstream file(output_filepath);
    file << j.dump(2);
    file.close();
}

int main(int argc, char* argv[]) {
    std::string input_file = "../documents.json";
    std::string output_file = "../preprocessed_documents.json";
    std::string model_path = "../bge-base-en-v1.5-f32.gguf";

    std::vector<Document> documents = load_documents(input_file);
    documents = encode_documents(documents, model_path);
    save_preprocessed_data(documents, output_file);
    return 1;
}