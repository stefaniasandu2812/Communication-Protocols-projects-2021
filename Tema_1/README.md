## Sandu Stefania-Cristina 324CA
## Tema1 - Pcom


Programul contine implementarea procesului de dirijare pentru un router in fisierul sursa - **router.c** si se bazeaza pe urmatoarea logica:

* router-ul primeste pachetul si extrage header-ele ethernet, ip, icmp si arp continute
* daca pachetul este **IP packet** , atunci verific daca exista header-ul **ICMP** si daca este destinat router-ului meu
* daca este destinat router-ului meu si este de tip **ICMP ECHO REQUEST** (type 8), trimit un pachet **ICMP ECHO REPLY** (type 0) catre sursa folosind functia din schelet: `send_icmp()` si arunc pachetul
* daca pachetul nu este destinat router-ului meu, atunci verific ttl-ul (ttl <= 1 -> trimit un pachet ICMP TIME EXCEEDED (type 11) catre sursa), daca checksum-ul este corect (fac drop daca nu), apo caut adresa next-hop-ului in tabela de rutare folosind adresa de destinatie a pachetului; daca nu exista, se trimite un mesaj ICMP HOST UNREACHABLE (type 3) si drop packet
* daca se gaseste in tabela de rutare, atunci se reduce ttl-ul si se updateaza checksum-ul
* se determina interfata si adresa MAC a next-hop-ului pentru a updata pachetul si a-l trimite; daca adresa MAC a acestuia nu este cunoscuta local, adica nu se afla in tabela ARP a router-ului, se trimite un **ARP REQUEST** ca broadcast pentru aflarea acesteia, pe interfata destinatie si se pune o copie a pachetului in coada
* pentru protocolul ARP, se verifica tipul pachetului si daca acesta este destinat router-ului meu
* daca pachetul este de tip **ARP REQUEST**, se updateaza header-ul ethernet pentru a trimite un **ARP REPLY** cu adresa MAC a router-ului inapoi la sursa
* daca pachetul primit este de tip **ARP REPLY**, daca nu exista entry-ul in tabela ARP, atunci se insereaza, iar daca acesta exista, scot pachetele din coada (bagate in urma unui **ARP REQ**) si le forwardez catre best route 

Functiile implementate sunt :
* `parsing_table` - pentru parsarea tabelei de rutare, folosind fucnti din skel, `inet_addr`; citeste cate o linie de max 50 caractere si o parseaza
* `get_best_route` - algoritmul folosit pentru a cauta in tabela de rutare este binary search, care are complexitatea de O(log n) si presupune ca vectorul sa fie deja sortat; pentru sortare am optat pentru quicksort, folosind functia `qsort`, care se ajuta de functia `comparator` implementata mai sus in cod (se compara prefixul si masca)
* `get_arp_entry` returneaza o structura de tip arp_entry daca se gaseste addr MAC corespunzatoare adresei ip data ca param in tabela ARP