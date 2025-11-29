#include <vector>
#include <string>
#include <cstdio>
#include <stdexcept>
#include "llama.h"

class QueryEncoder {
private:
    llama_model* model;
    llama_context* ctx;
    int embedding_dim;

public:
    QueryEncoder() {
        llama_model_params model_params = llama_model_default_params();
        model = llama_model_load_from_file("../bge-base-en-v1.5-f32.gguf", model_params);
        llama_context_params ctx_params = llama_context_default_params();
        
        ctx_params.no_perf = true;
        ctx_params.embeddings = true;
        ctx = llama_init_from_model(model, ctx_params);
        embedding_dim = llama_model_n_embd(model);
    }
    
    ~QueryEncoder() {
        if (ctx) llama_free(ctx);
        if (model) llama_model_free(model);
    }
    
    std::vector<float> encode(const std::string & query) {
            const llama_vocab* vocab = llama_model_get_vocab(model);
            std::vector<llama_token> tokens(llama_n_ctx(ctx));

            const int n_tokens = llama_tokenize(vocab, query.c_str(), query.size(), tokens.data(), tokens.size(), true, false);
            tokens.resize(n_tokens);
            llama_batch batch = llama_batch_init(n_tokens, 0, 1);

            for (int i = 0; i < n_tokens; i++) {
                batch.token[i] = tokens[i];
                batch.pos[i] = i;
                batch.n_seq_id[i] = 1;
                batch.seq_id[i][0] = 0;
                batch.logits[i] = false;
            }

            batch.n_tokens = n_tokens;
            llama_encode(ctx, batch);
            std::vector<float> result;

            const int n = llama_model_n_embd(model);
            const float* embd = llama_get_embeddings_seq(ctx, 0);

            if (!embd) embd = llama_get_embeddings(ctx);
            
            result.assign(embd, embd+n);
            return result;
    }
    
    int get_embedding_dim() {
        return embedding_dim;
    }
};