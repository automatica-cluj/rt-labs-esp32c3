# Modelul de task periodic

---

## 1. Problema

Pentru un sistem de timp real strict nu este de ajuns să constatăm la test că
termenele sunt respectate. Trebuie să arătăm *înainte de rulare* că vor fi
respectate în cel mai rău caz. O astfel de demonstrație nu se poate face pe
programul real, cu toate detaliile lui. Se face pe un **model**: o descriere
simplificată, în care fiecare activitate a sistemului este redusă la câteva
numere.

Modelul folosit de aproape toată teoria planificării în timp real este
**modelul de task periodic**, introdus de Liu și Layland în 1973. Toate
celelalte pagini din acest capitol lucrează cu el.

## 2. Ideea

Cele mai multe activități dintr-un sistem de timp real se repetă: citirea unui
senzor la fiecare 10 ms, calculul unei comenzi la fiecare 20 ms, înregistrarea
datelor la fiecare 40 ms. Fiecare activitate devine un **task**, iar fiecare
repetare a ei un **job**.

```
eliberări   |                   |                   |
            v                   v                   v
task        ######..............######..............######.........
            |<--- C --->|
            |<-------- T ------>|
                                ^
                                termenul limită al primului job (D = T)
```

Un job este **eliberat** la începutul perioadei, are nevoie de un anumit timp
de procesor și trebuie să termine înainte de termenul lui limită. Dacă pe
același procesor rulează și taskuri mai prioritare, jobul este întrerupt de
ele și termină mai târziu decât dacă ar fi singur:

```
task prioritar    ###.......###.......###.......
taskul analizat   ...####......##...............
                  ^               ^
                  eliberare       terminare
                  |<---- R ------>|      R = timpul de răspuns
```

Întrebarea la care răspunde analiza este mereu aceeași: în cel mai rău caz,
este $R \le D$ pentru fiecare task?

## 3. Teorie

Un set de $n$ taskuri periodice $\tau_1, \dots, \tau_n$ este descris de:

- **Perioada** $T_i$ (period): intervalul dintre două eliberări consecutive.
- **Eliberarea** (release): momentul în care un job devine gata de execuție.
  Jobul $k$ al lui $\tau_i$ este eliberat la $k \cdot T_i$.
- **Timpul de calcul** $C_i$ (computation time): timpul de procesor cerut de un
  job în cazul cel mai defavorabil, adică [WCET](wcet.md)-ul lui.
- **Termenul limită relativ** $D_i$ (deadline): cât timp are jobul la
  dispoziție de la eliberare. În modelul de bază $D_i = T_i$: un job trebuie să
  termine înainte de eliberarea următorului.
- **Timpul de răspuns** $R_i$ (response time): de la eliberare până la
  terminare, în cel mai rău caz. Un job **ratează termenul** dacă $R_i > D_i$.
- **Factorul de utilizare** (utilisation): fracțiunea de procesor cerută,

$$
U = \sum_{i=1}^{n} \frac{C_i}{T_i}
$$

Pe un singur procesor, $U \le 1$ este o condiție **necesară**: dacă $U > 1$,
niciun planificator nu poate respecta toate termenele. Nu este însă și
suficientă pentru planificarea cu priorități fixe (vezi
[Rate Monotonic](rate-monotonic.md)).

Pe lângă taskurile periodice există taskuri **sporadice**, declanșate de
evenimente, pentru care $T_i$ este intervalul *minim* dintre două eliberări.
În analiză se tratează ca taskuri periodice cu perioada $T_i$, pentru că acesta
este cazul lor cel mai defavorabil.

**Instantul critic** (critical instant). Timpul de răspuns al unui task
depinde de cum sunt decalate eliberările celorlalte. Liu și Layland au arătat
că, la priorități fixe, cel mai rău caz pentru un task apare atunci când este
eliberat *simultan* cu toate taskurile mai prioritare. Consecința practică:
este de ajuns să analizăm un singur scenariu, cel în care toate taskurile
pornesc împreună la $t = 0$. Dacă primul job al fiecărui task își respectă
termenul în acest scenariu, toate joburile și-l vor respecta mereu.

**Hiperperioada** (hyperperiod) este cel mai mic multiplu comun al perioadelor,
$H = \mathrm{cmmmc}(T_1, \dots, T_n)$. După $H$ tiparul eliberărilor se repetă
identic, deci comportarea sistemului pe un interval $H$ le descrie pe toate.

**Ipotezele modelului de bază:**

1. taskurile sunt independente: nu împart resurse și nu se așteaptă unul pe
   altul;
2. $D_i = T_i$;
3. $C_i$ este constant și cunoscut;
4. prioritățile sunt fixe, iar planificarea este preemptivă (vezi
   [Preempțiune cu priorități fixe](../2-scheduling/fixed-priority-preemption.md));
5. comutarea de context și planificatorul nu costă nimic;
6. eliberările sunt perfect periodice.

**Unde se abate realitatea:**

| Ipoteza | În realitate | Cum se corectează |
|---|---|---|
| taskuri independente | taskurile împart date și se blochează | un termen de blocare în analiză (vezi [Inversiunea de prioritate](../5-shared-resources/priority-inversion.md)) |
| $C_i$ constant | timpul de execuție variază cu datele și cu starea hardware | se folosește cazul cel mai defavorabil (vezi [WCET](wcet.md)) |
| costuri nule | comutările de context și întreruperile consumă timp | se adaugă la $C_i$ sau se tratează ca taskuri foarte prioritare |
| eliberări perfecte | ceasul sistemului are rezoluție finită; trezirea întârzie | un termen de jitter (vezi [Latență și jitter](../1-time/latency-and-jitter.md)) |

## 4. Exemplu

Setul A: *sensor* $(T, C) = (10, 2)$, *control* $(20, 4)$, *logger* $(40, 8)$,
în milisecunde.

- Utilizarea: $U = 2/10 + 4/20 + 8/40 = 0{,}2 + 0{,}2 + 0{,}2 = 0{,}600$.
- Hiperperioada: $H = \mathrm{cmmmc}(10, 20, 40) = 40$ ms.
- Joburi într-o hiperperioadă: 4, 2 și 1.
- Timp de procesor cerut într-o hiperperioadă: $4 \cdot 2 + 2 \cdot 4 + 1 \cdot 8 = 24$ ms
  din 40, adică exact $U \cdot H$. Procesorul este liber 16 ms.

Setul B: *sensor* $(10, 3)$, *control* $(15, 4)$, *logger* $(35, 10)$.

- $U = 0{,}300 + 0{,}267 + 0{,}286 = 0{,}852$.
- $H = \mathrm{cmmmc}(10, 15, 35) = 210$ ms, cu 21, 14 și 6 joburi.
- Cerere: $21 \cdot 3 + 14 \cdot 4 + 6 \cdot 10 = 179$ ms din 210. Rămân 31 ms.

## 5. Greșeli frecvente

- **„$U < 1$, deci setul este planificabil.”** Condiția este doar necesară.
  La priorități fixe un set cu $U = 0{,}85$ poate rata termene dacă
  prioritățile sunt alese prost.
- **$C$ luat ca timp mediu de execuție.** Modelul garantează ceva doar dacă
  $C$ acoperă cazul cel mai defavorabil.
- **Confuzia dintre $C$ și $R$.** $C$ este timpul de procesor de care are
  nevoie jobul; $R$ include și timpul în care jobul a așteptat după alții.
  Aproape întotdeauna $R > C$.

## 6. Întrebări de verificare

1. Un set are taskurile $(5, 1)$, $(8, 2)$ și $(20, 6)$. Calculați $U$,
   hiperperioada și numărul de joburi ale fiecărui task într-o hiperperioadă.
2. De ce este instantul critic cel mai rău caz pentru un task cu prioritate
   mică? Ce se schimbă pentru taskul cu cea mai mare prioritate?
3. Un task sporadic este declanșat de un buton care poate fi apăsat de cel mult
   5 ori pe secundă. Cu ce perioadă intră în model, și de ce este aceasta o
   alegere sigură?

Pentru mai mult: C. L. Liu, J. W. Layland, „Scheduling Algorithms for
Multiprogramming in a Hard-Real-Time Environment”, 1973; G. Buttazzo, *Hard
Real-Time Computing Systems*, cap. 2 și 4.

Laborator asociat: [Lab 3](../../lab3-periodic-rm/README.md).
