#include <vector>
#include <string>
#include <cstdio>
#include <stdexcept>
#include "llama.h"

class LLMGenerator {
private:
    llama_model* model;
    llama_context* ctx;
    llama_sampler* smpl;

public:
    LLMGenerator() {
        llama_model_params model_params = llama_model_default_params();
        model = llama_model_load_from_file("../tinyllama-1.1b-chat-v0.3.Q4_K_M.gguf", model_params);
        
        if (!model) {
            fprintf(stderr, "%s: error: unable to load LLM model\n", __func__);
            throw std::runtime_error("Failed to load LLM model");
        }
        
        llama_context_params ctx_params = llama_context_default_params();
        ctx_params.n_ctx = 2048;
        ctx_params.n_batch = 512;
        ctx_params.no_perf = false;
        
        ctx = llama_init_from_model(model, ctx_params);
        
        auto sparams = llama_sampler_chain_default_params();
        sparams.no_perf = false;
        smpl = llama_sampler_chain_init(sparams);
        llama_sampler_chain_add(smpl, llama_sampler_init_greedy());
    }
    
    ~LLMGenerator() {
        if (smpl) llama_sampler_free(smpl);
        if (ctx) llama_free(ctx);
        if (model) llama_model_free(model);
    }
    
    std::string generate(const std::string & prompt, int max_tokens = 256) {
        llama_context_params ctx_params = llama_context_default_params();
        ctx_params.no_perf = true;
        ctx_params.n_ctx = 2048;
        ctx_params.n_batch = 2048;
        ctx_params.no_perf = false;
        llama_context* gen_ctx = llama_init_from_model(model, ctx_params);

        const llama_vocab* vocab = llama_model_get_vocab(model);
        int n_prompt_tokens = -llama_tokenize(vocab, prompt.c_str(), prompt.size(), NULL, 0, true, true);
        std::vector<llama_token> prompt_tokens(n_prompt_tokens);
        llama_tokenize(vocab, prompt.c_str(), prompt.size(), prompt_tokens.data(), prompt_tokens.size(), true, true);

        llama_batch batch = llama_batch_get_one(prompt_tokens.data(), prompt_tokens.size());
        if (llama_model_has_encoder(model)) {
            llama_token decoder_start = llama_model_decoder_start_token(model);
            if (decoder_start == LLAMA_TOKEN_NULL) decoder_start = llama_vocab_bos(vocab);
            batch = llama_batch_get_one(&decoder_start, 1);
        }

        std::string response;
        for (int i = 0; i < max_tokens; i++) {
            if (llama_decode(gen_ctx, batch)) break;

            llama_token new_token = llama_sampler_sample(smpl, gen_ctx, -1);
            if (new_token == LLAMA_TOKEN_NULL) break;

            char piece_buf[512];
            int piece_len = llama_token_to_piece(vocab, new_token, piece_buf, sizeof(piece_buf), 0, true);
            if (piece_len < 0) break;
            std::string piece(piece_buf, piece_len);
            response += piece;

            if (new_token == llama_vocab_eos(vocab) || new_token == llama_vocab_sep(vocab)) break;
            
            batch = llama_batch_get_one(&new_token, 1);
        }

        llama_free(gen_ctx);

        while (!response.empty() && isspace((unsigned char)response.back())) response.pop_back();
        while (!response.empty() && isspace((unsigned char)response.front())) response.erase(response.begin());

        return response;
    }
};