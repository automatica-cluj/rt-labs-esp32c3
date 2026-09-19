# Analiza timpului de răspuns (RTA)

Citiți înainte: [Modelul de task periodic](task-model.md),
[Rate Monotonic](rate-monotonic.md).

---

## 1. Problema

Testul de utilizare Liu & Layland răspunde doar cu „sigur da” sau „nu știu”.
Pentru seturile aflate între limită și $U = 1$, adică cele mai multe seturi
reale bine încărcate, avem nevoie de un test care să spună exact dacă fiecare
termen este respectat. Mai mult, am vrea să știm nu doar *dacă*, ci și *cu
cât*: care este cel mai mare timp de răspuns al fiecărui task și câtă rezervă
rămâne până la termen.

## 2. Ideea

Un job termină atunci când a primit tot timpul de procesor de care are nevoie.
Până atunci, de fiecare dată când este eliberat un task mai prioritar, jobul
nostru este întrerupt și așteaptă. Timpul lui de răspuns este deci:

```
timp de răspuns  =  calculul propriu  +  tot ce au rulat taskurile mai prioritare
                                         între eliberarea și terminarea jobului
```

A doua parte se numește **interferență**. Dificultatea este că ea depinde
chiar de ceea ce căutăm: cu cât jobul durează mai mult, cu atât mai multe
eliberări ale taskurilor prioritare prinde, deci durează și mai mult.

```
t (ms)     0         10        20     27
           |         |         |      |
sensor     SSS.......SSS.......SSS....
control    ...CCCC........CCCC........
logger     .......LLL...LL....L...LLLL     logger: 10 ms de calcul, R = 27 ms
```

Rezolvarea este să căutăm, pas cu pas, primul moment în care cererea de
procesor acumulată este egală cu timpul scurs.

## 3. Teorie

Fie $hp(i)$ mulțimea taskurilor cu prioritate mai mare decât $\tau_i$. Într-un
interval de lungime $R$ care începe la instantul critic, taskul $\tau_j$ este
eliberat de $\lceil R / T_j \rceil$ ori și cere de fiecare dată $C_j$.
**Interferența** suferită de $\tau_i$ este deci

$$
I_i(R) \;=\; \sum_{j \in hp(i)} \left\lceil \frac{R}{T_j} \right\rceil C_j
$$

iar timpul de răspuns este cea mai mică soluție a ecuației

$$
R_i \;=\; C_i \;+\; \sum_{j \in hp(i)} \left\lceil \frac{R_i}{T_j} \right\rceil C_j
$$

$R_i$ apare în ambii membri, sub un plafon, deci ecuația nu se rezolvă
algebric. Se rezolvă prin **iterație de punct fix**:

$$
R_i^{(0)} = C_i, \qquad
R_i^{(k+1)} \;=\; C_i \;+\; \sum_{j \in hp(i)} \left\lceil \frac{R_i^{(k)}}{T_j} \right\rceil C_j
$$

Iterația se oprește în unul din două feluri:

- $R_i^{(k+1)} = R_i^{(k)}$: am găsit timpul de răspuns. Taskul își respectă
  termenul dacă $R_i \le D_i$.
- $R_i^{(k)} > D_i$: termenul poate fi ratat; nu are rost să continuăm.

**Proprietăți:**

- Șirul este crescător, pentru că plafonul nu scade când argumentul crește.
  Dacă $U \le 1$, el se oprește după un număr finit de pași.
- Fiecare valoare are un înțeles: $R^{(k+1)}$ este cererea de procesor
  acumulată până la momentul $R^{(k)}$. Cât timp cererea depășește timpul
  scurs, jobul nu a putut termina.
- Testul este **exact** (necesar și suficient) în ipotezele modelului: taskuri
  independente, $D_i \le T_i$, priorități fixe, costuri de comutare neglijabile.
  „Exact” înseamnă că valoarea calculată chiar apare, la instantul critic.
- Taskul cel mai prioritar are $R = C$. Timpul de răspuns al unui task depinde
  doar de taskurile *mai* prioritare, niciodată de cele de sub el.
- Analiza se face separat pentru fiecare task; setul este planificabil dacă
  toate taskurile trec.

Față de testul de utilizare, RTA cere mai mult calcul, dar dă un răspuns
definitiv și, în plus, **rezerva** fiecărui task, $D_i - R_i$.

## 4. Exemplu

**Setul B**, priorități Rate Monotonic: *sensor* $(10, 3)$, *control*
$(15, 4)$, *logger* $(35, 10)$; $U = 0{,}852$, peste limita de $0{,}780$.

- *sensor*: nu are taskuri mai prioritare. $R = 3 \le 10$.
- *control*, întrerupt doar de *sensor*:
  $R^{(0)} = 4$;
  $R^{(1)} = 4 + \lceil 4/10 \rceil \cdot 3 = 7$;
  $R^{(2)} = 4 + \lceil 7/10 \rceil \cdot 3 = 7$. Deci $R = 7 \le 15$.
- *logger*, întrerupt de amândouă:
  $R^{(0)} = 10$;
  $R^{(1)} = 10 + \lceil 10/10 \rceil \cdot 3 + \lceil 10/15 \rceil \cdot 4 = 10 + 3 + 4 = 17$;
  $R^{(2)} = 10 + \lceil 17/10 \rceil \cdot 3 + \lceil 17/15 \rceil \cdot 4 = 10 + 6 + 8 = 24$;
  $R^{(3)} = 10 + \lceil 24/10 \rceil \cdot 3 + \lceil 24/15 \rceil \cdot 4 = 10 + 9 + 8 = 27$;
  $R^{(4)} = 10 + \lceil 27/10 \rceil \cdot 3 + \lceil 27/15 \rceil \cdot 4 = 10 + 9 + 8 = 27$.
  Deci $R = 27 \le 35$.

Setul B este planificabil, cu rezerve de 7, 8 și 8 ms. Valoarea de 27 ms este
exact cea din diagrama de la secțiunea 2: până la 27 ms, *sensor* a fost
eliberat de 3 ori (9 ms), *control* de 2 ori (8 ms), iar $10 + 9 + 8 = 27$.

Rezultatele pentru celelalte seturi (timp de răspuns și, în paranteză,
termenul, în ms):

| Setul | sensor | control | logger | Concluzie |
|---|---|---|---|---|
| A: $(10,2)$, $(20,4)$, $(40,8)$ | 2 (10) | 6 (20) | 16 (40) | planificabil |
| B: $(10,3)$, $(15,4)$, $(35,10)$ | 3 (10) | 7 (15) | 27 (35) | planificabil |
| C: $(10,3)$, $(15,5)$, $(35,14)$ | 3 (10) | 8 (15) | 14, 25, 33, 41 > 35 | *logger* ratează |
| B cu prioritățile inversate | 3, 17 > 10 | 14 (15) | 10 (35) | *sensor* ratează |

În setul C, cu $U = 1{,}033$, iterația pentru *logger* trece de termen la al
patrulea pas. În ultimul rând, *sensor* are prioritatea cea mai mică și suferă
interferența celorlalte două: $3 + 4 + 10 = 17 > 10$.

## 5. Greșeli frecvente

- **Interferența numărată o singură dată**, ca $C_i + \sum C_j$. Un task
  prioritar cu perioada scurtă întrerupe de mai multe ori; de aici plafonul
  $\lceil R / T_j \rceil$.
- **Rotunjire în jos** în loc de plafon. Un job eliberat chiar înainte de
  terminarea noastră ne întrerupe *în întregime*, deci se numără complet.
- **Taskurile mai puțin prioritare incluse în sumă.** Ele nu pot întârzia un
  task mai prioritar, cât timp taskurile sunt independente.

## 6. Întrebări de verificare

1. Calculați timpii de răspuns pentru setul A, scriind fiecare iterație.
2. În setul B, cât poate crește $C$ al lui *logger* până când setul devine
   neplanificabil? Verificați pentru $C = 13$.
3. De ce este șirul iterațiilor crescător, și ce se întâmplă cu el dacă
   $U > 1$?

## 7. Aprofundare (opțional)

Ecuația se extinde ușor când ipotezele modelului de bază nu mai sunt valabile:

$$
R_i \;=\; C_i \;+\; B_i \;+\; \sum_{j \in hp(i)} \left\lceil \frac{R_i + J_j}{T_j} \right\rceil C_j
$$

- **Blocare** $B_i$: cea mai lungă secțiune critică prin care un task *mai
  puțin* prioritar îl poate întârzia pe $\tau_i$, atunci când taskurile împart
  resurse. Apare o singură dată (vezi
  [Inversiunea de prioritate](../5-shared-resources/priority-inversion.md)).
- **Jitter de eliberare** $J_j$: dacă eliberările unui task prioritar pot
  întârzia cu până la $J_j$, două dintre ele se pot apropia, iar interferența
  crește.
- **Termene mai scurte decât perioada**, $D_i < T_i$: ecuația rămâne aceeași,
  condiția devine $R_i \le D_i$, iar prioritățile se atribuie Deadline
  Monotonic.

Pentru mai mult: M. Joseph, P. Pandya, „Finding Response Times in a Real-Time
System”, *The Computer Journal*, 1986; N. Audsley și colab., „Applying New
Scheduling Theory to Static Priority Pre-emptive Scheduling”, 1993;
G. Buttazzo, *Hard Real-Time Computing Systems*, cap. 4.

Laborator asociat: [Lab 3](../../lab3-periodic-rm/README.md).
