# Ceasuri și rezoluție

## 1. Problema

Într-un sistem de timp real, timpul este o mărime care se măsoară, la fel ca o
tensiune sau o temperatură. Orice afirmație despre sistem („taskul răspunde în
cel mult 2 ms”, „perioada este de 10 ms”) se sprijină pe un ceas. Ca orice
instrument de măsură, un ceas are limite: nu poate arăta pași oricât de mici,
nu este perfect exact, iar citirea lui durează. Cine ignoră aceste limite
ajunge să măsoare proprietățile ceasului în locul proprietăților sistemului.

## 2. Ideea

Un ceas digital este un numărător care avansează în pași. Tot ce se întâmplă
între doi pași este invizibil pentru el:

```
timp real    |----|----|----|----|----|----|----|----|
ceas         0    1    2    3    4    5    6    7    8      (un pas = rezoluția)

eveniment A       ^ la 1,1          eveniment B    ^ la 6,9
citit             1                                6
durata reală: 5,8 pași        durata măsurată: 5 pași
```

Eroarea unei durate măsurate poate ajunge la un pas întreg. De aici regula de
bază: un ceas este potrivit pentru o măsurătoare doar dacă pasul lui este mult
mai mic decât durata măsurată.

Același raționament se aplică planificatorului. Un sistem de operare de timp
real își ține evidența timpului cu un **tick**: o întrerupere periodică, de
exemplu la fiecare 1 ms. Toate întârzierile și toate termenele de așteptare se
numără în tickuri, deci planificatorul „vede” timpul doar în pași de un tick.

## 3. Teorie

**Rezoluția** (resolution) este cel mai mic pas pe care îl poate arăta un ceas.

**Acuratețea** (accuracy) arată cât de aproape este valoarea citită de timpul
adevărat. Depinde de oscilatorul din care este derivat ceasul, care are o
abatere de frecvență, de obicei de ordinul zecilor de părți pe milion. Un ceas
poate avea rezoluție foarte bună și acuratețe slabă, sau invers. Pentru durate
scurte contează rezoluția; pentru durate lungi contează acuratețea.

**Costul citirii** este timpul de procesor consumat de o citire a ceasului. O
citire nu este gratuită și nu este instantanee. Un ceas a cărui citire durează
cât fenomenul măsurat este inutil pentru acel fenomen.

Surse de timp întâlnite pe un microcontroler:

| Sursa | Rezoluție tipică | Cost | Potrivită pentru |
|---|---|---|---|
| numărătorul de tickuri | un tick (de exemplu 1 ms) | foarte mic | întârzieri și perioade ale taskurilor |
| temporizator hardware liber (free-running timer) | microsecunde | mic | latențe, timpi de răspuns |
| contor de cicluri (cycle counter) | un ciclu de procesor | aproape zero | durate foarte scurte, timpi de execuție |

**Umplerea numărătorului** (wrap). Un numărător are un număr fix de biți, deci
revine la zero după $2^n$ pași. De exemplu, un contor de cicluri pe 32 de biți
la 160 MHz se umple după $2^{32} / (160 \cdot 10^6) \approx 26{,}8$ s. Diferența
a două citiri, calculată în aritmetică fără semn pe aceeași lățime, rămâne
corectă chiar dacă între ele a avut loc o umplere, cu condiția ca durata
măsurată să fie mai scurtă decât un ciclu complet al numărătorului.

**Granularitatea tickului.** O cerere „așteaptă $n$ tickuri” înseamnă „până la
al $n$-lea tick de acum înainte”. Primul dintre ele poate veni aproape imediat,
deci durata reală a așteptării este

$$
(n - 1) \cdot t_{tick} \;<\; d \;\le\; n \cdot t_{tick}.
$$

Pentru $n = 1$, așteptarea durează între 0 și un tick. O întârziere cerută în
tickuri este deci o limită superioară, nu o durată garantată.

**Somn și așteptare activă.** Un task poate lăsa timpul să treacă în două
feluri. În **somn** taskul este blocat, procesorul este liber pentru alte
taskuri, iar trezirea se face la un tick, deci cu rezoluția tickului. În
**așteptare activă** (busy waiting) taskul ține procesorul și verifică ceasul
în buclă: rezoluția este foarte bună, dar taskurile mai puțin prioritare nu
rulează deloc în acest timp (vezi
[Înfometare și watchdog](../2-scheduling/starvation-and-watchdogs.md)).

**Efectul de sondă** (probe effect). Măsurarea modifică sistemul măsurat:
citirea ceasului, memorarea valorii și mai ales afișarea ei consumă timp de
procesor. Transmiterea unui rând de text poate dura cât sute sau mii de citiri
ale ceasului. Regula care decurge de aici: în timpul măsurătorii valorile doar
se înregistrează în memorie, iar raportarea se face după ce măsurătoarea s-a
încheiat.

## 4. Exemplu

Se măsoară o operație care durează în realitate 250 µs.

- Cu un ceas cu rezoluția de 1 ms, rezultatul este 0 sau 1 ms, după cum
  operația a prins sau nu un pas al ceasului. Eroarea relativă poate ajunge la
  300 %. Ceasul este nepotrivit.
- Cu un ceas cu rezoluția de 1 µs, eroarea este de cel mult 1 µs, adică 0,4 %.
- Dacă o citire a acestui ceas costă 2 µs, cele două citiri adaugă până la
  4 µs, adică 1,6 %. Costul citirii este acum mai mare decât eroarea de
  rezoluție, deci el limitează măsurătoarea.
- Dacă în intervalul măsurat se afișează și un mesaj care durează 500 µs,
  rezultatul devine 750 µs: se măsoară afișarea, nu operația.

Al doilea exemplu: un task cere o așteptare de un tick, cu tickul de 1 ms, la
0,9 ms după tickul precedent. Tickul următor vine după 0,1 ms, deci așteptarea
durează 0,1 ms, nu 1 ms. Cine are nevoie de *cel puțin* 1 ms trebuie să ceară
două tickuri și obține între 1 și 2 ms.

## 5. Greșeli frecvente

- **Confuzia dintre rezoluție și acuratețe.** Un ceas care afișează
  microsecunde nu este neapărat exact la microsecundă. Multe cifre nu înseamnă
  multă precizie.
- **„O așteptare de 1 ms durează 1 ms.”** Durează cel mult 1 ms și poate dura
  aproape zero. Întârzierile în tickuri sunt limite superioare.
- **„Măsurarea nu costă nimic.”** Citirea ceasului și, mai ales, afișarea
  rezultatelor fac parte din timpul măsurat dacă au loc în interiorul
  intervalului.

## 6. Întrebări de verificare

1. Un ceas are rezoluția de 1 µs, iar o citire a lui durează 3 µs. Care este
   cea mai scurtă durată pe care o puteți măsura cu o eroare sub 10 %?
2. De ce rămâne corectă diferența a două citiri ale unui numărător pe 32 de
   biți după o umplere? Ce condiție trebuie să îndeplinească durata măsurată?
3. Un dispozitiv are nevoie de o pauză de cel puțin 1 ms, iar tickul este de
   1 ms. Câte tickuri cereți și cât poate dura pauza în cel mai rău caz?
4. Când este justificată așteptarea activă, deși blochează toate taskurile mai
   puțin prioritare?

Pentru mai mult: H. Kopetz, *Real-Time Systems: Design Principles for
Distributed Embedded Applications*, ed. a 2-a, Springer, 2011, cap. 3;
G. Buttazzo, *Hard Real-Time Computing Systems*, cap. 1.

Laborator asociat: [Lab 1](../../lab1-time-latency/README.md).
