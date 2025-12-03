ya hijos de la vende super 8

para entender la historia de fnaf

nah enserio

se tienen 5 codigos voy a explicar paso a paso que hacen

-proceso.h

Es el primer codigo importante y es el que da estructura tanto a los procesos como 
a las paginas

-proceso.cpp

Es la ejecucion de las clases del codigo .h del mismo nombre
en este codigo lo que se hace es tomar los datos que ingresa el usuario 
para calcular la cantidad de paginas
una vez hecho el calculo se procede a crear las paginas e inicializarlas vacias

-memoria.h 

Es la estructura de los marcos de pagina y lo mas importante
es el inicio de la memoria ram y swap como tal
estos funcionan como vectores, al mismo tienen las funciones para cambiar los 
procesos de una memoria a la otra

-memoria.cpp

Es practicamente lo mas importante de la tarea
aqui se ocupa todo lo pedido y antes hecho, toda la estructura de memoria.h se 
ocupa aqui, en este codigo ocurren 4 cosas importantes
1) primero asigna un proceso a la ram, si esta llena se asigna a la swap
2) libera ram o swap dependiendo de donde se encuentre el proceso que se mato (se tiro al metro XD)
3) se busca el proceso, si esta en ram, lo devuelve sin mas, si esta en la swap se elimina del tipo fifo en ram y se trae a esta desde swap
4) imprime el estado diciendo cuanto es lo que queda de swap, ram y los procesos creados o eliminados

-main.cpp

Es finalmente el que maneja el tiempo de ejecucion de los procesos y simulador
en este codigo esta el bucle principal que cada 2 segundos crea un proceso
pasado lo s 30 empieza a cada 5 segundos liberar un proceso y acceder a memoria
todo esto mientras sigue creando procesos
cada proceso que se crea esta definido entre un tamaño minimo de 1mb para que sea rapido
pero no puede sobrepasar mas de la mitad del tamaño de la memoria fisica

y ya eso es todo
