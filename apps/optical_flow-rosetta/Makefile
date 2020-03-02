SRC=optical_flow.cpp ./imageLib/Convert.cpp imageLib/Convolve.cpp ./imageLib/flowIO.cpp ./imageLib/Image.cpp ./imageLib/ImageIO.cpp ./imageLib/RefCntMem.cpp
RUN_ARGS=-p /workspace/apps/inputs/optical_flow/current -o out.flo
OUTPUTS=outputs.txt out.flo

main: $(SRC) $(shell artisan --root)/rose/repo/cpp/artisan.hpp
	g++ -o main $(SRC) -I $(shell artisan --root)/rose/repo/cpp

run:
	./main $(RUN_ARGS)

clean:
	rm -rf main $(OUTPUTS)


