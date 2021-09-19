compile: 
	$(info Compiling code...)
	@g++-11 src/server.cpp -o server 
	@g++-11 src/client.cpp -o client	
	$(info Compilation complete)

runserver:
	$(info Running server...)
	@./server

newclient:
	$(info Creating new client...)
	@./client

clean:
	$(info Cleaning up...)
	@rm ./client
	@rm ./server