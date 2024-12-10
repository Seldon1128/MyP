Chat

Este es un proyecto de chat de cliente-servidor en C++ que permite a múltiples usuarios comunicarse entre ellos a través de un chat general, envío de mensajes privados y esperemos lograr las ”salas" o "cuartos" de chat. La implementación utiliza programación concurrente con hilos para manejar múltiples conexiones de clientes al servidor. El programa está implementado con un protocolo. 

El Chat implementa las siguientes funcionalidades con palabras claves: (para acceder a ellas escribir solamente las palabras claves en la terminal)
1. Imprimir una lista de Usuarios con su respectivo estado: /USERS
2. Cambiar el estado de un Usuario: /STATUS
3. Enviar un mensaje privado: /TEXT
4. Crear un cuarto: /NEW_ROOM
5. Invitar usuarios a un cuarto: /INVITE 
6. Entrar a un cuarto: /JOIN_ROOM
7. Imprimir una lista de Usuarios de un cuarto con su respectivo estado: /ROOM_USERS
8. Enviar mensaje a un cuarto: /ROOM_TEXT
9. Abandonar un cuarto: /LEAVE_ROOM
10. Desconecta a un usuario del chat: /DISCONNECT

Cosas que no implemente:
1. Al un usuario desconectarse del servidor no envio mensajes a los cuartos en los que estaba

Para compilar el proyecto se usa clang, verificar la instalacion del compilador
1. Abrir en terminal y ejecutar: clang++ --version
2. Si no aparece información sobre clang, instala las herramientas de Xcode ejecutando: xcode-select --install

Pasos para compilar el proyecto una vez descargado:  

Antes que nada modificar la dirección ip y el puerto del server y el cliente:

Para modificar la dirección ip del server modificar linea 515 y el puerto en la 512 del server
Para modificar la dirección ip del cliente modificar linea 165 y el puerto en la 164


Situarse en carpeta Proyecto1C++ y seguir los comandos en terminal

Limpiar el directorio de compilacion:
1. rm -rf build
2. mkdir build
3. cd build

Situarse en el directorio de compilacion build:

4. Configurar CMake especificando el compilador: cmake -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ ..
5. Construir: cmake --build .

6. ./server

En otras terminales ubicarse en el directorio build y correr el cliente (pueden ser varias terminales o de diferente compu)
7. /client
