SRC=main.cpp
RUN_ARGS=2097152 /workspace/apps/inputs/adpredictor

main: $(SRC) $(shell artisan --root)/rose/repo/cpp/artisan.hpp
	g++ -o main $(SRC) -I $(shell artisan --root)/rose/repo/cpp

run:
	./main $(RUN_ARGS)

clean:
	rm -rf main

