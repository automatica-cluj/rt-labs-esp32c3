# Întreruperi și latență

## 1. Problema

Planificatorul unui sistem de timp real lucrează cu pasul unui tick, de
exemplu 1 ms. Lumea din afară nu așteaptă tick-ul: un front pe o linie de
intrare, un octet sosit pe o interfață de comunicație sau expirarea unui
temporizator hardware apar oricând și cer un răspuns în microsecunde.
Mecanismul pentru așa ceva este **întreruperea** (interrupt).

Întrebarea unui inginer de timp real nu este „cât de repede răspunde sistemul
de obicei?”, ci „cât de târziu poate răspunde, în cel mai rău caz, și ce anume
îl întârzie?”. Răspunsul este o **latență** maximă, iar pentru a o putea
garanta trebuie să știm din ce este compusă.

## 2. Ideea

Când un periferic cere o întrerupere, procesorul oprește ce făcea și sare la o
funcție specială, **rutina de tratare a întreruperii** (Interrupt Service
Routine, ISR). ISR-ul nu este un task și nu are prioritate de task: rulează
înaintea oricărui task, oricât de prioritar. Planificatorul nu are niciun
cuvânt de spus până când ISR-ul se termină.

Drumul de la eveniment până la codul care face munca are două bucăți:

```
eveniment ──(latența întreruperii)──> ISR ──(latența ISR–task)──> taskul care
hardware                                                          tratează evenimentul
```

Între eveniment și prima instrucțiune din ISR se întâmplă, de fiecare dată:

1. procesorul termină instrucțiunea în curs;
2. dacă întreruperile sunt oprite sau rulează un alt ISR de nivel egal sau mai
   mare, cererea *așteaptă*;
3. procesorul sare la vectorul de întreruperi, iar sistemul salvează registrele
   taskului întrerupt, adică **contextul** lui;
4. sistemul află ce periferic a cerut întreruperea și apelează ISR-ul
   înregistrat pentru el.

Pașii 1, 3 și 4 costă puțin și sunt aproape constanți: sunt prețul fix al
intrării în ISR. Pasul 2 este cel care face latența să crească și este singurul
pe care îl controlează proiectantul.

## 3. Teorie

**Latența întreruperii** (interrupt latency): timpul de la evenimentul hardware
până la prima instrucțiune din ISR.

**Latența ISR–task** (ISR-to-task latency): timpul de la ISR până când rulează
taskul care tratează evenimentul. Este subiectul paginii
[Procesare amânată](deferred-processing.md).

**Nivel de întrerupere** (interrupt level): prioritatea hardware a unei
întreruperi. Un ISR poate fi întrerupt doar de o întrerupere de nivel *mai
mare* (întreruperi imbricate, nesting). Una de nivel egal sau mai mic așteaptă
până la sfârșitul ISR-ului curent. Nivelurile de întrerupere nu au nicio
legătură cu prioritățile taskurilor: cel mai modest ISR trece înaintea celui
mai prioritar task.

Latența întreruperii în cazul cel mai defavorabil are trei componente:

$$
L_{max} \;=\; e \;+\; \max(\text{secțiune cu întreruperile oprite}) \;+\; \sum \text{ISR-uri de nivel egal sau mai mare}
$$

- $e$ este costul fix al intrării: terminarea instrucțiunii, salvarea
  contextului, identificarea sursei. Pe sistemele la care codul se execută
  dintr-o memorie lentă printr-un cache, $e$ include și eventuala lipsă din
  cache.
- Al doilea termen este cea mai lungă porțiune de cod, din *tot* sistemul, care
  rulează cu întreruperile oprite (vezi [Secțiuni critice](critical-sections.md)).
- Al treilea termen este timpul cât procesorul poate fi ocupat de alte ISR-uri
  care nu pot fi întrerupte de al nostru. În cazul simplu, în care fiecare
  apare cel mult o dată în fereastra analizată, este suma duratelor lor.

Consecințe:

- Latența unei întreruperi depinde de cod care nu are nicio legătură cu ea: o
  secțiune critică lungă într-un task oarecare sau un ISR lung al altui
  periferic. Prioritatea taskurilor nu oferă nicio protecție.
- Un ISR lung nu strică doar propriul eveniment, ci toate întreruperile de
  nivel egal sau mai mic și toate taskurile.
- Latența este o proprietate a *întregului sistem*, nu a unui modul. Se poate
  garanta doar dacă există o regulă de proiect pentru durata maximă a
  ISR-urilor și a secțiunilor cu întreruperile oprite.

Alternativa este **interogarea** (polling): programul verifică periodic dacă
evenimentul a apărut. Latența este cel mult o perioadă de verificare și perfect
previzibilă, dar se consumă procesor și în repaus. Întreruperea reacționează
repede și nu costă nimic în repaus, dar îngreunează analiza.

## 4. Exemplu

Un sistem are costul fix de intrare $e$ = 5 µs (de exemplu). În sistem mai
există:

- un task care ține întreruperile oprite cel mult 200 µs;
- un ISR al altui periferic, de același nivel, care durează cel mult 300 µs.

| Situația | Latența maximă a întreruperii |
|---|---|
| sistem liniștit | $e$ = 5 µs |
| doar secțiunea cu întreruperile oprite | 5 + 200 = 205 µs |
| doar celălalt ISR | 5 + 300 = 305 µs |
| ambele, în cel mai rău caz | 5 + 200 + 300 = 505 µs |

Ultimul rând apare când evenimentul sosește imediat după începutul secțiunii cu
întreruperile oprite, iar la sfârșitul ei este servit mai întâi celălalt ISR.
Dacă cerința este o latență de cel mult 50 µs, niciuna dintre cele două nu este
acceptabilă în sistem, chiar dacă fiecare pare „scurtă” privită izolat.

Dacă cele 300 µs de muncă sunt mutate din ISR într-un task, ISR-ul celuilalt
periferic se reduce la câteva microsecunde, iar al treilea termen aproape
dispare (vezi [Procesare amânată](deferred-processing.md)).

## 5. Greșeli frecvente

- **„Taskul meu are prioritatea cea mai mare, deci nimic nu îl întârzie.”**
  Orice ISR și orice secțiune cu întreruperile oprite trece înaintea lui.
- **„Latența este o proprietate a procesorului.”** Procesorul dă doar costul
  fix $e$. Restul vine din software-ul care rulează pe el.
- **„Contează latența medie.”** Media este aproape de $e$ și arată bine
  aproape mereu. Garanția se dă pe maxim, iar maximul apare rar.

## 6. Întrebări de verificare

1. De ce poate un task de prioritate mică să întârzie o întrerupere, deși
   întreruperea trece înaintea oricărui task?
2. Două întreruperi au același nivel. Care este latența maximă a fiecăreia,
   în funcție de durata ISR-ului celeilalte?
3. Când este interogarea periodică o alegere mai bună decât întreruperea?
4. Un sistem are cerința de latență de 50 µs. Ce reguli de proiect decurg din
   formula pentru $L_{max}$?

Pentru mai mult: G. Buttazzo, *Hard Real-Time Computing Systems*, cap. 1 și
cap. 12; A. Burns, A. Wellings, *Real-Time Systems and Programming Languages*,
capitolul despre programarea de nivel scăzut; vezi și
[Bibliografie](../further-reading.md).

Laborator asociat: [Lab 4](../../lab4-interrupts/README.md).
