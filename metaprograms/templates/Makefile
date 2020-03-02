SRC=
INC=
RUN_ARGS=
OUTPUTS=

main: $(SRC) $(shell artisan --root)/rose/repo/cpp/artisan.hpp
	g++ -o main $(SRC) -I $(shell artisan --root)/rose/repo/cpp -I $(INC)

run:
	./main $(RUN_ARGS)

clean:
	rm -rf main $(OUTPUTS)


