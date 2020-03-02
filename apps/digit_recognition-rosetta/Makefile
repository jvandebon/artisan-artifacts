SRC=digit_recognition.cpp
RUN_ARGS=
OUTPUTS=output.txt

main: $(SRC) $(shell artisan --root)/rose/repo/cpp/artisan.hpp
	g++ -o main $(SRC) -I $(shell artisan --root)/rose/repo/cpp

run:
	./main $(RUN_ARGS)

clean:
	rm -rf main $(OUTPUTS)
