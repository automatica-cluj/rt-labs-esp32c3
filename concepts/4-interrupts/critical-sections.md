# Secțiuni critice

Citiți înainte: [Întreruperi și latență](interrupts-and-latency.md).

## 1. Problema

Unele secvențe de cod nu au voie să fie întrerupte: actualizarea unei date pe
care o folosește și un ISR, o măsurătoare de timp care nu trebuie deranjată,
câteva operații asupra unui periferic care trebuie făcute una după alta. Cea
mai simplă cale de a garanta acest lucru este oprirea întreruperilor pe durata
secvenței.

Este o unealtă foarte puternică și foarte ieftină, și tocmai de aceea
periculoasă: cât timp întreruperile sunt oprite, *nimic altceva* din sistem nu
mai poate reacționa. Întrebarea este cât de lungă are voie să fie o astfel de
secvență și când trebuie folosit altceva.

## 2. Ideea

Cu întreruperile oprite nu mai există tick, deci planificatorul nu mai rulează,
deci nu mai există comutare de task. Nu mai rulează nici ISR-urile: cererile de
întrerupere rămân în așteptare până la repornire.

```
task oarecare, prioritate mică:   ....[ întreruperi oprite, 200 µs ]....
eveniment hardware:                       ^ sosește aici
ISR:                                      |------ așteaptă ------>[ISR]
task de tratare, prioritate mare:                                      [rulează]
```

Un task de prioritate mică a întârziat o întrerupere și, prin ea, un task de
prioritate mare. Prioritățile nu oferă nicio protecție, pentru că mecanismul
acționează *sub* planificator: planificatorul însuși este scos din joc pe
durata secțiunii.

## 3. Teorie

**Secțiune critică** (critical section): în sens larg, orice porțiune de cod
care nu are voie să fie intercalată cu alt cod ce folosește aceleași date. În
sens restrâns, folosit pe această pagină: o porțiune de cod executată cu
întreruperile oprite.

Proprietăți:

- **Excludere totală.** Pe un procesor cu un singur nucleu, oprirea
  întreruperilor exclude orice alt task și orice ISR. Este singurul mecanism
  care protejează datele împărțite cu un ISR, pentru că un ISR nu se poate
  bloca în așteptarea unui zăvor.
- **Cost propriu foarte mic.** Intrarea și ieșirea înseamnă câteva
  instrucțiuni, fără apel la planificator și fără comutare de context.
- **Cost pentru ceilalți egal cu durata ei.** Dacă cea mai lungă secțiune cu
  întreruperile oprite durează $d_{max}$, atunci latența *fiecărei* întreruperi
  din sistem crește în cel mai rău caz cu $d_{max}$:

$$
L_{max} \;=\; e \;+\; d_{max} \;+\; \sum \text{ISR-uri de nivel egal sau mai mare}
$$

- **Fără inversiune nemărginită.** Secțiunea nu poate fi preemptată, deci se
  termină în timpul ei propriu de execuție. Este principiul plafonului de
  prioritate dus la extrem: plafonul este mai presus de orice task și de orice
  întrerupere (vezi
  [Inversiunea de prioritate](../5-shared-resources/priority-inversion.md)).
- **Pe mai multe nuclee nu ajunge.** Oprirea întreruperilor acționează pe un
  singur nucleu; celălalt rulează mai departe. Acolo este nevoie în plus de un
  zăvor cu așteptare activă (spinlock).

Reguli care decurg de aici:

1. Durata trebuie să fie **mărginită și cunoscută**: fără bucle cu număr
   necunoscut de pași, fără apeluri a căror durată nu se știe.
2. **Fără operații blocante** în interior. Cu planificatorul oprit, un task
   care așteaptă ceva nu mai poate fi trezit de nimeni.
3. În secțiune se **copiază** datele; prelucrarea se face în afara ei.
4. Durata maximă admisă se stabilește din cea mai strictă cerință de latență a
   sistemului, nu din nevoile codului care o folosește.

Comparația cu un mutex (vezi
[Excludere mutuală](../5-shared-resources/mutual-exclusion.md)):

| | Întreruperi oprite | Mutex |
|---|---|---|
| Pe cine întârzie | pe toată lumea: taskuri și ISR-uri | doar taskurile care vor aceeași resursă |
| Cost propriu | câteva instrucțiuni | apeluri la nucleu, eventual o comutare de context |
| Date împărțite cu un ISR | da | nu |
| Durată potrivită | microsecunde | oricât, dar cât mai scurt |

Multe sisteme au un **watchdog de întreruperi** (interrupt watchdog): un
temporizator hardware care resetează sistemul dacă întreruperile rămân oprite
peste o limită, de exemplu câteva sute de milisecunde. Este o plasă de
siguranță împotriva blocărilor, nu o limită de proiectare: o secțiune critică
acceptabilă este cu câteva ordine de mărime mai scurtă.

## 4. Exemplu

Un sistem are costul fix de intrare în ISR $e$ = 5 µs (de exemplu) și o cerință:
întreruperea unui senzor trebuie servită în cel mult 50 µs.

Un task de prioritate mică actualizează un tabel de 100 de valori cu
întreruperile oprite, iar actualizarea durează 200 µs. Latența maximă a
senzorului devine $5 + 200 = 205$ µs și cerința este încălcată, deși taskul
vinovat are cea mai mică prioritate din sistem.

Trei remedii, în ordinea preferinței:

| Remediu | Durata cu întreruperile oprite | Latența maximă |
|---|---|---|
| tabelul este pregătit într-o copie, iar în secțiune se schimbă doar un indicator către copia nouă | câteva instrucțiuni, de exemplu 1 µs | $5 + 1 = 6$ µs |
| tabelul este păzit de un mutex, dacă niciun ISR nu îl folosește | 0 | 5 µs |
| actualizarea se face în 100 de secțiuni scurte, câte una pe valoare | 2 µs | $5 + 2 = 7$ µs, dar tabelul poate fi văzut pe jumătate actualizat |

Bugetul se împarte: dacă în sistem mai există și un ISR de același nivel care
durează 30 µs, pentru secțiunile critice rămân cel mult $50 - 5 - 30 = 15$ µs.

## 5. Greșeli frecvente

- **„O fac mai lungă, ca să fiu sigur.”** Tot ce este în secțiune se adaugă la
  latența tuturor întreruperilor și la timpul de răspuns al tuturor taskurilor.
- **„Taskul meu este puțin prioritar, deci nu deranjează pe nimeni.”** Cu
  întreruperile oprite prioritatea nu mai contează.
- **„Media latenței nu s-a schimbat, deci secțiunea este inofensivă.”** Efectul
  apare doar când evenimentul cade în interiorul secțiunii, adică rar. Se vede
  în maxim, nu în medie.

## 6. Întrebări de verificare

1. De ce nu poate proteja un mutex o dată împărțită între un task și un ISR?
2. Un sistem are cerința de latență de 100 µs, costul fix de 10 µs și un ISR de
   același nivel de 40 µs. Cât poate dura cel mult o secțiune cu întreruperile
   oprite?
3. De ce este interzisă o operație blocantă într-o secțiune cu întreruperile
   oprite? Ce s-ar întâmpla cu sistemul?
4. În ce sens este oprirea întreruperilor un caz extrem al plafonului de
   prioritate?

Pentru mai mult: G. Buttazzo, *Hard Real-Time Computing Systems*, cap. 7 și
cap. 12; R. Barry, *Mastering the FreeRTOS Real Time Kernel*, capitolul despre
gestiunea resurselor; vezi și [Bibliografie](../further-reading.md).

Laborator asociat: [Lab 4](../../lab4-interrupts/README.md).
