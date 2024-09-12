Chat

Este es un proyecto de chat de cliente-servidor en C++ que permite a múltiples usuarios comunicarse entre ellos a través de un chat general, envío de mensajes privados y esperemos lograr las ”salas" o "cuartos" de chat. La implementación utiliza programación concurrente con hilos para manejar múltiples conexiones de clientes al servidor. El programa está implementado con un protocolo. 

El Chat implementa las siguientes funcionalidades con palabras claves: (para acceder a ellas escribir solamente las palabras claves en la terminal)
1. Imprimir una lista de Usuarios con su respectivo estado: /USERS
2. Cambiar el estado de un Usuario: /STATUS
3. Enviar un mensaje privado: /TEXT
4. Crear un cuarto: /NEW_ROOM
5. Invitar usuarios a un cuarto: /INVITE (solo se puede invitar a uno a la vez)
6. Entrar a un cuarto: /JOIN_ROOM

Pasos para compilar el proyecto una vez descargado:  
Situarse en carpeta build y seguir los comandos en terminal

cmake ..

cmake --build .

./server

./client
