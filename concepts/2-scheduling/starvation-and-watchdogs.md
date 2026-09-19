# Înfometare și watchdog

Citiți înainte: [Preempțiune cu priorități fixe](fixed-priority-preemption.md).

## 1. Problema

Regula „rulează taskul gata de execuție cel mai prioritar” are un revers. Dacă
un task prioritar nu se blochează niciodată, fie pentru că are prea mult de
calculat, fie pentru că a intrat din greșeală într-o buclă fără sfârșit,
niciun task mai puțin prioritar nu mai primește procesorul. Planificatorul nu
intervine: din punctul lui de vedere totul este corect.

Un sistem înglobat rulează ani de zile fără operator. Nimeni nu apasă un buton
când programul se blochează. Sistemul trebuie să observe singur că ceva nu mai
avansează și să își revină.

## 2. Ideea

**Înfometarea** (starvation) este situația în care un task este gata de
execuție, dar nu primește procesorul un timp nelimitat. Un task prioritar care
calculează fără pauză 1 s îl ține pe loc pe cel mai puțin prioritar exact 1 s:

```
fără pauze     HHHHHHHHHHHHHHHHHHHHHHHHHHHHHH....   L așteaptă cât calculează H
               L nu rulează deloc                   

cu pauze       HHHHHHHHHH.HHHHHHHHHH.HHHHHHHHHH.    H se blochează scurt, periodic
               ..........L..........L..........L    L rulează în fiecare pauză
```

Este de ajuns ca H să se blocheze foarte puțin, de exemplu 1 ms după fiecare
10 ms de calcul, pentru ca L să ruleze regulat. H pierde cel mult o zecime din
timp, iar cea mai lungă așteptare a lui L scade de la durata întregului calcul
la aproximativ 10 ms.

Pentru defectele pe care proiectarea nu le-a prevăzut există **watchdog-ul**:
un temporizator hardware care numără spre zero și, dacă ajunge acolo, resetează
sistemul. Programul sănătos îl reîncarcă periodic („îl hrănește”). Un program
care nu mai avansează nu îl mai hrănește, deci este repornit.

## 3. Teorie

**Înfometare** (starvation): un task Ready nu este ales de planificator un timp
nemărginit. Sub priorități fixe, ea nu este un defect al planificatorului, ci o
consecință directă a regulii. Proprietăți:

- un task poate fi înfometat doar de taskuri *mai prioritare* (sau, prin zăvoare,
  vezi [Inversiunea de prioritate](../5-shared-resources/priority-inversion.md));
- taskurile mai puțin prioritare rulează doar cât timp **toate** cele mai
  prioritare sunt blocate. Un task trebuie deci să se blocheze, așteptând timp
  sau un eveniment, ca să lase loc celor de sub el;
- **așteptarea activă** (busy waiting), adică o buclă care verifică repetat o
  condiție, nu este o blocare: taskul rămâne Running și înfometează tot ce este
  sub el;
- **cedarea voluntară** (yield) a procesorului ajută doar taskurile de aceeași
  prioritate. Pentru cele mai puțin prioritare este nevoie de o blocare
  adevărată;
- într-un sistem proiectat corect, înfometarea este exclusă prin analiză: dacă
  [factorul de utilizare](../3-analysis/task-model.md) este sub 1 și fiecare
  task are timp de calcul mărginit, fiecare task ajunge să ruleze.

**Watchdog** (watchdog timer): mecanism de **revenire din defect**, nu de
prevenire. Ce detectează: *lipsa de progres* a activității supravegheate într-un
interval dat. Nu detectează rezultate greșite, termene ratate izolat sau
defecte care nu opresc hrănirea.

Reguli de folosire:

- **Hrănirea se face din activitatea al cărei progres contează**, într-un punct
  prin care programul trece doar dacă a funcționat corect un ciclu întreg. Dacă
  sunt mai multe taskuri supravegheate, watchdog-ul este hrănit doar după ce
  *toate* și-au semnalat progresul.
- **Hrănirea din interiorul activității defecte anulează watchdog-ul.** Un task
  care intră într-o buclă fără sfârșit și hrănește watchdog-ul din acea buclă
  raportează „sunt în viață” în timp ce înfometează restul sistemului. La fel,
  hrănirea dintr-o întrerupere periodică dovedește doar că întreruperile
  funcționează.
- **Intervalul** se alege mai lung decât cel mai rău caz legitim al activității
  supravegheate, dar mai scurt decât timpul în care defectul produce pagube.
- Sistemele de operare de timp real oferă de obicei două niveluri: un watchdog
  pentru taskuri (un task supravegheat nu a mai rulat) și unul pentru
  întreruperi (întreruperile au rămas oprite prea mult, vezi
  [Secțiuni critice](../4-interrupts/critical-sections.md)).

**Resetul și cauza lui.** După repornire, sistemul trebuie să afle *de ce* a
repornit: alimentare, comandă software, watchdog, cădere de tensiune.
Procesoarele păstrează această cauză într-un registru care supraviețuiește
resetului. Un reset de watchdog este un simptom care trebuie înregistrat și
investigat, nu o funcționare normală.

Prin comparație, un sistem de operare de uz general limitează timpul pe care îl
pot consuma taskurile de timp real (throttling), astfel încât restul sistemului
să rămână utilizabil; taskul vinovat este încetinit, nu repornit.

## 4. Exemplu

Un task H, prioritar, are de făcut un calcul de 6 s. Un task L, mai puțin
prioritar, trebuie să ruleze cel puțin o dată la 3 s, altfel watchdog-ul, care
îl supraveghează pe L, resetează sistemul.

| Cum se comportă H | Cea mai lungă așteptare a lui L | Urmarea |
|---|---|---|
| calculează 1 s fără pauză (caz de comparație) | 1 s | L este înfometat temporar; fără reset |
| calculează 6 s fără pauză | 6 s, peste 3 s | reset după 3 s |
| calculează 6 s, cu 1 ms de blocare după fiecare 10 ms | aproximativ 10 ms | fără reset; calculul lui H se lungește cu cel mult $600 \cdot 1$ ms $= 0{,}6$ s |
| calculează 6 s fără pauză și hrănește el însuși watchdog-ul | 6 s | reset după 3 s: watchdog-ul îl supraveghează pe L, iar L tot nu rulează |

Ultimul rând arată ce supraveghează de fapt watchdog-ul: nu „că cineva îl
hrănește”, ci că *fiecare* activitate înscrisă își dovedește progresul. Dacă
H ar fi singura activitate supravegheată, hrănirea din bucla lui ar ascunde
complet înfometarea lui L.

## 5. Greșeli frecvente

- **„Watchdog-ul previne blocajele.”** Nu le previne; le transformă într-un
  reset. Cauza rămâne în program și trebuie găsită.
- **„Hrănesc watchdog-ul cât mai des, ca să fiu sigur.”** Hrănirea pusă peste
  tot, inclusiv în bucle care pot rula la nesfârșit, face watchdog-ul inutil.
  Un singur punct de hrănire, la sfârșitul unui ciclu complet, este regula.
- **„Un task prioritar care nu se blochează este doar ineficient.”** Sub
  priorități fixe el oprește complet tot ce este sub el, inclusiv taskurile de
  întreținere ale sistemului de operare.

## 6. Întrebări de verificare

1. De ce o pauză de 1 ms la fiecare 10 ms este suficientă pentru ca un task
   puțin prioritar să ruleze regulat? Ce se întâmplă dacă acel task are nevoie
   de 5 ms de calcul la fiecare rulare?
2. Un sistem are trei taskuri critice. Propuneți o schemă prin care un singur
   watchdog să detecteze blocarea oricăruia dintre ele.
3. Cum alegeți intervalul watchdog-ului pentru un task care rulează la fiecare
   100 ms și al cărui defect devine periculos după 1 s?
4. De ce limitarea timpului de procesor este potrivită pentru un calculator de
   birou, iar resetul prin watchdog pentru un stimulator cardiac?

Pentru mai mult: G. Buttazzo, *Hard Real-Time Computing Systems*, cap. 2 și 9;
J. Ganssle, „Great Watchdog Timers for Embedded Systems”; A. Burns,
A. Wellings, *Real-Time Systems and Programming Languages*, cap. 2.

Laborator asociat: [Lab 2](../../lab2-scheduling/README.md).
