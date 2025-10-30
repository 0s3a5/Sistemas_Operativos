Volvemos con la tarea 2

aqui no se como empezar porque es literal un puro codigo

priemra actualizacion 


se supone que seran n+m treahds donde n son los heroes y m los monstruos

Lo ideal es hacerlo en un arreglo bidimensional
Los datos se deben de ingresar desde un archivo
Aqui vamos a descubrir como abrir el archivo y ademas como leerlo

segunda actualizacion

logre hacer algo pero no me imprime mapa 

codigo con modificaciones mias y hecho por mi es pruebas

tareas es de gpt pero le falta mucho

*27/10/25 // 22:10

para explicar que esta sucediendo decidi mejor separar las partes importantes del codigo y explicarlas
aca unicamente voy a dejar que pruebas es la mejor version hecha y update_pruebas son cosas experimetnales 
el update es mi version+chatgpt para que limpie y arregle la basura de trabajo 

--Primero y lo mas importante se separa por funciones de threads, hasta el momento tengo 2 y son solo para 1 thread
-- son la de mover heroe y la vista del monstruo

destaco que mi codigo funciona a lo autista asi que gpt lo hizo mas entendible

*28/10/25 //00:28 update_prueba v2.0 funcional

Le pase un codigo de prueba para calcular distancias y que se active el movimiento del monstruo
la distancia esta en manhatan, que es un nombre bonito para lo clasico de siempre
el mapa sigue siendo la zona critica, esta hecho de tal forma que se bloquee para que se muevan
todo tiene mutex, encontre inutil el uso de semaforos

--creacion de codifgo Mover_monstruo

*28/10/25 00:52 prueba v2.0 funcional 

me simplifico 2 funciones que habia hecho, me junto funciones de ataque de heroe y monstruo en uno solo
cada uno quita segun corresponde
falta agregar que funcione para muchos, la funcion esta hecha para mas de 1 monstruo pero el programa solo para 1

--creacion de codigo ataque

cambios a realizar
-hacer que se cambien bien las letras en el mapa
-que todos los monstruos se pongan en alerta y se muevan
-verificar las posiciones (no se ven en el mapa las de heroes)

*29/10/25 23:20 prueba v3.0 funcional

actualizacion en el uso de threads
viendo el codigo del erick me di cuenta que me complique la vida
tengo muchas funciones y muchos threads
zona critica sigue siendo el grid tablero o mapa
ahora el heroe y monstruos se manejan como threads

cambios a realizar
-hacer que funcione con mas de un heroe
-hacer que se le pueda meter un archivo 
