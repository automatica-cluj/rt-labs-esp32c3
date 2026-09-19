# Condiții de cursă

---

## 1. Problema

Două taskuri care scriu aceeași variabilă pot strica rezultatul chiar dacă
fiecare, luat separat, este corect. Rezultatul depinde de *momentul* în care
planificatorul comută între ele: aceasta este o **condiție de cursă** (race
condition). Într-un sistem de timp real este deosebit de periculoasă, pentru
că momentele comutărilor depind de tick, de întreruperi și de încărcare, deci
defectul apare rar, diferit de la o rulare la alta, și aproape niciodată la
test.

## 2. Ideea

Două taskuri adună fiecare 1 la același contor, de un milion de ori. Rezultatul
corect este 2 000 000. Operația „contor = contor + 1” pare un singur pas, dar
procesorul o execută în trei: **citește** valoarea din memorie într-un
registru, **adună** 1, **scrie** registrul înapoi. Dacă planificatorul comută
taskul între citire și scriere, se pierde o actualizare (lost update):

```
task 1:  citește 500 ............................................ scrie 501
task 2:              citește 500, scrie 501, ... , scrie 4500
                     (o felie întreagă de timp)

contorul ajunge la 501: toate adunările taskului 2 din acea felie s-au pierdut
```

De aceea o singură comutare nefericită costă mult: nu se pierde *o* adunare, ci
tot ce a făcut celălalt task până când primul apucă să scrie valoarea lui
veche. Valorile din diagramă sunt ilustrative.

## 3. Teorie

**Secțiune critică** (critical section): porțiunea de cod care nu are voie să
fie intercalată cu alt cod ce folosește aceleași date.

**Operație atomică** (atomic): se execută complet sau deloc, fără stări
intermediare vizibile din afară.

**Condiție de cursă**: corectitudinea rezultatului depinde de ordinea sau de
momentul relativ al execuției mai multor fire.

O dată partajată are nevoie de protecție dacă sunt adevărate toate trei:

1. este folosită de cel puțin două fire de execuție (taskuri sau rutine de
   întrerupere);
2. cel puțin unul dintre ele o **scrie**;
3. operația are mai mulți pași: citește–modifică–scrie, sau actualizarea mai
   multor câmpuri care trebuie să rămână coerente între ele.

Trei proprietăți care fac cursele greu de stăpânit:

- **Sunt nedeterministe.** Apar doar când comutarea cade într-o fereastră de
  câteva instrucțiuni; frecvența lor depinde de încărcare, de priorități și de
  momentul întreruperilor.
- **Se pot ascunde complet.** Dacă cele două taskuri au priorități diferite și
  cel prioritar nu se blochează niciodată, el rulează până la capăt înaintea
  celuilalt. Nu există intercalare, deci nu se pierde nimic, deși codul este la
  fel de greșit. Orice schimbare care readuce intercalarea (o așteptare în
  buclă, priorități egale, un al doilea nucleu) readuce defectul.
- **Un singur nucleu nu protejează.** Preempțiunea la tick sau la o întrerupere
  intercalează taskurile la fel de bine ca două nuclee.

Concluzia: **un test care trece nu dovedește absența unei curse.** Cursele se
elimină prin proiectare (vezi [Excludere mutuală](mutual-exclusion.md)), nu
prin testare.

## 4. Exemplu

Un task de măsurare actualizează o pereche (valoare, momentul măsurării); un
task de afișare o citește.

```
măsurare:  scrie valoare = 25.1 ......................... scrie moment = 1200
afișare:                        citește valoare = 25.1,
                                citește moment = 1100   <- momentul vechi
```

Taskul de afișare nu scrie nimic, și totuși vede o stare care nu a existat
niciodată: valoarea nouă cu momentul vechi. Condițiile 1–3 sunt îndeplinite
(două fire, unul scrie, actualizare în mai mulți pași), deci *și cititorul*
trebuie să participe la protecție.

## 5. Greșeli frecvente

- **„Compilatorul citește variabila din memorie de fiecare dată, deci e
  sigur.”** Faptul că valoarea este recitită nu face operația indivizibilă; ea
  rămâne în trei pași.
- **„Pe un singur nucleu nu pot apărea curse.”** Pot, prin preempțiune.
- **„A trecut testul, deci e corect.”** O cursă ascunsă de priorități trece
  orice test până când se schimbă ceva în sistem.

## 6. Întrebări de verificare

1. De ce numărul actualizărilor pierdute este mult mai mare decât numărul
   comutărilor nefericite?
2. Două taskuri cu priorități diferite incrementează același contor și nu se
   pierde nimic. Este codul corect? Ce l-ar strica din nou?
3. O variabilă este incrementată într-o rutină de întrerupere și citită într-un
   task. Care dintre cele trei condiții sunt îndeplinite?

Pentru mai mult: G. Buttazzo, *Hard Real-Time Computing Systems*, cap. 7;
A. Burns, A. Wellings, *Real-Time Systems and Programming Languages*, cap.
despre sincronizare cu variabile partajate.

Laborator asociat: [Lab 5](../../lab5-shared-resources/README.md).
