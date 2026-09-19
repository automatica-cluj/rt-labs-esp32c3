# Preempțiune cu priorități fixe

Citiți înainte: [Taskuri și stări](tasks-and-states.md).

## 1. Problema

Mai multe taskuri sunt gata de execuție și există un singur procesor. Cine
rulează? Un sistem de operare de uz general răspunde cu reguli complicate,
gândite pentru echitate și debit mediu. Într-un sistem de timp real avem nevoie
de altceva: o regulă atât de simplă încât să putem *calcula dinainte* cât
așteaptă fiecare task în cel mai rău caz.

## 2. Ideea

Fiecare task primește la proiectare o **prioritate** care nu se mai schimbă.
Planificatorul urmează o singură regulă:

> Rulează întotdeauna taskul gata de execuție cu prioritatea cea mai mare.

Dacă un task mai prioritar devine gata în timp ce rulează altul, acesta din
urmă este oprit pe loc: **preempțiune** (preemption). Taskul oprit nu pierde
nimic; își continuă calculul când procesorul redevine liber.

Trei taskuri cu câte 100 ms de calcul, L (prioritate mică) eliberat la 0 ms, M
(medie) la 40 ms și H (mare) la 80 ms. O coloană înseamnă 10 ms:

```
t (ms)   0         100       200       300
         |         |         |         |
         LLLLMMMMHHHHHHHHHHMMMMMMLLLLLL
             ^   ^
             |   H îl preemptează pe M
             M îl preemptează pe L
```

H nu simte prezența celorlalte două. M este întârziat doar de H. L plătește
pentru amândouă. Aceasta este toată planificarea cu priorități fixe: un task
este întârziat numai de taskurile *mai prioritare* decât el.

## 3. Teorie

**Planificare preemptivă cu priorități fixe** (fixed-priority preemptive
scheduling): prioritățile se stabilesc la proiectare, iar în orice moment
rulează taskul gata de execuție cel mai prioritar. Este planificarea oferită de
aproape toate sistemele de operare de timp real.

**Timp de răspuns** (response time): timpul de la eliberarea unui job până la
terminarea lui. Are două componente:

$$
R_i = C_i + I_i
$$

unde $C_i$ este timpul de calcul propriu, iar $I_i$ este **interferența**
(interference): timpul în care procesorul a fost ocupat de taskuri mai
prioritare, între eliberarea și terminarea jobului.

Proprietăți:

- Taskul cel mai prioritar are $I = 0$, deci $R = C$, indiferent de ce fac
  celelalte. Garanția lui nu depinde de restul sistemului.
- Interferența unui task depinde doar de taskurile *mai* prioritare. Adăugarea
  unui task mai puțin prioritar nu strică nicio garanție existentă.
- Prioritatea exprimă **urgența**, nu importanța. Cum se aleg prioritățile se
  vede în [Rate Monotonic](../3-analysis/rate-monotonic.md).
- Dacă taskul mai prioritar este **periodic**, el întrerupe un calcul lung de
  mai multe ori. Interferența crește cu durata calculului întârziat:
  $I_i = \lceil R_i / T_j \rceil \cdot C_j$ pentru un singur task mai prioritar
  cu perioada $T_j$. De aici pornește
  [Analiza timpului de răspuns](../3-analysis/response-time-analysis.md).

**Felii de timp** (time slicing): regula de mai sus nu spune ce se întâmplă
între taskuri cu *aceeași* prioritate. De obicei planificatorul le rotește la
fiecare **tick**, de exemplu o dată pe milisecundă: fiecare primește o felie,
apoi trece la coada celor cu prioritatea lui. Consecințe:

- toate avansează „deodată”, deci toate termină târziu, aproape în același
  moment;
- timpul *mediu* de răspuns este mai mare decât dacă ar rula unul după altul,
  iar primul care termină, termină mult mai târziu;
- fiecare rotire este o comutare de context, deci un cost în plus.

Feliile de timp sunt utile pentru echitate între activități fără termene. Într-o
analiză de timp real se preferă priorități distincte, pentru că ordinea de
execuție devine cunoscută.

**Planificare nepreemptivă**: taskul curent rulează până se blochează singur.
Este mai simplă și evită unele probleme de acces la date comune, dar un task
urgent poate aștepta cât cel mai lung calcul din sistem.

## 4. Exemplu

**Preempțiune.** Pentru diagrama din secțiunea 2, calculat de mână:

| Task | Eliberat la | Rulează în intervalele | Termină la | $C$ | $I$ | $R$ |
|---|---|---|---|---|---|---|
| H | 80 ms | 80–180 | 180 ms | 100 | 0 | 100 ms |
| M | 40 ms | 40–80, 180–240 | 240 ms | 100 | 100 (H) | 200 ms |
| L | 0 ms | 0–40, 240–300 | 300 ms | 100 | 200 (M și H) | 300 ms |

**Priorități egale.** Trei taskuri A, B, C, de aceeași prioritate, eliberate
împreună, cu câte 40 ms de calcul:

| Mod de execuție | A termină la | B | C | Timp mediu de răspuns |
|---|---|---|---|---|
| felii de 1 ms, pe rând | 118 ms | 119 ms | 120 ms | 119 ms |
| unul după altul | 40 ms | 80 ms | 120 ms | 80 ms |

Ultimul termină la fel în ambele cazuri, dar cu felii de timp *toate* termină
târziu.

**Un task periodic peste un calcul lung.** H, prioritar, calculează 4 ms la
fiecare 20 ms. L, mai puțin prioritar, are un calcul de 120 ms. În fiecare
fereastră de 20 ms, L primește 16 ms. După șapte ferestre, la 140 ms, L a
adunat 112 ms; în a opta, H rulează între 140 și 144 ms, iar L își termină
ultimele 8 ms la **152 ms**. Verificare cu formula: $120 + \lceil 152/20 \rceil
\cdot 4 = 120 + 8 \cdot 4 = 152$.

```
t (ms)   0                   20                  40
         |                   |                   |
         HHHHLLLLLLLLLLLLLLLLHHHHLLLLLLLLLLLLLLLLHHHH...
```

## 5. Greșeli frecvente

- **„Prioritate mare înseamnă mai mult timp de procesor.”** Prioritatea nu
  împarte procesorul în proporții. Taskul mai prioritar primește *tot* ce cere,
  iar restul primesc ce rămâne.
- **„Prioritatea se alege după importanță.”** Un task important, dar cu termen
  lung, pus deasupra unuia cu termen scurt îl poate face pe acesta să rateze.
  Prioritatea se alege după urgență.
- **„Timpul de răspuns este timpul de calcul.”** Sunt egale doar pentru taskul
  cel mai prioritar. Pentru toate celelalte se adaugă interferența.

## 6. Întrebări de verificare

1. În exemplul cu L, M și H, cum se schimbă cei trei timpi de răspuns dacă H
   este eliberat la 20 ms în loc de 80 ms? Dar dacă H calculează 200 ms?
2. Trei taskuri cu aceeași prioritate și durate diferite, 10, 40 și 100 ms,
   pornesc împreună. Estimați când termină fiecare cu felii de timp și comparați
   cu execuția în ordinea „cel mai scurt primul”.
3. În exemplul cu taskul periodic, ce se întâmplă cu timpul de răspuns al lui L
   dacă H calculează 10 ms la fiecare 20 ms? Dar 20 ms la fiecare 20 ms?
4. De ce adăugarea unui task nou cu prioritatea cea mai mică nu poate strica
   nicio garanție dată taskurilor existente? Este aceasta adevărat și dacă noul
   task împarte date cu ele?

Pentru mai mult: G. Buttazzo, *Hard Real-Time Computing Systems*, cap. 2 și 4;
A. Burns, A. Wellings, *Real-Time Systems and Programming Languages*, cap. 11.

Laborator asociat: [Lab 2](../../lab2-scheduling/README.md).
