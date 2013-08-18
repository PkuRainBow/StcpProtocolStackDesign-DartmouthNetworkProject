weather: data_cli.o city.o query.o
	gcc -o weather data_cli.o city.o query.o

data_cli.o: data_cli.c tcp.h
	gcc -c -o data_cli.o data_cli.c

city.o: city.c tcp.h
	gcc -c -o city.o city.c
query.o: query.c tcp.h
	gcc -c -o query.o query.c

clean: 
	rm -rf *.o weather
