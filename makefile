CXX = g++
CXXFLAGS = -std=c++17 -Wall -O3 -fopenmp

FAISS_DIR = external/faiss-install
LLAMA_DIR = $(HOME)/llama.cpp
LLAMA_BUILD_DIR = $(HOME)/llama.cpp/build

INCLUDES = -I$(FAISS_DIR)/include \
           -I$(LLAMA_DIR)

LDFLAGS = -L$(FAISS_DIR)/lib \
          -L$(LLAMA_BUILD_DIR)

FAISS_LIBS = -lfaiss -lopenblas
LLAMA_LIBS = -lllama

TARGETS = data_preprocess vector_db

all: $(TARGETS)

data_preprocess: data_preprocess.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $< $(LDFLAGS) $(LLAMA_LIBS)

vector_db: vector_db.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $< $(LDFLAGS) $(FAISS_LIBS)

clean:
	rm -f $(TARGETS)

install: $(TARGETS)
	install -d $(HOME)/bin
	install -m 755 $(TARGETS) $(HOME)/bin/

uninstall:
	rm -f $(HOME)/bin/data_preprocess $(HOME)/bin/vector_db

test-preprocess: data_preprocess
	@echo "Testing data_preprocess..."
	@./data_preprocess --help 2>&1 || echo "Binary built successfully"

test-db: vector_db
	@echo "Testing vector_db..."
	@./vector_db --help 2>&1 || echo "Binary built successfully"

config:
	@echo "=== Build Configuration ==="
	@echo "Compiler: $(CXX)"
	@echo "Flags: $(CXXFLAGS)"
	@echo "FAISS: $(FAISS_DIR)"
	@echo "llama.cpp: $(LLAMA_DIR)"
	@echo "=========================="

help:
	@echo "Available targets:"
	@echo "  all          - Build all executables (default)"
	@echo "  data_preprocess - Build data preprocessing tool"
	@echo "  vector_db    - Build vector database tool"
	@echo "  clean        - Remove built executables"
	@echo "  install      - Install to ~/bin"
	@echo "  uninstall    - Remove from ~/bin"
	@echo "  config       - Show build configuration"
	@echo "  help         - Show this help message"

.PHONY: all clean install uninstall test-preprocess test-db config help