# Compiler settings
CXX = g++
CXXFLAGS = -std=c++17 -Wall -O3 -fopenmp

# Paths - adjust these to match your setup
FAISS_DIR = external/faiss-install
LLAMA_DIR = $(HOME)/llama.cpp
LLAMA_BUILD_DIR = $(HOME)/llama.cpp/build

# Include directories
INCLUDES = -I$(FAISS_DIR)/include \
           -I$(LLAMA_DIR)

# Library directories
LDFLAGS = -L$(FAISS_DIR)/lib \
          -L$(LLAMA_BUILD_DIR)

# Libraries to link
FAISS_LIBS = -lfaiss -lopenblas
LLAMA_LIBS = -lllama

# Targets
TARGETS = data_preprocess vector_db main

# Default target
all: $(TARGETS)

# Build data_preprocess (Part 1)
data_preprocess: data_preprocess.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $< $(LDFLAGS) $(LLAMA_LIBS)

# Build vector_db (Part 1)
vector_db: vector_db.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $< $(LDFLAGS) $(FAISS_LIBS)

# Build main RAG system (Part 2)
main: main.cpp encode.cpp llm_generation.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ main.cpp $(LDFLAGS) $(FAISS_LIBS) $(LLAMA_LIBS)

# Clean build artifacts
clean:
	rm -f $(TARGETS)

# Download TinyLlama model
download-llm:
	@echo "Downloading TinyLlama model..."
	wget https://huggingface.co/TheBloke/TinyLlama-1.1B-Chat-v0.3-GGUF/resolve/main/tinyllama-1.1b-chat-v0.3.Q4_K_M.gguf

# Test Part 1 components
test-part1: data_preprocess vector_db
	@echo "Testing Part 1 - Vector Database"
	@./data_preprocess documents.json preprocessed_documents.json bge-base-en-v1.5-f32.gguf
	@./vector_db preprocessed_documents.json

# Test Part 2 RAG system
test-part2: main
	@echo "Testing Part 2 - RAG System"
	@./main preprocessed_documents.json bge-base-en-v1.5-f32.gguf tinyllama-1.1b-chat-v0.3.Q4_K_M.gguf

# Show configuration
config:
	@echo "=== Build Configuration ==="
	@echo "Compiler: $(CXX)"
	@echo "Flags: $(CXXFLAGS)"
	@echo "FAISS: $(FAISS_DIR)"
	@echo "llama.cpp: $(LLAMA_DIR)"
	@echo "=========================="

# Help target
help:
	@echo "Available targets:"
	@echo "  all          - Build all executables (default)"
	@echo "  data_preprocess - Build data preprocessing tool (Part 1)"
	@echo "  vector_db    - Build vector database tool (Part 1)"
	@echo "  main         - Build RAG system (Part 2)"
	@echo "  clean        - Remove built executables"
	@echo "  download-llm - Download TinyLlama model"
	@echo "  test-part1   - Run Part 1 tests"
	@echo "  test-part2   - Run Part 2 RAG system"
	@echo "  config       - Show build configuration"
	@echo "  help         - Show this help message"

.PHONY: all clean download-llm test-part1 test-part2 config help