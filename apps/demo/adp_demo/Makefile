SRC=main.cpp
RUN_ARGS=8388608 /workspace/apps/inputs/adpredictor
OUTPUTS=
FLAGS=

main: $(SRC) $(shell artisan --root)/rose/repo/cpp/artisan.hpp
	g++ $(FLAGS) -o main $(SRC) -I $(shell artisan --root)/rose/repo/cpp

run:
	./main $(RUN_ARGS)

clean:
	rm -rf main $(OUTPUTS)

