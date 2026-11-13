# sec_guard

Herramienta de monitoreo básico de recursos en Linux.

## Respuestas a las preguntas

### Etapa 1. Organización del entorno y revisión de archivos

**1. ¿Por qué los permisos adecuados para un directorio pueden
ser distintos de los necesarios para un archivo de datos?**

En un directorio, *r* permite listar su contenido, *w* permite crear
o borrar archivos dentro y *x* permite entrar y acceder a sus
archivos. En un archivo de datos, *x* significa ejecutarlo
como programa, algo que nunca hacemos con ellos.

Por eso asignamos 750 a los directorios y 640 a los datos:
el directorio necesita *x* para poder usarse; el archivo de
datos no.

**2. ¿Qué diferencia existe entre el archivo original y su
enlace duro?**

En cuanto a datos, ninguna. Ambos son nombres que apuntan al
mismo inodo, por lo que comparten contenido,
permisos, propietario, tamaño y marcas de tiempo.

Lo comprobamos de dos formas. Al renombrar *prueba_doc.txt* a
*prueba_doc_original.txt*, *hardlink.txt* siguió funcionando.
Al agregar una línea al original, el tamaño de ambos pasó de
116 a 143 bytes y la línea apareció también en *hardlink.txt*.

El campo *Links: 2* indica que hay dos nombres para el mismo
inodo. Los datos solo se borran del disco cuando se elimina
el último nombre.

**3. ¿Cuál es la utilidad de establecer enlaces simbólicos en
el desarrollo de un sistema?**

Permiten acceder a un archivo o directorio desde otra
ubicación sin duplicarlo, y cambiar a dónde apunta el enlace
sin modificar los programas que lo usan.

A diferencia del enlace duro, el simbólico es un archivo
aparte que solo guarda la ruta del destino.
Por eso mide 14 bytes, la longitud de "prueba_doc.txt".
Su desventaja es que depende del nombre: al renombrar el
original, *simbolico_doc.txt* quedó roto.

**4. ¿Qué información de los metadatos permite reconocer
cambios en un archivo?**

El tamaño y las marcas de tiempo:

- *Size*: pasó de 116 a 143 bytes al agregar una línea.
- *Modify*: cambia cuando se modifica el contenido.
- *Change*: cambia cuando se modifican los metadatos (nombre,
  permisos, número de enlaces) o el contenido. Antes del
  renombrado ya era posterior a *Modify*, porque al crear
  *hardlink.txt* el contador de enlaces subió a 2.
- *Access*: registra la última lectura.
- *Birth*: no cambia; indica cuándo se creó el archivo.

El inodo, en cambio, permite reconocer que se trata del mismo
archivo aunque cambie su nombre o su contenido.

Evidencias: *evidencias/permissions.txt*,
*evidencias/link_record.txt* y
*evidencias/arbol_directorios.txt*.


## Política de permisos

### Criterio general

Aplicamos el principio de menor privilegio;
cada elemento recibe solo los permisos que necesita para cumplir
su función. Decidimos que únicamente nosotros como propietarios
podamos modificar el proyecto, que el grupo pueda consultarlo y que
el resto de los usuarios no tenga acceso.

Mantuvimos la lectura para el grupo porque permite que un
revisor consulte el trabajo sin poder alterarlo.
Retiramos todo acceso a otros usuarios porque trabajamos en
un servidor compartido y los registros de auditoría que
procesaremos más adelante (secure.log, net_traffic.log, sshd_config) no
deberían quedar expuestos.

Permisos: r (4) = lectura, w (2) = escritura, x (1) = ejecución

### 1. Directorios: 750 (rwxr-x---)

En un directorio:

- r permite listar los nombres de su contenido.
- w permite crear, borrar o renombrar archivos dentro.
- x permite entrar al directorio y acceder a sus archivos.

Propietario (rwx = 7): necesitamos crear y borrar archivos.

Grupo (r-x = 5): le permitimos entrar y listar, pero no
crear ni borrar. Además conservamos x porque si no, el grupo
no podría abrir ningún archivo.

Otros (--- = 0): no pueden ni entrar ni listar.

### 2. Datos y fuentes C (.log, .txt, .c): 640 (rw-r-----)

Propietario (rw- = 6): nuestros scripts y el programa en C
se ejecutan con nuestra cuenta, y necesitan leer las
capturas y escribir los archivos preparados y el informe.

Grupo (r-- = 4): solo consulta. Así protegemos la integridad
de las capturas originales y de los resultados.

Otros (--- = 0): sin acceso.

No asignamos x porque estos archivos no son programas.
Los .c los lee gcc para compilarlos; los .log y .txt los leen
los scripts y el analizador. Darles x no aporta nada y
permitiría intentar ejecutarlos por error.

### 3. Scripts shell y binarios (.sh, sec_engine): 750 (rwxr-x---)

Propietario (rwx = 7): necesitamos editarlos y ejecutarlos.

Grupo (r-x = 5): puede leerlos y ejecutarlos, pero no
modificarlos. Si alguien más pudiera editarlos, podría cambiar
las instrucciones que obtienen o procesan las mediciones, y el
informe mostraría datos incorrectos.

Otros (--- = 0): sin acceso.

A los scripts sí les asignamos x con chmod 750 después de
crearlos.

### 4. Máscara de creación: umask 027

Linux crea los archivos con 666 y los directorios con 777,
y después les quita los permisos indicados por la máscara.
Elegimos 027 porque:

- 0 -> el propietario conserva todos sus permisos.
- 2 -> le quitamos la escritura al grupo.
- 7 -> les quitamos todos los permisos a otros.

Así que con esta máscara, todo lo que generen nuestros scripts nace
con los permisos correctos, sin aplicar chmod archivo por
archivo. Como los archivos nunca nacen con x, los scripts
son los únicos a los que les vamos a aplicar chmod de forma manual.

La evidencia de los permisos aplicados está en
*evidencias/permissions.txt*, que genera *etapa1_entorno.sh*.



### Etapa 2. Desempaquetado, auditoría y captura de mediciones

**1. ¿Qué transformación necesita la salida de cada utilería para
convertirse en un registro del archivo normalizado?**

De *df -P* tomamos la segunda línea y la quinta columna con *awk*,
y quitamos el símbolo % con *tr -d*. De *ps* y de *find* contamos
las líneas con *wc -l*. En *capturar.sh*, cada valor se guarda en
*telemetria_raw.log* con fecha, hora y categoría. Después,
*normalizar.sh* conserva solo la categoría y el valor, y agrega al
inicio la etiqueta y la cantidad de registros.

**2. ¿Por qué un valor cero puede ser válido y no debe confundirse
con una captura fallida?**

Porque cero es una medición real: PERMISOS = 0 significa que ningún
archivo tiene escritura para el grupo u otros, que es lo esperado
con nuestra política. Una captura fallida significa que no sabemos
el valor. Si la sustituyéramos por cero, el informe afirmaría algo
que no medimos. Por eso, con *set -o pipefail*, si un comando de la
tubería falla registramos *ERROR*; *normalizar.sh* descarta esas
líneas y avisa cuántas fueron.

**3. ¿Cómo comprobarán que la cantidad del encabezado corresponde a
los registros conservados?**

Después de escribir *mediciones_sec.txt*, contamos las líneas desde
la tercera con *tail -n +3 | wc -l* y las comparamos con la cantidad
declarada. Si no coinciden, el script termina con error.

**4. ¿Por qué el orden de las instrucciones dentro del flujo de sed
es estrictamente determinante al habilitar directivas y eliminar
comentarios?**

Porque *sed* aplica las expresiones en orden a cada línea, y las
directivas que queremos habilitar (*#Port 22* y *#HostKey*) son
comentarios. Si primero borráramos las líneas que empiezan con #,
desaparecerían antes de poder habilitarlas. Por eso primero quitamos
el # y cambiamos el puerto a 8080, y después borramos los
comentarios restantes y las líneas vacías.

**5. ¿Qué estrategia se empleó para aislar el puerto sin limitar
obtener correctamente los cuatro octetos de la IP?**

El puerto va pegado a la dirección, después del último punto. Con
*sed* reemplazamos por un espacio solo el punto seguido de dígitos
que termina la dirección, es decir, que va seguido de un espacio o
del final de la línea. Los puntos internos de la IP van seguidos de
otro número y otro punto, así que no se modifican. No contamos
cuatro octetos porque algunas conexiones tienen destino IPv6
(*fd00::2.80*); con el último punto, la regla funciona en ambos
casos.

Además, validamos la integridad del paquete *.tar.gz* con *gzip -t*
(compresión) y *tar -tzf* (estructura) antes de extraerlo, y
comprobamos que contenga al menos 3 archivos.
