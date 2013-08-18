all: son/son sip/sip client/im_client server/im_server

common/pkt.o: common/pkt.c common/pkt.h common/constants.h
	gcc -Wall -pedantic -std=gnu99 -g -c common/pkt.c -o common/pkt.o
topology/topology.o: topology/topology.c 
	gcc -Wall -pedantic -std=gnu99 -g -c topology/topology.c -o topology/topology.o
son/neighbortable.o: son/neighbortable.c
	gcc -Wall -pedantic -std=gnu99 -g -c son/neighbortable.c -o son/neighbortable.o
son/son: topology/topology.o common/pkt.o son/neighbortable.o son/son.c 
	gcc -Wall -pedantic -std=gnu99 -g -pthread son/son.c topology/topology.o common/pkt.o son/neighbortable.o -o son/son
sip/nbrcosttable.o: sip/nbrcosttable.c
	gcc -Wall -pedantic -std=gnu99 -g -c sip/nbrcosttable.c -o sip/nbrcosttable.o
sip/dvtable.o: sip/dvtable.c
	gcc -Wall -pedantic -std=gnu99 -g -c sip/dvtable.c -o sip/dvtable.o
sip/routingtable.o: sip/routingtable.c
	gcc -Wall -pedantic -std=gnu99 -g -c sip/routingtable.c -o sip/routingtable.o
sip/sip: common/pkt.o common/seg.o topology/topology.o sip/nbrcosttable.o sip/dvtable.o sip/routingtable.o sip/sip.c 
	gcc -Wall -pedantic -std=gnu99 -g -pthread sip/nbrcosttable.o sip/dvtable.o sip/routingtable.o common/pkt.o common/seg.o topology/topology.o sip/sip.c -o sip/sip 
client/im_client: client/im_client.c common/seg.o client/stcp_client.o topology/topology.o 
	gcc -Wall -pedantic -std=gnu99 -g -pthread client/im_client.c common/seg.o client/stcp_client.o topology/topology.o -o client/im_client
server/im_server: server/im_server.c server/im_server.h common/seg.o server/stcp_server.o topology/topology.o 
	gcc -Wall -pedantic -std=gnu99 -g -pthread server/im_server.c common/seg.o server/stcp_server.o topology/topology.o -o server/im_server
common/seg.o: common/seg.c common/seg.h
	gcc -Wall -pedantic -std=gnu99 -g -c common/seg.c -o common/seg.o
client/stcp_client.o: client/stcp_client.c client/stcp_client.h 
	gcc -Wall -pedantic -std=gnu99 -g -c client/stcp_client.c -o client/stcp_client.o
server/stcp_server.o: server/stcp_server.c server/stcp_server.h 
	gcc -Wall -pedantic -std=gnu99 -g -c server/stcp_server.c -o server/stcp_server.o

clean:
	rm -rf common/*.o
	rm -rf topology/*.o
	rm -rf son/*.o
	rm -rf son/son
	rm -rf sip/*.o
	rm -rf sip/sip 
	rm -rf client/*.o
	rm -rf server/*.o
	rm -rf client/im_client
	rm -rf server/im_server
	rm -rf server/receivedtext.txt



