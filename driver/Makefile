obj-m += pico_driver.o

# Target 'all': Compila e carrega o driver automaticamente
all:
	# 1. Compila o módulo usando os headers do Kernel atual
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	
	# 2. Tenta remover o driver antigo se ele já estiver rodando
	# (O sinal '-' no início faz o make ignorar erro se o driver não estiver carregado)
	-sudo rmmod pico_driver
	
	# 3. Carrega o novo driver compilado
	sudo insmod pico_driver.ko
	
	@echo "--- SUCESSO: Driver compilado e carregado! ---"

# Target 'clean': Remove o driver e limpa os arquivos
clean:
	# 1. Remove o driver da memória
	-sudo rmmod pico_driver
	
	# 2. Limpa os arquivos binários gerados
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	
	@echo "--- LIMPEZA: Driver removido e arquivos limpos. ---"