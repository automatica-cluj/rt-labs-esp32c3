# Rate Monotonic și limita Liu & Layland

Citiți înainte: [Modelul de task periodic](task-model.md),
[Preempțiune cu priorități fixe](../2-scheduling/fixed-priority-preemption.md).

---

## 1. Problema

Un planificator cu priorități fixe are o singură regulă: rulează taskul gata
de execuție cu prioritatea cea mai mare. Regula este simplă, dar lasă
proiectantului două întrebări:

1. **Ce prioritate primește fiecare task?** Pentru $n$ taskuri există $n!$
   atribuiri posibile, iar unele respectă toate termenele și altele nu.
2. **Cum știm dinainte** că, odată prioritățile alese, toate termenele vor fi
   respectate?

## 2. Ideea

Răspunsul la prima întrebare este **Rate Monotonic** (RM): *cu cât perioada
este mai scurtă, cu atât prioritatea este mai mare.* Prioritatea se alege după
**ritm**, nu după importanța taskului.

Intuiția: un task cu perioada scurtă are și termenul apropiat, deci puțin timp
de așteptat. Un task cu perioada lungă își permite să fie întrerupt de mai
multe ori. Două taskuri, $\tau_1 = (T, C) = (10, 3)$ și $\tau_2 = (35, 10)$,
eliberate împreună:

```
t (ms)             0         10        20        30
                   |         |         |         |
RM (tau1 sus)      1112222222111222............    tau1 termină la 3, tau2 la 16
invers (tau2 sus)  2222222222111.......            tau1 termină la 13 > 10: ratat
```

Cu RM, ambele taskuri își respectă termenele. Cu prioritățile inversate,
$\tau_1$ așteaptă cele 10 ms ale lui $\tau_2$ și ratează, deși procesorul este
ocupat mai puțin de 60 %.

Răspunsul la a doua întrebare este un **test de planificabilitate**. Cel mai
simplu privește doar factorul de utilizare $U$.

## 3. Teorie

**Atribuirea RM**: $T_i < T_j \Rightarrow$ prioritatea lui $\tau_i$ este mai
mare decât a lui $\tau_j$. Prioritățile sunt fixe: se stabilesc la proiectare
și nu se schimbă la rulare.

**Optimalitate** (Liu și Layland, 1973). În modelul de bază, cu $D_i = T_i$,
RM este optimă între atribuirile cu priorități fixe: *dacă un set de taskuri
poate fi planificat cu vreo atribuire de priorități fixe, poate fi planificat
și cu RM.* Nu are deci rost să căutăm altă atribuire; dacă RM nu reușește,
niciuna nu reușește.

**Limita Liu & Layland** (testul de utilizare). Un set de $n$ taskuri este
planificabil cu RM dacă

$$
U \;=\; \sum_{i=1}^{n} \frac{C_i}{T_i} \;\le\; n\left(2^{1/n} - 1\right)
$$

| $n$ | 1 | 2 | 3 | 4 | 5 | $\to \infty$ |
|---|---|---|---|---|---|---|
| limita | 1,000 | 0,828 | 0,780 | 0,757 | 0,743 | $\ln 2 \approx 0{,}693$ |

Testul este **suficient, dar nu necesar**:

- $U \le$ limita: setul este *garantat* planificabil, oricare ar fi perioadele.
- limita $< U \le 1$: testul **nu spune nimic**. Setul poate fi planificabil
  sau nu, în funcție de cum se potrivesc perioadele între ele. Este nevoie de
  un test exact, [analiza timpului de răspuns](response-time-analysis.md).
- $U > 1$: setul nu este planificabil cu niciun algoritm (vezi
  [Supraîncărcare](overload.md)).

Limita este pesimistă pentru că acoperă cea mai nefericită combinație de
perioade posibilă pentru un $U$ dat. Seturile reale au rareori acele perioade;
pentru perioade alese la întâmplare, RM reușește în medie până spre
$U \approx 0{,}88$.

**Prețul priorităților fixe.** Un algoritm cu priorități dinamice, cum este
EDF (Earliest Deadline First), planifică orice set cu $U \le 1$. RM renunță la
o parte din procesor în schimbul simplității și al unei comportări previzibile
la supraîncărcare; de aceea este totuși alegerea obișnuită în practică.

## 4. Exemplu

**Setul A**: *sensor* $(10, 2)$, *control* $(20, 4)$, *logger* $(40, 8)$.
Priorități RM: *sensor* > *control* > *logger*.
$U = 0{,}600 \le 0{,}780$: planificabil, garantat de test.

**Setul B**: *sensor* $(10, 3)$, *control* $(15, 4)$, *logger* $(35, 10)$.
$U = 0{,}300 + 0{,}267 + 0{,}286 = 0{,}852 > 0{,}780$: testul nu spune nimic.
Desenăm planificarea din instantul critic (S, C, L = taskul care rulează,
o coloană = 1 ms):

```
t (ms)   0         10        20     27
         |         |         |      |
         SSSCCCCLLLSSSLLCCCCLSSSLLLL
```

- *sensor* termină la 3 ms ($\le 10$);
- *control* termină la 7 ms ($\le 15$);
- *logger* adună 10 ms de procesor în bucățile rămase libere și termină la
  27 ms ($\le 35$).

Instantul critic este cazul cel mai rău, deci setul B **este planificabil**,
deși depășește limita. Testul de utilizare doar nu a putut să o arate.

**Setul B cu prioritățile inversate** (*logger* cea mai mare, *sensor* cea mai
mică), de exemplu pentru că „înregistrarea datelor este cea mai importantă”:

```
t (ms)   0         10     17
         |         |      |
         LLLLLLLLLLCCCCSSS
```

*sensor* termină la 17 ms, după termenul lui de 10 ms. Același set, aceeași
utilizare, alt rezultat: prioritățile au fost alese după importanță, nu după
ritm.

## 5. Greșeli frecvente

- **„$U$ depășește limita, deci setul nu este planificabil.”** Testul este doar
  suficient. Peste limită nu se trage nicio concluzie; se aplică un test exact.
- **Priorități după importanță.** Prioritatea este un mijloc de a respecta
  termene, nu o măsură a importanței. Dacă un task important are perioada
  lungă, RM îi dă prioritate mică *și totuși* îi garantează termenul.
- **„$U \le 1$ este de ajuns.”** Este de ajuns doar pentru priorități dinamice
  (EDF). La priorități fixe, între limită și 1 rezultatul depinde de perioade.

## 6. Întrebări de verificare

1. Un set are taskurile $(10, 4)$ și $(14, 6)$. Calculați $U$ și comparați cu
   limita pentru $n = 2$. Desenați planificarea RM din instantul critic și
   stabiliți dacă setul este planificabil.
2. De ce este de ajuns să verificăm planificarea pornind din instantul critic?
3. Dați un exemplu de situație în care taskul cel mai important din sistem
   primește, corect, cea mai mică prioritate.

## 7. Aprofundare (opțional)

**Limita hiperbolică** (Bini, Buttazzo, Buttazzo, 2003) este un test suficient
mai puțin pesimist, la fel de ușor de calculat:

$$
\prod_{i=1}^{n} \left(U_i + 1\right) \;\le\; 2
$$

Pentru setul A: $1{,}2^3 = 1{,}728 \le 2$, planificabil. Pentru setul B:
$1{,}300 \cdot 1{,}267 \cdot 1{,}286 = 2{,}117 > 2$, deci nici acest test nu
decide; rămâne analiza exactă.

**Perioade armonice.** Dacă fiecare perioadă este multiplu al celor mai scurte
(ca în setul A: 10, 20, 40), limita devine $U \le 1$. Alegerea unor perioade
armonice este, de aceea, un mijloc de proiectare: același procesor suportă mai
multă încărcare, iar hiperperioada este scurtă.

**Termene mai scurte decât perioada.** Când $D_i < T_i$, locul RM este luat
de **Deadline Monotonic** (termen mai scurt, prioritate mai mare), iar testul
de utilizare este înlocuit de analiza timpului de răspuns.

Pentru mai mult: C. L. Liu, J. W. Layland, „Scheduling Algorithms for
Multiprogramming in a Hard-Real-Time Environment”, *Journal of the ACM*, 1973;
G. Buttazzo, *Hard Real-Time Computing Systems*, cap. 4.

Laborator asociat: [Lab 3](../../lab3-periodic-rm/README.md).
