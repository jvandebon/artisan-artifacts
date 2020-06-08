SRC=main.cpp
RUN_ARGS=-p /workspace/apps/inputs/spam_filter/
OUTPUTS=outputs.txt
FLAGS= 

main: $(SRC) $(shell artisan --root)/rose/repo/cpp/artisan.hpp
	g++ $(FLAGS) -o main $(SRC) -I $(shell artisan --root)/rose/repo/cpp

run:
	./main $(RUN_ARGS)

clean:
	rm -rf main $(OUTPUTS)

