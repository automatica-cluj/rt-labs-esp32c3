# Deadlock

Citiți înainte: [Excludere mutuală](mutual-exclusion.md).

---

## 1. Problema

Un task care are nevoie de două resurse în același timp ia două zăvoare. Dacă
alt task ia aceleași două zăvoare în ordine inversă, se poate ajunge în
situația în care fiecare ține unul și îl așteaptă pe celălalt. Niciunul nu mai
avansează vreodată: **deadlock** (interblocare).

Spre deosebire de o întârziere, un deadlock nu trece de la sine. Taskurile
implicate sunt blocate, deci nu consumă procesor, iar restul sistemului pare să
funcționeze normal. De aceea este greu de observat și, în varianta lui rară,
aproape imposibil de prins la testare.

## 2. Ideea

Două taskuri și două zăvoare, m1 și m2:

```
task A: ia m1, ..., ia m2, lucrează, eliberează ambele
task B: ia m2, ..., ia m1, lucrează, eliberează ambele   <- ordine inversă
```

1. A ia m1. B ia m2.
2. A cere m2, care este la B: A se blochează.
3. B cere m1, care este la A: B se blochează.

**Graful de așteptare** (wait-for graph) are câte un nod pentru fiecare task și
câte o săgeată de la cel care așteaptă la cel care ține resursa. Un ciclu în
acest graf este semnul deadlock-ului:

```
   A --așteaptă m2--> B
   ^                  |
   +---așteaptă m1----+
```

Dacă între cele două luări trece mult timp, deadlock-ul apare aproape de
fiecare dată. Dacă cele două luări sunt lipite, mai apare doar când comutarea
de task cade exact între ele. Este același defect, dar rar, deci mult mai
periculos într-un produs: trece de toate testele și apare la client.

## 3. Teorie

Un deadlock este posibil doar dacă sunt adevărate **toate patru** condițiile
lui Coffman (1971):

| Condiția | Înțelesul |
|---|---|
| **excludere mutuală** | resursa poate fi ținută de un singur task |
| **ține și așteaptă** (hold and wait) | un task ține o resursă în timp ce cere alta |
| **fără preempțiune a resursei** | resursa nu poate fi luată cu forța; o eliberează doar cel care o ține |
| **așteptare circulară** (circular wait) | există un ciclu de taskuri care se așteaptă unul pe altul |

Este de ajuns să eliminăm **una** dintre ele. Strategii:

| Strategia | Cum | Condiția eliminată | Cost |
|---|---|---|---|
| **prevenire prin ordine globală** | toate taskurile iau zăvoarele în aceeași ordine | așteptarea circulară | niciunul la rulare; cere disciplină de proiect |
| **un singur zăvor** pentru ambele resurse | — | ține și așteaptă | mai puțin paralelism |
| **detectare și revenire** | fiecare luare are timp limită; cine nu primește al doilea zăvor îl eliberează pe primul, așteaptă și reîncearcă | ține și așteaptă | timp pierdut la fiecare revenire |
| **plafonul de prioritate** | vezi [Inversiunea de prioritate](priority-inversion.md) | așteptarea circulară | suport din partea nucleului |

La revenire, timpul de așteptare trebuie să fie **aleator**. Dacă ambele
taskuri ar reveni după exact același timp, ar lua din nou câte un zăvor în
același moment și s-ar bloca la fel, la nesfârșit. Aceasta se numește
**livelock**: taskurile rulează, dar nu avansează.

Două observații pentru sistemele de timp real:

- Moștenirea priorității **nu** previne deadlock-ul.
- O așteptare fără timp limită transformă un deadlock într-o oprire definitivă
  și tăcută. Cu timp limită, deadlock-ul devine o eroare care poate fi
  raportată și tratată. Un watchdog care supraveghează taskurile blocate este
  ultima plasă de siguranță.

## 4. Exemplu

Trei taskuri și trei zăvoare: T1 folosește (a, b), T2 folosește (b, c), T3
folosește (c, a), fiecare luându-le în ordinea scrisă.

```
T1 ține a, așteaptă b  ->  T2 ține b, așteaptă c  ->  T3 ține c, așteaptă a  ->  T1
```

Graful are un ciclu, deci deadlock-ul este posibil. Stabilim ordinea globală
a < b < c. T1 ia (a, b), T2 ia (b, c), iar T3 trebuie să ia (a, c) în loc de
(c, a). Acum un ciclu ar cere ca un task să țină un zăvor „mai mare” și să
aștepte unul „mai mic”, ceea ce ordinea interzice. Deadlock-ul a devenit
imposibil fără niciun cost la rulare.

## 5. Greșeli frecvente

- **Așteptări fără timp limită peste tot.** Un deadlock devine definitiv și
  invizibil.
- **Ordinea zăvoarelor ținută „în cap”.** Trebuie scrisă ca regulă de proiect,
  altfel următorul programator o încalcă.
- **Revenire cu timp fix.** Duce la livelock.

## 6. Întrebări de verificare

1. În ce stare sunt taskurile aflate în deadlock și cât procesor consumă? De ce
   nu le observă un watchdog care supraveghează doar bucla principală?
2. Care dintre condițiile lui Coffman sunt, practic, imposibil de eliminat
   într-un sistem cu resurse partajate, și care sunt cele asupra cărora se
   acționează de obicei?
3. De ce este deadlock-ul rar mai periculos într-un produs decât cel care apare
   la fiecare rulare?

Pentru mai mult: E. G. Coffman, M. Elphick, A. Shoshani, „System Deadlocks”,
*ACM Computing Surveys*, 1971; G. Buttazzo, *Hard Real-Time Computing
Systems*, cap. 7.

Laborator asociat: [Lab 5](../../lab5-shared-resources/README.md).
