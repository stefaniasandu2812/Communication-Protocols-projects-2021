## Sandu Stefania-Cristina 324CA
## Pcom - Tema 2

Implementarea presupune realizarea unei aplicatii de tip
client-server in C++ pentru gestionarea mesajelor, a carei 
functionalitate este prezentata in urmatoarele randuri.

Am folosit 3 structuri declarate in utils.h, dintre care
2 pentru mesajele UDP si TCP, iar cealalta pentru clienti.
Mesajul primit de la UDP va contine topicul, tipul de date
si payload-ul. Mesajul primit de clientul TCP va avea
un camp pentru addr si portul clientului UDP care a
trimis mesajul si un camp de tip udp_msg, in care
se copiaza mesajul trimis, urmand ca acesta sa fie
decodificat in subscriber.cpp.

Pentru conexiunea dintre server si client se creeaza
sockets, unul pentru legatura UDP client - server, pe care
se vor primi si date, si unul pasiv pentru legatura TCP client - server,
pe care se "asculta" cererile de conexiune din partea clientilor.

Pentru server, se completeaza informatiile privind adresa
si portul pe care se asculta.

Pentru ca aplicatia server sa poata raspunda cererilor
primite de la un numar variabil de clienti, am folosit
multiplexare, care consta in crearea unui set de descriptori
de fisier(socketii udp si tcp si stdin) si folosirea functiei 
select(), care ne va spune pe care din descriptorii de
fisiere avem date.

Daca se primesc date de la tastatura si comanda
primita este "exit", atunci server-ul se inchide si se
inchid si conexiunile cu clientii (inchiderea sockets-ilor)date.

Pentru a retine datele despre clienti si topicurile la care
acestia se aboneaza, am ales sa folosesc map-uri, deja
implementate in C++, pentru ca operatiile sa se realizeze
eficient (timp de cautare O(1)), fapt vizibil in trimiterea
mesajelor catre clientii TCP. Astfel, *topics_subs* presupune
retinerea topicurilor si a unui map de clienti abonati la 
topicul respectiv, *clients* reprezinta un map al clientilor
conectati la server, identificati dupa id, iar *msg_holder*
este un map in care se retin, pentru fiecare client
cu SF = 1, mesajele trimise pentru topicul la care acesta era
abonat, in cazul in care se deconecteaza.   

Server - client flow:

* server-ul primeste pe socket-ul pasiv cereri de conectare
de la clienti TCP, le accepta si verifica daca clientul se
conecteaza pentru prima oara - se adauga socket-ul nou in
multimea cu descriptori de citire, se adauga acesta in map-ul 
clientilor server-ului si se creeaza coada pentru
client

* server-ul primeste date de la clientul UDP, mesajul transmis
fiind reprezentat printr-o structura de tip *struct udp_msg*

* verifica daca topicul exista in map-ul topics_subs 
(daca nu, se creeaza) si daca exista abonati pentru acest 
topic, iar daca da, se completeaza campurile pentru mesajul
ce urmeaza a fi transmis catre clientul TCP respectiv 
(structura de tip *struct tcp_msg_to_send*)

* daca clientul TCP este conectat, se trimite mesajul pe
socket-ul corespunzator, daca nu, se verifica daca este un
client cu SF = 1 si se adauga mesajul in coada clientului

* daca un client se reconecteaza, adica exista deja un client
cu acleasi id si are campul connected = 0, atunci se updateaza
socket-ul si este recunoscut ca fiind iar conectat,
se cauta in map-ul de retinere a mesajelor si verific daca
are in coada mesaje ce trebuie transmise si le forwardez

* daca se incearca conectarea unui nou client cu un id
al unui client deja existent si conectat, aceasta conexiune
este refuzata si inchisa

* daca se primesc date de pe unul din sockets-ii clientilor,
se receptioneaza mesajul si in functie de acesta, clientul
se deconecteaza sau se aboneaza/dezaboneaza la/de la
topicul specificat

Clientul TCP:

* clientul TCP face request pentru conectarea la server,
trimitand apoi server-ului id-ul

* mesajele pe care clientul le primeste de la tastatura sunt
trimise catre server - cand un client primeste "exit" anunta
server-ul ca s-a deconectat si cand primeste subscribe sau
unsubscribe anunta server-ul pentru a-l abona/dezabona

* cand clientul TCP primeste mesaje de la server (sub forma
unei structuri de tip *tcp_msg_to_send*), pe socketul
corespunzator conexiunii, se decodeaza si este printat in
formatul cerut

* decodarea mesajului se realizeaza cu ajutorul functiei
decode() din subscriber.cpp, care decodeaza payload-ul primit
in cadrul mesajului si il copiaza intr-un string; in cazul
unui INT sau un FLOAT, functia copiaza numarul si ii adauga
semnul daca este negativ, pentru SHORT_REAL si string doar
se copiaza numarul, respectiv string-ul 

Tratarea erorilor care pot sa apara se face cu ajutorul
lui DIE din header-ul helpers.h pus la dispozitie in cadrul
laboratorului, iar eventualele mesaje de eroare sunt printate
la stderr, realizand astfel o implementare robusta.
