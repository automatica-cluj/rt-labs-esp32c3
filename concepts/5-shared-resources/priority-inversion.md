# Inversiunea de prioritate

Citiți înainte: [Excludere mutuală](mutual-exclusion.md),
[Analiza timpului de răspuns](../3-analysis/response-time-analysis.md).

---

## 1. Problema

Cu priorități fixe ne așteptăm ca un task să aștepte doar după taskuri *mai
prioritare* decât el. Pe această regulă se bazează toată analiza de
planificabilitate. Regula cade când două taskuri împart o resursă păzită de un
zăvor (lock): dacă taskul puțin prioritar **L** ține zăvorul, taskul prioritar
**H** trebuie să aștepte după L. Aceasta este **inversiunea de prioritate**
(priority inversion).

Cât durează secțiunea critică a lui L, inversiunea este acceptabilă: este
prețul excluderii mutuale, iar secțiunile critice sunt scurte. Problema reală
apare când intervine un al treilea task, **M**, cu prioritate medie, care nici
nu folosește resursa.

## 2. Ideea

Trei taskuri pe un singur procesor:

| Task | Prioritate | Ce face |
|---|---|---|
| L | mică | la 0 ms ia zăvorul și calculează 40 ms în secțiunea critică |
| H | mare | la 10 ms cere zăvorul |
| M | medie | la 20 ms începe un calcul de 150 ms, fără zăvor |

1. La 10 ms H îl preemptează pe L, cere zăvorul și se blochează. L continuă.
2. La 20 ms apare M și îl preemptează pe L, care rămâne oprit *cu zăvorul
   luat*.
3. M calculează 150 ms. L nu rulează, deci nu eliberează zăvorul, deci H
   așteaptă.
4. După ce M termină, L își încheie secțiunea critică și abia atunci pornește
   H.

H, cel mai prioritar task, a așteptat după M, cu care nu împarte nimic
(o coloană = 10 ms):

```
t (ms)                  0    50   100  150  200
                        |    |    |    |    |
zăvor simplu            LLMMMMMMMMMMMMMMMLLH      H blocat de la 10 la 190 ms
moștenirea priorității  LLLLHMMMMMMMMMMMMMMM      H blocat de la 10 la 40 ms
```

Inversiunea se numește **nemărginită** (unbounded) pentru că durata ei nu mai
depinde de secțiunea critică a lui L, ci de cât calculează taskurile medii.
Mai multe taskuri medii înseamnă o așteptare oricât de lungă.

Exact acest scenariu a resetat în mod repetat calculatorul sondei Mars
Pathfinder în 1997: un zăvor fără moștenirea priorității, un task meteorologic
puțin prioritar, taskuri de comunicație medii și un watchdog care reseta
sistemul când taskul prioritar întârzia. Remedierea, trimisă de pe Pământ, a
fost activarea moștenirii priorității.

## 3. Teorie

**Interferență**: întârzierea provocată de taskuri cu prioritate *mai mare*.
Este normală și este prinsă în analiza timpului de răspuns.

**Blocare** (blocking): întârzierea provocată de un task cu prioritate *mai
mică*. Durata ei maximă pentru taskul $\tau_i$ se notează $B_i$.

**Moștenirea priorității** (priority inheritance; Sha, Rajkumar, Lehoczky,
1990): când H se blochează pe un zăvor ținut de L, L rulează cu prioritatea lui
H până eliberează zăvorul, apoi revine la prioritatea lui. Consecințe:

- H așteaptă cel mult **o secțiune critică** a lui L. Blocarea devine
  **mărginită** (bounded): se poate stabili privind doar taskurile care
  folosesc resursa.
- Taskurile medii plătesc și ele: M așteaptă după L cât timp acesta rulează cu
  prioritate moștenită, deși nu împarte nimic cu el (**blocare prin
  împingere**, push-through blocking).
- Un task care folosește mai multe zăvoare poate fi blocat câte o dată pentru
  fiecare (**blocare în lanț**).
- Moștenirea **nu previne deadlock-ul** (vezi [Deadlock](deadlock.md)).

**Plafonul de prioritate** (priority ceiling): fiecare zăvor primește ca plafon
prioritatea celui mai prioritar task care îl folosește, iar taskul care ia
zăvorul rulează imediat cu acea prioritate. Un task este blocat cel mult o
singură dată, oricâte zăvoare folosește, și deadlock-ul devine imposibil.
Oprirea întreruperilor pe durata unei secțiuni critice este același principiu
dus la extrem: plafonul este mai presus de orice task.

**Blocarea în analiza timpului de răspuns.** Ecuația primește un termen în
plus:

$$
R_i \;=\; C_i \;+\; B_i \;+\; \sum_{j \in hp(i)} \left\lceil \frac{R_i}{T_j} \right\rceil C_j
$$

$B_i$ apare o singură dată: un job poate fi blocat doar de o secțiune critică
începută *înainte* de eliberarea lui. Taskul cu cea mai mică prioritate are
$B = 0$. De aici se vede de ce blocarea trebuie să fie mărginită: fără o
valoare pentru $B_i$ nu se poate garanta niciun termen.

## 4. Exemplu

**Cele trei taskuri de mai sus.** H cere zăvorul la 10 ms, când lui L i-au
rămas 30 ms din secțiunea critică. Fiecare task mediu calculează 150 ms.

| Zăvor | Taskuri medii | Blocarea lui H |
|---|---|---|
| simplu | unul | $30 + 150 = 180$ ms |
| simplu | două | $30 + 150 + 150 = 330$ ms |
| cu moștenirea priorității | unul sau două | $30$ ms |

Sub zăvorul simplu blocarea crește cu fiecare task mediu; sub moștenire rămâne
aceeași. Aceasta este diferența dintre „nemărginit” și „mărginit”.

**Un set de taskuri cu o resursă.** *sensor* și *logger* împart un buffer păzit
de un zăvor cu moștenirea priorității; secțiunea critică a lui *logger* durează
cel mult 1 ms.

| Task | $T$ (ms) | $C$ (ms) | Prioritate | $B$ (ms) | De ce |
|---|---|---|---|---|---|
| sensor | 10 | 2 | mare | 1 | așteaptă direct după *logger* |
| control | 20 | 4 | medie | 1 | *logger* rulează 1 ms cu prioritate moștenită, deasupra lui |
| logger | 40 | 8 | mică | 0 | nu are cine să îl blocheze |

Pentru *control*: $R^0 = 4 + 1 = 5$; $R^1 = 5 + \lceil 5/10 \rceil \cdot 2 = 7$;
$R^2 = 7$. Deci $R = 7$ ms, față de 6 ms fără resursă. Resursa partajată costă
1 ms, o valoare cunoscută dinainte, și setul rămâne planificabil.

## 5. Greșeli frecvente

- **„Ridic prioritatea lui H.”** H are deja prioritatea cea mai mare; problema
  este că L nu rulează. Doar ridicarea lui L (moștenire sau plafon) ajută.
- **Secțiuni critice lungi.** Moștenirea limitează blocarea la durata secțiunii
  critice; dacă aceasta este lungă, și blocarea este lungă.
- **„Moștenirea previne deadlock-ul.”** Nu îl previne; doar plafonul de
  prioritate o face.

## 6. Întrebări de verificare

1. De ce nu îl ajută pe H faptul că are cea mai mare prioritate din sistem?
2. În exemplul cu setul de taskuri, de ce $B_{control} = 1$ ms deși *control*
   nu folosește bufferul?
3. Un task folosește pe rând trei zăvoare, fiecare împărțit cu alt task mai
   puțin prioritar, cu secțiuni critice de 2, 3 și 5 ms. Cât este blocarea lui
   maximă sub moștenire? Dar sub plafonul de prioritate?
4. De ce apare $B_i$ o singură dată în ecuație, și nu înmulțit cu numărul de
   eliberări, ca interferența?

Pentru mai mult: G. Buttazzo, *Hard Real-Time Computing Systems*, cap. 7;
L. Sha, R. Rajkumar, J. Lehoczky, „Priority Inheritance Protocols”, *IEEE
Transactions on Computers*, 1990; G. Reeves, „What really happened on Mars”,
1997.

Laborator asociat: [Lab 5](../../lab5-shared-resources/README.md).
