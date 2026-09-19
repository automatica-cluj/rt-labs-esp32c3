# Eliberare periodică: întârziere relativă și absolută

## 1. Problema

Cele mai multe activități de timp real sunt periodice: un senzor citit la
fiecare 10 ms, un regulator calculat la fiecare 1 ms. Felul cel mai la îndemână
de a obține periodicitatea este „fă calculul, apoi așteaptă o perioadă”. Pare
corect, dar produce un task care rulează sistematic mai rar decât s-a cerut și
care rămâne tot mai mult în urmă față de timpul real. Un ceas construit astfel
ar întârzia cu fiecare oră. Pagina explică de ce și care este forma corectă.

## 2. Ideea

Un task periodic are o **grilă de eliberări**: momentele $t_0,\; t_0 + T,\;
t_0 + 2T, \dots$ la care ar trebui să înceapă fiecare execuție. Întrebarea este
față de ce se măsoară așteptarea dintre două execuții.

Dacă se măsoară **de la sfârșitul calculului**, durata calculului se adaugă la
fiecare perioadă:

```
grila             |         |         |         |         |
relativ           CCC.......... CCC.......... CCC.......... CCC
                  |<--- C + T ->|<--- C + T ->|
```

Dacă se măsoară **de la eliberarea precedentă**, grila este respectată,
indiferent cât a durat calculul:

```
grila             |         |         |         |         |
absolut           CCC.......CCC.......CCC.......CCC.......CCC
                  |<-- T -->|<-- T -->|
```

Prima formă se numește întârziere relativă, a doua întârziere absolută.
Diferența pe o perioadă este mică, dar în prima formă ea se adună.

## 3. Teorie

**Task periodic** (periodic task): un task care produce un șir nesfârșit de
execuții, numite **joburi** (jobs). Jobul $k$ este **eliberat** (released) la

$$
r_k = t_0 + k \cdot T,
$$

unde $T$ este **perioada**. Fiecare job are nevoie de un timp de calcul $C$ și,
în modelul uzual, trebuie să termine până la eliberarea următoare (vezi
[Modelul de task](../3-analysis/task-model.md)).

**Întârzierea relativă** (relative delay): „trezește-mă peste $T$, socotit de
acum”. Momentul „acum” este sfârșitul calculului, care depinde de durata
calculului și de toate întârzierile suferite de job. Intervalul dintre două
porniri devine

$$
t_{k+1} - t_k = T + C_k + \delta_k,
$$

unde $\delta_k \ge 0$ adună preempțiunile și rotunjirea la tick. Perioada
reală este mai mare decât $T$ și, în plus, variabilă.

**Deriva** (drift) este abaterea acumulată față de grilă. Cu întârziere
relativă, după $n$ perioade,

$$
\Delta_n = \sum_{k=0}^{n-1} (C_k + \delta_k) \;\ge\; n \cdot C_{min}.
$$

Deriva crește nelimitat: erorile fiecărei perioade se adună, pentru că fiecare
pornire este calculată din cea precedentă, cu tot cu eroarea ei.

**Întârzierea absolută** (absolute delay): „trezește-mă la momentul
$r_{k+1} = r_k + T$”. Referința este eliberarea precedentă de pe grilă, nu
momentul curent. O întârziere suferită într-o perioadă afectează doar acea
perioadă; următoarea eliberare rămâne la locul ei. Eroarea nu se acumulează,
deci deriva este zero. Rămâne doar [jitterul](latency-and-jitter.md) fiecărei
porniri în parte.

Întârzierea relativă rămâne forma potrivită când se cere o *pauză* între două
acțiuni (de exemplu un timp minim între două comenzi), nu un *ritm*.

**Depășirea perioadei** (overrun). Dacă un job durează mai mult decât $T$,
eliberarea următoare este deja în trecut când jobul termină. Cu întârziere
absolută, așteptarea se încheie imediat și jobul următor pornește pe loc, cu
întârziere. Dacă depășirea a fost un accident, taskul recuperează în câteva
perioade și revine pe grilă. Dacă depășirea este permanentă ($C > T$), taskul
nu mai doarme deloc, rămâne tot mai mult în urmă și ține procesorul ocupat
continuu, în dauna taskurilor mai puțin prioritare. Ce se face cu joburile
întârziate, omitere sau recuperare, este o decizie de proiectare (vezi
[Supraîncărcare](../3-analysis/overload.md)).

## 4. Exemplu

Un task cu $T = 10$ ms și $C = 3$ ms, fără alte taskuri în sistem.

**Cu întârziere relativă.** Intervalul dintre două porniri este
$T + C = 13$ ms. După 300 de perioade au trecut $299 \cdot 13 = 3887$ ms în loc
de $299 \cdot 10 = 2990$ ms. Deriva este de 897 ms: taskul a rulat cu o
frecvență de aproximativ 77 Hz în loc de 100 Hz. Într-o oră ar executa cam
277 000 de joburi în loc de 360 000.

**Cu întârziere absolută.** Pornirile au loc la 0, 10, 20, ... ms. După 300 de
perioade au trecut 2990 ms, iar deriva este zero. Dacă un job este întârziat cu
2 ms de un task mai prioritar, el pornește la $r_k + 2$ ms, dar jobul următor
pornește tot la $r_{k+1}$.

**O depășire accidentală.** Jobul 5 durează 14 ms în loc de 3 ms. El termină la
$50 + 14 = 64$ ms, după eliberarea jobului 6 (60 ms). Jobul 6 pornește imediat,
la 64 ms, și termină la 67 ms; jobul 7 pornește la timp, la 70 ms. Taskul a
revenit pe grilă după o singură perioadă.

## 5. Greșeli frecvente

- **„Aștept T după calcul, deci perioada este T.”** Perioada reală este
  $T + C$ plus întârzierile, și variază odată cu ele.
- **Compensarea „de mână”**: așteptarea a $T - C$ în loc de $T$. Funcționează
  doar dacă $C$ este constant și taskul nu este niciodată preemptat; altfel
  deriva rămâne, doar mai mică.
- **„Întârzierea absolută elimină jitterul.”** Elimină deriva. Fiecare pornire
  în parte poate fi în continuare întârziată de tick, de întreruperi și de
  taskurile mai prioritare.

## 6. Întrebări de verificare

1. Un task cu $T = 20$ ms și $C = 5$ ms folosește întârziere relativă. Care
   este frecvența lui reală și cu cât rămâne în urmă după un minut?
2. De ce nu se acumulează eroarea în cazul întârzierii absolute, deși fiecare
   pornire poate fi întârziată?
3. Un job depășește perioada o singură dată. Descrieți ce se întâmplă cu
   joburile următoare în cele două forme de întârziere.
4. Dați un exemplu în care întârzierea relativă este alegerea corectă.

Pentru mai mult: G. Buttazzo, *Hard Real-Time Computing Systems*, cap. 4;
A. Burns, A. Wellings, *Real-Time Systems and Programming Languages*, cap.
despre facilități de timp (întârzieri relative și absolute).

Laborator asociat: [Lab 1](../../lab1-time-latency/README.md).
