all:ipc_server.o ipc_client.o ipc_client_fork.o hash_process.o
	ar cr hash_process.a hash_process.o
	gcc -pthread -o ipc_server ipc_server.o hash_process.a
	gcc -o ipc_client ipc_client.o
	gcc -o ipc_client_fork ipc_client_fork.o
ipc_server.o:ipc_server.c ipc_hash.h
	gcc -c -Wall -g ipc_server.c
ipc_client.o:ipc_client.c ipc_hash.h
	gcc -c -Wall -g ipc_client.c
ipc_client_fork.o:
	gcc -c -Wall -g ipc_client_fork.c
hash_process.o:hash_process.c ipc_hash.h
	gcc -c -Wall -g hash_process.c
clean:
	rm ipc_server ipc_server.o ipc_client ipc_client.o hash_process.o 
	rm ipc_client_fork ipc_client_fork.o 
	rm hash_process.a
