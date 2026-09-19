# Supraîncărcare și politica de depășire

Citiți înainte: [Analiza timpului de răspuns](response-time-analysis.md).

---

## 1. Problema

Analiza de planificabilitate presupune că știm timpii de calcul și că
$U \le 1$. În exploatare, ipotezele pot cădea: un calcul durează mai mult
decât s-a estimat, apare o rafală de evenimente, un defect face ca un task să
ruleze prea des. Sistemul intră în **supraîncărcare** (overload): cererea de
procesor depășește ce poate oferi procesorul, deci *cineva* va rata termene.

Nu se mai pune întrebarea „cum respectăm toate termenele”, ci două întrebări
noi: **cine** ratează, și **ce se întâmplă** cu joburile care au întârziat. Un
sistem bine proiectat are un răspuns ales dinainte la amândouă.

## 2. Ideea

**Cine ratează.** La priorități fixe, un task este întârziat doar de taskurile
mai prioritare decât el. Supraîncărcarea nu schimbă această regulă: taskurile
prioritare își primesc procesorul ca și înainte, iar lipsa se adună în josul
listei. Ratează taskul cu **cea mai mică prioritate**, și doar el, cât timp
lipsa este mică.

**Ce se întâmplă cu joburile întârziate.** Când un job termină după ce
următoarea lui eliberare a trecut deja, sunt două posibilități:

```
eliberări        |         |         |         |         |
                 0         1         2         3         4

omitere          [==== job 0 ====]   [== job 2 ==]       [== job 4 ...
(skip)                     ^ jobul 1 este abandonat; se așteaptă eliberarea 2

recuperare       [==== job 0 ====][== job 1 ==][== job 2 ==][== job 3 ...
(catch-up)       niciun job nu se pierde, dar fiecare pornește tot mai târziu
```

La **omitere**, taskul renunță la joburile al căror moment a trecut și se
realiniază la grila de eliberări. La **recuperare**, toate joburile se execută,
unul după altul, cât de repede se poate.

## 3. Teorie

**Supraîncărcare permanentă**: $U > 1$. Cererea medie depășește capacitatea,
deci lipsa crește fără limită în timp. Niciun algoritm de planificare nu poate
respecta toate termenele.

**Supraîncărcare tranzitorie**: $U \le 1$ pe termen lung, dar pe un interval
scurt cererea depășește capacitatea, de exemplu pentru că un job a durat mai
mult decât WCET-ul presupus sau pentru că au sosit multe evenimente deodată.
După acel interval sistemul are rezervă și își poate reveni.

**Comportarea planificatoarelor la supraîncărcare:**

- La **priorități fixe**, efectul este *previzibil și localizat*. Timpul de
  răspuns al lui $\tau_i$ depinde doar de $hp(i)$, deci taskurile prioritare nu
  simt nimic; ratează cele de jos, în ordinea priorității. Proiectantul poate
  ști dinainte cine cedează.
- La **priorități dinamice** (EDF), care altfel folosește procesorul până la
  $U = 1$, supraîncărcarea produce un **efect de domino**: un job întârziat își
  împinge termenul peste ale celorlalte, și ajung să rateze pe rând taskuri
  fără nicio legătură cu cauza.

Această previzibilitate este unul dintre motivele pentru care prioritățile fixe
domină în practică. Are și un revers: cu Rate Monotonic prioritatea urmează
perioada, nu importanța. Dacă taskul cu perioada cea mai lungă este și cel care
nu are voie să rateze, el va fi primul sacrificat. Remediile sunt de proiectare:
un task important și rar poate fi împărțit în pași mai scurți și mai deși, iar
taskurile neesențiale pot fi oprite sau rărite la supraîncărcare (**degradare
controlată**, graceful degradation).

**Politica de depășire** (overrun policy) stabilește soarta joburilor
întârziate:

| | Omitere (skip) | Recuperare (catch-up) |
|---|---|---|
| Joburi pierdute | da | nu |
| Întârzierea joburilor executate | mărginită: se revine la grilă | la supraîncărcare permanentă crește fără limită |
| Date prelucrate | cele mai recente | tot mai vechi |
| Revenire după o supraîncărcare tranzitorie | imediată | treptată, cât timp există rezervă |
| Potrivită pentru | control și măsurare: o comandă calculată din date vechi este inutilă sau dăunătoare | jurnalizare, contorizare, comunicație: fiecare element contează, momentul mai puțin |

Alegerea ține de natura taskului, în termenii din
[Ce înseamnă timp real](../00-what-is-real-time.md): omiterea tratează taskul ca
**firm** (un rezultat întârziat nu mai valorează nimic), recuperarea îl
tratează ca **soft** (un rezultat întârziat valorează mai puțin, dar valorează).

Pentru un task periodic, așteptarea **absolută** (până la următorul punct al
grilei; vezi [Eliberare periodică](../1-time/periodic-release.md)) duce în mod
natural la recuperare: dacă punctul următor a trecut deja, jobul pornește
imediat. Omiterea trebuie cerută explicit, sărind peste punctele trecute.

## 4. Exemplu

**Setul C**, priorități Rate Monotonic: *sensor* $(10, 3)$, *control*
$(15, 5)$, *logger* $(35, 14)$.
$U = 0{,}300 + 0{,}333 + 0{,}400 = 1{,}033 > 1$.

Într-o hiperperioadă de 210 ms:

- cererea este $21 \cdot 3 + 14 \cdot 5 + 6 \cdot 14 = 63 + 70 + 84 = 217$ ms;
  lipsesc 7 ms;
- *sensor* și *control* își iau cele $63 + 70 = 133$ ms fără nicio întârziere:
  timpii lor de răspuns rămân 3 și 8 ms;
- pentru *logger* rămân $210 - 133 = 77$ ms, față de 84 ms cerute.

Primul job al lui *logger*, din instantul critic (o coloană = 1 ms):

```
t (ms)   0         10        20        30        40  44
         |         |         |         |         |   |
         SSSCCCCCLLSSSLLCCCCCSSSLLLLLLLSSSCCCCCLLSSSL
```

*logger* termină la 44 ms, după termenul de 35 ms și după următoarea lui
eliberare.

- **Cu omitere**: jobul eliberat la 35 ms este abandonat, iar *logger* reia de
  la 70 ms. Din cele 6 joburi ale unei hiperperioade se pot executa cel mult
  $\lfloor 77 / 14 \rfloor = 5$. Întârzierea rămâne mărginită.
- **Cu recuperare**: toate joburile se execută, dar fiecare hiperperioadă
  adaugă 7 ms de restanță. După zece hiperperioade (2,1 s) întârzierea lui
  *logger* este de ordinul a 70 ms și continuă să crească; datele scrise sunt
  tot mai vechi.

În ambele cazuri, *sensor* și *control* nu observă nimic.

## 5. Greșeli frecvente

- **„La supraîncărcare ratează toate taskurile câte puțin.”** La priorități
  fixe ratează doar cele de jos; cele de sus nu sunt afectate deloc.
- **„Recuperarea este mai sigură, pentru că nu pierde nimic.”** La
  supraîncărcare permanentă pierde altceva: actualitatea datelor, fără limită.
- **„Taskul cel mai important este protejat automat.”** Este protejat taskul
  cel mai *prioritar*, iar cu Rate Monotonic acesta este cel mai *des*, nu cel
  mai important.

## 6. Întrebări de verificare

1. Cât trebuie să scadă $C$ al lui *logger* în setul C pentru ca $U \le 1$?
   Este aceasta de ajuns pentru ca setul să devină planificabil? Verificați cu
   analiza timpului de răspuns.
2. Alegeți politica de depășire pentru: un regulator de turație, un contor de
   energie, un afișaj de temperatură. Justificați.
3. Un set cu $U = 0{,}9$ suferă o supraîncărcare tranzitorie de 20 ms. Cu
   recuperare, în cât timp dispare restanța?

Pentru mai mult: G. Buttazzo, *Hard Real-Time Computing Systems*, cap. 9
(„Handling Overload Conditions”); A. Burns, A. Wellings, *Real-Time Systems
and Programming Languages*, cap. despre planificare.

Laborator asociat: [Lab 3](../../lab3-periodic-rm/README.md).
