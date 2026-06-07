CXX      = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -I.
TARGET   = cvm
SRCS     = main.cpp

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f $(TARGET)

test: $(TARGET)
	@echo ""
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	@echo " TEST 1: hello.cvm"
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	./$(TARGET) tests/hello.cvm
	@echo ""
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	@echo " TEST 2: control.cvm"
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	./$(TARGET) tests/control.cvm
	@echo ""
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	@echo " TEST 3: functions.cvm (recursion demo)"
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	./$(TARGET) tests/functions.cvm

debug-test: $(TARGET)
	./$(TARGET) --debug tests/hello.cvm
