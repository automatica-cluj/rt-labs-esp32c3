# Ce înseamnă timp real

---

## 1. Problema

Un airbag trebuie să se umfle în câteva zeci de milisecunde de la impact. Dacă
se umflă corect, dar cu o secundă mai târziu, rezultatul este la fel de rău ca
un calcul greșit. Un regulator de motor care își citește senzorul la intervale
neregulate devine instabil, deși fiecare calcul al lui este corect.

În aceste sisteme corectitudinea are două părți: **valoarea** rezultatului și
**momentul** în care este livrat. Un **sistem de timp real** (real-time
system) este un sistem în care un rezultat corect livrat prea târziu este un
rezultat greșit.

## 2. Ideea

**Timp real nu înseamnă rapid. Înseamnă previzibil.**

Un sistem rapid are un timp *mediu* de răspuns mic. Un sistem de timp real
are un timp *maxim* de răspuns cunoscut și garantat. Cele două nu sunt același
lucru:

```
timp de răspuns ->   0        5        10       15  ms     termen limită: 10 ms

sistem A (rapid)     ||||||||.  .   .        .         |   medie 2 ms, maxim 14 ms
sistem B (timp real)       ||||||||||                      medie 6 ms, maxim 8 ms
```

Sistemul A este mai rapid în medie, dar ratează uneori termenul. Sistemul B
este mai lent în medie și nu îl ratează niciodată. Pentru un airbag, doar B
este acceptabil. Valorile din diagramă sunt ilustrative.

De aceea mărimea care contează este întotdeauna **maximul**, nu
media: latența maximă, jitterul, timpul de execuție în cazul cel mai
defavorabil, blocarea maximă.

## 3. Teorie

Cerința de timp se exprimă printr-un **termen limită** (deadline): momentul
până la care rezultatul trebuie livrat. După ce se întâmplă când termenul este
ratat, sistemele se împart în trei clase:

| Clasa | Un termen ratat înseamnă | Exemple |
|---|---|---|
| **hard** (strict) | defectare a sistemului, posibil pericol | airbag, ABS, comanda unui motor, pacemaker |
| **firm** | rezultatul întârziat nu mai are nicio valoare, dar sistemul continuă | un cadru video într-o conferință, o măsurătoare periodică omisă |
| **soft** | rezultatul întârziat are valoare mai mică | interfața cu utilizatorul, redarea unui fișier audio |

Pentru un sistem hard nu este de ajuns să *măsurăm* că termenele sunt
respectate la test. Trebuie să *demonstrăm*, înainte de rulare, că vor fi
respectate în cel mai rău caz. Pentru aceasta este nevoie de trei lucruri:

1. **Să măsurăm timpul corect** și să știm ce strică regularitatea (vezi
   [Ceasuri și rezoluție](1-time/clocks-and-resolution.md),
   [Latență și jitter](1-time/latency-and-jitter.md)).
2. **Un planificator previzibil**: reguli simple, pe care se poate face o
   analiză (vezi [Preempțiune cu priorități fixe](2-scheduling/fixed-priority-preemption.md),
   [Rate Monotonic](3-analysis/rate-monotonic.md),
   [Analiza timpului de răspuns](3-analysis/response-time-analysis.md)).
3. **Întârzieri mărginite peste tot**: la întreruperi (vezi
   [Întreruperi și latență](4-interrupts/interrupts-and-latency.md)) și la
   resursele împărțite între taskuri (vezi
   [Inversiunea de prioritate](5-shared-resources/priority-inversion.md)). O
   singură întârziere fără limită anulează toată analiza.

Un sistem de timp real este, de obicei, și un sistem **înglobat** (embedded):
rulează ani de zile fără operator, deci trebuie să își revină singur din
defecte. De aici vine rolul watchdog-ului (vezi
[Înfometare și watchdog](2-scheduling/starvation-and-watchdogs.md)).

## 4. Sistem de operare de timp real și sistem de uz general

| | Uz general (desktop, server) | Timp real (RTOS) |
|---|---|---|
| Scopul planificatorului | debit mediu mare, echitate între programe | respectarea termenelor în cel mai rău caz |
| Regula de planificare | complexă, adaptivă, greu de prezis | simplă: rulează taskul gata de execuție cu prioritatea cea mai mare |
| Prioritățile | sugestii, ajustate de sistem | stricte, stabilite de proiectant |
| Întârzierile interne | de obicei mici, fără limită garantată | mărginite și documentate |
| Ce se optimizează | cazul mediu | cazul cel mai defavorabil |

O regulă simplă de planificare nu este o slăbiciune, ci o condiție: doar un
comportament simplu poate fi analizat și garantat dinainte. Există și variante
de timp real ale sistemelor de uz general (de exemplu Linux cu PREEMPT_RT),
care mărginesc întârzierile interne ale nucleului.

Nici hardware-ul nu este neutru: memoriile cache, magistralele partajate și
perifericele cu driver propriu (de exemplu radioul) fac execuția mai rapidă în
medie și mai puțin previzibilă în cel mai rău caz.

## 5. Greșeli frecvente

- **„Timp real = foarte rapid.”** Un sistem care răspunde în 2 secunde, dar
  *mereu* în mai puțin de 2 secunde, este de timp real. Unul care răspunde în
  1 ms „de obicei” nu este.
- **„Un procesor mai rapid rezolvă problema.”** Micșorează timpii de calcul,
  dar nu elimină o inversiune de prioritate sau o secțiune critică prea lungă.
- **„Am testat o oră și nu a ratat niciun termen.”** Cazul cel mai rău poate
  apărea o dată pe lună. Testul arată prezența defectelor, nu absența lor.

## 6. Întrebări de verificare

1. Clasificați ca hard, firm sau soft: frâna unui lift, afișarea vitezei pe
   bord, un robot de sudură, un player video. Justificați.
2. Un sistem are timpul mediu de răspuns 1 ms și maximul 50 ms. Altul are
   media 8 ms și maximul 9 ms. Care este potrivit pentru un termen de 10 ms?
3. De ce este mai ușor de analizat un planificator cu o singură regulă simplă
   decât unul care încearcă să fie echitabil cu toate taskurile?

Pentru mai mult: G. Buttazzo, *Hard Real-Time Computing Systems*, cap. 1;
vezi și [Bibliografie](further-reading.md).

Laborator asociat: toate; începeți cu [Lab 1](../lab1-time-latency/README.md).
