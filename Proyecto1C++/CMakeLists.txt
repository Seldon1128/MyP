# Nombre del proyecto
project(ChatApp)

# Establecer el estándar de C++ (C++11 en este caso)
set(CMAKE_CXX_STANDARD 11)

#Biblioteca JSON
include_directories(${PROJECT_SOURCE_DIR}/json-develop/include)

# Agregar ejecutables
add_executable(server server.cpp Room.cpp)
add_executable(client client.cpp)

