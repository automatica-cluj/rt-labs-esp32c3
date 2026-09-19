# Sisteme de timp real — note de curs pentru laborator

Aceste pagini descriu conceptele teoretice din spatele celor cinci lucrări de
laborator. Sunt pagini de teorie: nu conțin cod, funcții ale vreunui sistem de
operare sau instrucțiuni de lucru. Partea practică se află în lucrările de
laborator (`labN-.../README.md`).

Termenii de specialitate apar în română, cu termenul englezesc în paranteză la
prima folosire, pentru că documentația FreeRTOS și literatura sunt în engleză.
Dicționarul complet este în [glossary.md](glossary.md).

## Cum este construită o pagină

Fiecare pagină are aceleași secțiuni:

1. **Problema** — o situație concretă în care apare conceptul.
2. **Ideea** — explicația intuitivă, de obicei cu o diagramă de timp.
3. **Teorie** — definițiile, proprietățile și formulele de reținut, fără
   demonstrații.
4. **Exemplu** — un caz mic, calculat de mână.
5. **Greșeli frecvente** — idei greșite des întâlnite despre concept.
6. **Întrebări de verificare**.

Fiecare pagină se încheie cu o trimitere la bibliografie și cu laboratorul
asociat. O pagină are una-două ecrane.

Formulele sunt scrise în LaTeX (`$...$`), afișat corect de GitHub și de
site-ul generat din aceste pagini.

## Ordinea de citire

| Înainte de | Citiți |
|---|---|
| toate | [Ce înseamnă timp real](00-what-is-real-time.md) |
| Lab 1 | [Ceasuri și rezoluție](1-time/clocks-and-resolution.md), [Latență și jitter](1-time/latency-and-jitter.md), [Eliberare periodică](1-time/periodic-release.md) |
| Lab 2 | [Taskuri și stări](2-scheduling/tasks-and-states.md), [Preempțiune cu priorități fixe](2-scheduling/fixed-priority-preemption.md), [Înfometare și watchdog](2-scheduling/starvation-and-watchdogs.md) |
| Lab 3 | [Modelul de task](3-analysis/task-model.md), [WCET](3-analysis/wcet.md), [Rate Monotonic](3-analysis/rate-monotonic.md), [Analiza timpului de răspuns](3-analysis/response-time-analysis.md), [Supraîncărcare](3-analysis/overload.md) |
| Lab 4 | [Întreruperi și latență](4-interrupts/interrupts-and-latency.md), [Procesare amânată](4-interrupts/deferred-processing.md), [Secțiuni critice](4-interrupts/critical-sections.md), [Debouncing](4-interrupts/debouncing.md) |
| Lab 5 | [Condiții de cursă](5-shared-resources/race-conditions.md), [Excludere mutuală](5-shared-resources/mutual-exclusion.md), [Inversiunea de prioritate](5-shared-resources/priority-inversion.md), [Deadlock](5-shared-resources/deadlock.md) |

## Notații folosite peste tot

| Simbol | Semnificație |
|---|---|
| $\tau_i$ | taskul $i$; un indice mai mic înseamnă prioritate mai mare |
| $T_i$ | perioada: la fiecare $T_i$ se eliberează (release) un nou job |
| $C_i$ | timpul de calcul al unui job, în cazul cel mai defavorabil (WCET) |
| $D_i$ | termenul limită relativ (deadline); în laborator $D_i = T_i$ |
| $R_i$ | timpul de răspuns: de la eliberare până la terminare |
| $B_i$ | timpul de blocare: cât poate fi întârziat $\tau_i$ de taskuri cu prioritate *mai mică* |
| $U$ | factorul de utilizare, $\sum C_i / T_i$ |
| $P_i$ | prioritatea lui $\tau_i$ (în FreeRTOS: număr mai mare = prioritate mai mare) |

## Alte pagini

- [Glosar](glossary.md): termenii în română și engleză, cu trimitere la pagina
  care îi explică.
- [Bibliografie](further-reading.md): cărțile și articolele în care găsiți
  teoria completă, cu demonstrații.

Câteva pagini (Rate Monotonic, analiza timpului de răspuns, inversiunea de
prioritate) au la sfârșit o secțiune **Aprofundare (opțional)**. Nu este
necesară pentru laborator.
