# Timpul de execuție în cazul cel mai defavorabil (WCET)

Citiți înainte: [Modelul de task periodic](task-model.md).

---

## 1. Problema

Toată analiza de planificabilitate pornește de la $C_i$, timpul de calcul al
fiecărui task. Dacă valoarea folosită este prea mică, garanția obținută nu
valorează nimic: un singur job care durează mai mult decât s-a presupus poate
face să rateze termenul un task care, pe hârtie, era în siguranță.

De aceea $C_i$ nu este timpul *mediu* de execuție, ci **timpul de execuție în
cazul cel mai defavorabil** (worst-case execution time, WCET). Dificultatea
este că acest caz nu se vede ușor: aceeași porțiune de program durează diferit
de la o execuție la alta, iar cazul cel mai rău poate fi foarte rar.

## 2. Ideea

Dacă executăm de multe ori aceeași porțiune de program și numărăm cât a durat
de fiecare dată, obținem o distribuție:

```
număr de
execuții
   |       ##
   |      ####
   |     ######
   |    #########  #  .  .      .
   +----|----|-------------|---------|----------|-----> timp de execuție
       BCET  medie       maxim      WCET      limita
                         măsurat    real      garantată
```

- Cele mai multe execuții sunt aproape de medie.
- Distribuția are o **coadă** lungă spre dreapta: puține execuții, mult mai
  lente.
- **Maximul măsurat** este cea mai lentă execuție pe care am apucat să o
  vedem. **WCET-ul real** este cea mai lentă execuție posibilă. Între ele
  poate fi o distanță mare, pentru că nimic nu garantează că testele au
  produs cazul cel mai rău.
- O **limită garantată**, obținută prin analiză, este mai mare sau egală cu
  WCET-ul real.

Pentru un sistem de timp real contează capătul din dreapta al distribuției, nu
mijlocul ei.

## 3. Teorie

**BCET** (best-case execution time) și **WCET** sunt cel mai scurt și cel mai
lung timp de execuție posibil, peste *toate* datele de intrare și *toate*
stările inițiale ale hardware-ului. Relația fundamentală este:

$$
\text{maximul măsurat} \;\le\; \text{WCET real} \;\le\; \text{limita garantată}
$$

Maximul măsurat este deci o **limită inferioară** a WCET-ului, nu WCET-ul
însuși.

**Ce influențează timpul de execuție:**

1. **Datele de intrare și calea de execuție.** Numărul de iterații ale unei
   bucle și ramurile alese depind de date. Cazul cel mai rău corespunde de
   obicei unei intrări anume, care nu apare neapărat la teste aleatoare.
2. **Starea hardware-ului.** Memoriile cache, predicția salturilor și
   execuția în bandă de asamblare (pipeline) fac ca aceeași instrucțiune să
   dureze diferit în funcție de ce s-a executat înainte. Prima execuție după
   pornire, sau prima după ce alt cod a umplut cache-ul, este mai lentă
   (cache „rece” față de cache „cald”).
3. **Întreruperile și preempțiunea.** Dacă măsurăm timpul scurs între început
   și sfârșit, măsurăm și rutinele de întrerupere sau taskurile care au
   intervenit. Acestea **nu fac parte din $C_i$**: analiza le numără separat,
   ca interferență (vezi
   [Analiza timpului de răspuns](response-time-analysis.md)). Incluse și în
   $C_i$, ar fi numărate de două ori. Rămâne însă un efect indirect real: codul
   care a intervenit strică starea cache-ului, iar reluarea este mai lentă.

**Două metode de a obține WCET-ul:**

| | Bazată pe măsurare | Analiză statică |
|---|---|---|
| Cum | se execută programul pe hardware-ul real, cu multe intrări | se analizează programul și un model al procesorului, fără execuție |
| Rezultat | o limită inferioară, plus o margine | o limită superioară garantată |
| Avantaj | simplă, ține seama de hardware-ul real | sigură |
| Slăbiciune | cazul cel mai rău poate lipsi din teste | pesimistă; cere un model exact al procesorului |
| Folosită în | majoritatea sistemelor industriale | avionică, auto, acolo unde se cere certificare |

**Marginea de siguranță.** La metoda bazată pe măsurare, valoarea folosită în
analiză este maximul măsurat înmulțit cu un factor supraunitar. Marginea
acoperă ce nu s-a putut testa; mărimea ei este o decizie inginerească,
justificată prin cât de bine acoperă testele intrările și stările hardware.

**Pesimism și siguranță.** O valoare prea mică este periculoasă. O valoare
prea mare este sigură, dar risipește procesorul: un set de taskuri apare ca
neplanificabil deși ar funcționa. Cu cât hardware-ul este mai complex (mai
multe niveluri de cache, execuție speculativă, mai multe nuclee), cu atât
distanța dintre medie și limita garantată crește. De aceea în sistemele de
timp real strict se preferă procesoare simple și previzibile.

## 4. Exemplu

**Dependența de date.** Sortarea prin inserție a $n = 64$ de numere. Numărul de
mutări depinde de cât de dezordonată este intrarea:

| Intrarea | Mutări | Pentru $n = 64$ |
|---|---|---|
| deja sortată | $0$ | 0 |
| aleatoare, în medie | $n(n-1)/4$ | 1008 |
| sortată invers | $n(n-1)/2$ | 2016 |

Cazul cel mai defavorabil este de două ori mai costisitor decât media.
Testarea cu intrări aleatoare nu îl găsește: una singură din cele $64!$
ordonări posibile este cea inversă. Cazul cel mai rău trebuie *construit* din
cunoașterea algoritmului, nu așteptat de la teste.

**Marginea.** Să presupunem, de exemplu, că maximul măsurat al unui job este
de 100 µs, pe intrarea cea mai defavorabilă și cu cache-ul rece. Cu o margine
de 20 %, în analiză intră $C = 120$ µs. Dacă măsurătorile s-ar fi făcut doar pe
intrări aleatoare și cu cache-ul cald, aceeași margine de 20 % nu ar fi
acoperit nici măcar diferența dintre medie și cazul cel mai rău din tabel.

**Dubla numărare.** Un job de 100 µs este întrerupt de o rutină de întrerupere
de 30 µs. Timpul scurs este 130 µs, dar $C$ rămâne 100 µs; cele 30 µs intră în
analiză ca interferență a întreruperii.

## 5. Greșeli frecvente

- **„Am măsurat de un milion de ori, deci maximul este WCET-ul.”** Numărul de
  măsurători nu contează dacă intrarea și starea hardware cele mai defavorabile
  nu s-au aflat printre ele.
- **„WCET-ul este o proprietate a programului.”** Este o proprietate a
  programului *împreună cu* procesorul, memoria și compilatorul. Același
  program are alt WCET pe alt hardware sau cu alte opțiuni de compilare.
- **„Un procesor mai rapid în medie are și un WCET mai bun.”** Mecanismele care
  îmbunătățesc media (cache, predicție, execuție speculativă) măresc de obicei
  distanța până la cazul cel mai rău.

## 6. Întrebări de verificare

1. De ce este maximul măsurat o limită inferioară a WCET-ului, și nu una
   superioară?
2. O căutare liniară într-un tablou de 100 de elemente se oprește la primul
   element găsit. Care este intrarea cea mai defavorabilă? Dar pentru o căutare
   binară?
3. Un job măsurat cu întreruperile active are maximul de 140 µs, iar cu ele
   oprite, 105 µs. Care valoare stă la baza lui $C$, și unde intră în analiză
   diferența?

Pentru mai mult: R. Wilhelm și colab., „The Worst-Case Execution-Time Problem
— Overview of Methods and Survey of Tools”, *ACM TECS*, 2008; G. Buttazzo,
*Hard Real-Time Computing Systems*, cap. 1.

Laborator asociat: [Lab 3](../../lab3-periodic-rm/README.md).
