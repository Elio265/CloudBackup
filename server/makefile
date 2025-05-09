test : test.cc
	g++ -o $@ $^ -std=c++17 -lpthread -lstdc++fs -ljsoncpp -lzstd

.PTHONY : clean
clean:
	rm -rf test backdir/ packdir/ cloud.dat
