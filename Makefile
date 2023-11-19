###############
#- VARIABLES -#
###############

# Compilador y opciones de compilación
CC = gcc
CFLAGS = -Wall -Wpedantic -g

# Carpeta con las cabeceras
HEADERS_DIR = host

# Opción de compilación que indica dónde están los archivos .h
INCLUDES = -I$(HEADERS_DIR)

# Archivos de cabecera para generar dependencias
HEADERS = $(HEADERS_DIR)/host.h $(HEADERS_DIR)/getlocalips.h $(HEADERS_DIR)/getpublicip.h $(HEADERS_DIR)/loging.h

# Fuentes con las funcionalidades básicas de cliente y servidor (implementaciones de los .h)
COMMON = $(HEADERS:.h=.c)

# Emisor y receptor básicos
BASIC = basic

## Emisor básico
### Fuentes
SRC_BASIC_TRANSMITTER_SPECIFIC = $(BASIC)/emisor.c
SRC_BASIC_TRANSMITTER = $(SRC_BASIC_TRANSMITTER_SPECIFIC) $(COMMON)

### Objetos
OBJ_BASIC_TRANSMITTER = $(SRC_BASIC_TRANSMITTER:.c=.o)

### Ejecutable o archivo de salida
OUT_BASIC_TRANSMITTER = $(SRC_BASIC_TRANSMITTER_SPECIFIC:.c=)

## Receptor básico
### Fuentes
SRC_BASIC_RECEIVER_SPECIFIC = $(BASIC)/receptor.c
SRC_BASIC_RECEIVER = $(SRC_BASIC_RECEIVER_SPECIFIC) $(COMMON)

### Objetos
OBJ_BASIC_RECEIVER = $(SRC_BASIC_RECEIVER:.c=.o)

### Ejecutable o archivo de salida
OUT_BASIC_RECEIVER = $(SRC_BASIC_RECEIVER_SPECIFIC:.c=)

# Servidor y cliente de mayúsculas
MAYUS = mayus

## Servidor de mayúsculas
### Fuentes
SRC_MAYUS_SERVER_SPECIFIC = $(MAYUS)/servidorUDP.c
SRC_MAYUS_SERVER = $(SRC_MAYUS_SERVER_SPECIFIC) $(COMMON)

### Objetos
OBJ_MAYUS_SERVER = $(SRC_MAYUS_SERVER:.c=.o)

### Ejecutable o archivo de salida
OUT_MAYUS_SERVER = $(SRC_MAYUS_SERVER_SPECIFIC:.c=)

## Cliente básico
### Fuentes
SRC_MAYUS_CLIENT_SPECIFIC = $(MAYUS)/clienteUDP.c
SRC_MAYUS_CLIENT = $(SRC_MAYUS_CLIENT_SPECIFIC) $(COMMON)

### Objetos
OBJ_MAYUS_CLIENT = $(SRC_MAYUS_CLIENT:.c=.o)

### Ejecutable o archivo de salida
OUT_MAYUS_CLIENT = $(SRC_MAYUS_CLIENT_SPECIFIC:.c=)

# Listamos todos los archivos de salida
OUT = $(OUT_BASIC_TRANSMITTER) $(OUT_BASIC_RECEIVER) $(OUT_MAYUS_SERVER) $(OUT_MAYUS_CLIENT)

# # Servidor remoto al que subir los archivos relacionados con servidores
REMOTE_HOST = debian-server
REMOTE_USER = pedro
REMOTE_DIR = ~/ServidorUDP
REMOTE_SRC_DIR = $(REMOTE_DIR)/src
REMOTE_HEADERS_DIR = $(REMOTE_SRC_DIR)/host


############
#- REGLAS -#
############

# Regla por defecto: compila todos los ejecutables
all: $(OUT)

# Compila emisor y receptor básicos
basic: $(OUT_BASIC_TRANSMITTER) $(OUT_BASIC_RECEIVER)

# Compila servidor y cliente de mayúsculas
mayus: $(OUT_MAYUS_SERVER) $(OUT_MAYUS_CLIENT)

# Genera el ejecutable del servidor básico, dependencia de sus objetos.
$(OUT_BASIC_TRANSMITTER): $(OBJ_BASIC_TRANSMITTER)
	$(CC) $(CFLAGS) -o $@ $(OBJ_BASIC_TRANSMITTER)

# Genera el ejecutable del cliente básico, dependencia de sus objetos.
$(OUT_BASIC_RECEIVER): $(OBJ_BASIC_RECEIVER)
	$(CC) $(CFLAGS) -o $@ $(OBJ_BASIC_RECEIVER) 

# Genera el ejecutable del servidor de mayúsculas, dependencia de sus objetos.
$(OUT_MAYUS_SERVER): $(OBJ_MAYUS_SERVER)
	$(CC) $(CFLAGS) -o $@ $(OBJ_MAYUS_SERVER)

# Genera el ejecutable del cliente de mayúsculas, dependencia de sus objetos.
$(OUT_MAYUS_CLIENT): $(OBJ_MAYUS_CLIENT)
	$(CC) $(CFLAGS) -o $@ $(OBJ_MAYUS_CLIENT)

# Genera los ficheros objeto .o necesarios, dependencia de sus respectivos .c y todas las cabeceras.
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c -o $@ $< $(INCLUDES)

# Borra todos los resultados de la compilación (prerrequisito: cleanobj)
clean: cleanobj
	rm -f $(OUT)

# Borra todos los ficheros objeto del directorio actual y todos sus subdirectorios
cleanobj:
	find . -name "*.o" -delete

deploy:
	scp $(HEADERS) $(COMMON) $(REMOTE_USER)@$(REMOTE_HOST):$(REMOTE_HEADERS_DIR)
	scp $(SRC_BASIC_TRANSMITTER_SPECIFIC) $(SRC_MAYUS_SERVER_SPECIFIC) $(REMOTE_USER)@$(REMOTE_HOST):$(REMOTE_SRC_DIR)
	ssh $(REMOTE_USER)@$(REMOTE_HOST) make --directory=$(REMOTE_DIR)

