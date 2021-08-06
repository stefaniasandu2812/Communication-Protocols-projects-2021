## Sandu Stefania-Cristina 324CA
## Pcom - Tema 3 -> Client Web

Sursele folosite in cadrul rezolvarii temei au fost preluate
din laboratorul 10 si modificate astfel incat sa se plieze
pe ideea temei, iar parson-ul folosit este cel pus la 
dispozitie in enunt - https://github.com/kgabis/parson .

Principala sursa devine, astfel, `client.c `, unde sunt
tratate toate comenzile care pot fi primite de la input.

Protocolul folosit este **HTTP** care asigura interactiunea
cu server-ul prin **cereri** trimise de catre client si
**raspunsuri** primite (de la server) de catre client.
Tipurile de request-uri au fost implementate in `requests.c`
-> GET (interogare de resurse), POST (adaugare de resure),
DELETE (stergere de resurse). Astfel, fiecare functie de 
request presupune completarea campurilor cu datele
corespunzatoare date ca parametrii acestora. Cererea de tip
POST difera de celelalte doua prin completarea campurilor
de `content_type` si `body_data` in care se retin
payload-urile trimise.

Sursa `client.c` :

* se primeste o comanda ca input si se citeste
* daca se primeste `exit` se iese din bucla si implicit
din program
* daca se primeste `register` se citesc username-ul si
parola (reister info), sunt parsate datele care trebuie
trimise catre server si se face un POST request catre
acesta
* pentru comanda `login`, in cazul in care un client care
este deja logat incearca sa se logheze, se returneaza
un mesaj cu _Already logged in!_; daca se logheaza pentru
prima oara, se procedeaza ca la `register`, iar in plus
se retine cookie-ul returnat de server prin functia 
`get_cookies(...)` implementata in `helpers.c`, fiind modul
prin care clientul demonstreaza in urmatoarele comenzi ca
este autentificat
* daca se primeste `enter_library` se realizeaza un GET
request in care se trimite si cookie-ul si se primeste
acel token (*JWT*) care se retine pentru a demonstra, in
urmatoarele comenzi, accesul la biblioteca
* comanda `get_books` presupune trimiterea unui GET req
cu cookie si *JWT* pentru a primi, ca raspuns, o lista cu
cartile adaugate de catre utilizator + detalii despre
acestea
* daca se primeste `get_book`, se citeste id-ul cartii
despre care vrem informatii, construim url-ul potrivit
pentru cartea respectiva (lipim id-ul la url-ul standard)
si facem un GET request cu *JWT*
* pentru `add_book` am creat o structura `struct book` in
`helpers.h` care contine campurile cu informatiile care
trebuie adaugate despre o carte in server; aceste campuri
se citesc rand pe rand, sunt parsate si apoi se face un
POST request cu *JWT*
* comanda `del_book` presupune citirea id-ului coresp.
cartii pe care clientul doreste sa o stearga, se
construieste url-ul potrivit (prin concatenare) si se
realizeaza un DELETE request catre server cu *JWT*
* pentru `logout` se trimite un GET request cu cookie
si variabilele cookie si *JWT* sunt setate pe NULL
* daca s eprimeste orice altceva ca input, se returneaza
mesajul _Bad command, try again!_ si se astepta o comanda
valida

De precizat ca raspunsurile primite de la server sunt
printate si ele ca output.

Pentru a reliza o implementare defensiva am folosit DIE-ul
pus la dispozitie in scheletele de lab si am verificat
alocarile de memorie.

Deoarece implementarea temei este in C, am ales folosirea
parson-ului pus la dispozitie in enunt, fiind si usor de
inteles cum se foloseste, din documentatia de pe github.
(Serialization). L-am folsoit doar pentru a crea
JSON values cu informatii simple, necesare pentru fiecare
comanda, iar string-ul obtinut in urma parsarii se trimite
catre server.
