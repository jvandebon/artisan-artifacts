SRC=spam_filter.cpp
RUN_ARGS=-p /workspace/apps/inputs/spam_filter/
OUTPUTS=outputs.txt

main: $(SRC) $(shell artisan --root)/rose/repo/cpp/artisan.hpp
	g++ -o main $(SRC) -I $(shell artisan --root)/rose/repo/cpp

run:
	./main $(RUN_ARGS)

clean:
	rm -rf main $(OUTPUTS)

