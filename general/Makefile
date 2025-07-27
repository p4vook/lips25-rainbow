# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++23 -O2

# Executables
PROGRAMS = perebor drawer

# Maximum value for graph generation
MAX_N = 4

# Generate list of numbers from 2 to MAX_N
NUMBERS = $(shell seq 2 $(MAX_N))

# Output files
MAX_FILES = $(patsubst %,results/%max.txt,$(NUMBERS))
GRAPH_FILES = $(patsubst %,results/%maxgraphs_all.txt,$(NUMBERS))

# Default target
all: $(PROGRAMS) $(MAX_FILES) $(GRAPH_FILES)

# Compile programs
%: %.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

# Generate max files
results/%max.txt: perebor | results
	echo $* | ./perebor > $@

# Generate graph files
results/%maxgraphs_all.txt: results/%max.txt drawer | results
	./drawer < $< > $@

# Create results directory if it doesn't exist
results:
	mkdir -p results

# Clean build artifacts
clean:
	rm -f $(PROGRAMS) *.gcda *.gcno

# Clean only results
clean-results:

# Phony targets
.PHONY: all clean

# Prevent intermediate files from being deleted
.PRECIOUS: results/%max.txt
