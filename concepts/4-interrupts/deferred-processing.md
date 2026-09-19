# Procesare amânată

Citiți înainte: [Întreruperi și latență](interrupts-and-latency.md).

## 1. Problema

Un ISR rulează înaintea oricărui task, iar cât rulează el așteaptă toate
întreruperile de nivel egal sau mai mic. Dacă munca provocată de un eveniment
(filtrarea unei măsurători, decodarea unui pachet) se face *în* ISR, tot
sistemul plătește: niciun task nu rulează, iar latența celorlalte întreruperi
crește cu durata acelei munci.

Pe de altă parte, evenimentul trebuie tratat repede. Cum împăcăm „ISR scurt”
cu „răspuns rapid”?

## 2. Ideea

Soluția standard este **procesarea amânată** (deferred processing): munca se
împarte în două jumătăți.

```
eveniment ──> ISR: notează ce s-a întâmplat,   ──> task de tratare: face munca,
              trezește taskul, iese                la prioritatea aleasă de proiectant
              (microsecunde)                       (oricât durează)
```

ISR-ul, jumătatea de sus (top half), face doar ce nu poate aștepta: citește
valoarea din hardware, o păstrează și trezește un task. Taskul de tratare
(handler), jumătatea de jos (bottom half), este un task obișnuit, deci
planificatorul îl tratează după prioritate, ca pe oricare altul.

Câștigul este dublu. Întreruperile rămân scurte, deci latența tuturor
întreruperilor rămâne mică. Iar urgența muncii devine o *alegere*: proiectantul
îi dă taskului de tratare prioritatea potrivită, în loc ca munca să fie automat
mai presus de orice task doar pentru că se execută într-un ISR.

## 3. Teorie

**Semnalizarea din ISR.** ISR-ul are nevoie de un mecanism prin care să
trezească taskul. Există trei variante uzuale:

| Mecanism | Ce transmite | Potrivit pentru |
|---|---|---|
| **notificare** (task notification) sau semafor binar | „s-a întâmplat” | un eveniment fără date |
| notificare cu contor sau semafor numărător | „s-a întâmplat de $n$ ori” | evenimente care se pot aduna, fără date proprii |
| **coadă** (queue) | datele fiecărui eveniment, prin copiere | evenimente care poartă date și nu au voie să se piardă |

Un ISR nu se poate bloca, deci folosește doar operații care nu așteaptă. Dacă o
coadă este plină, ISR-ul nu poate aștepta să se elibereze loc: evenimentul se
pierde sau se numără ca pierdut. Dimensiunea cozii se alege deci din cea mai
lungă întârziere posibilă a taskului de tratare.

**Latența ISR–task** (ISR-to-task latency): timpul de la ISR până când rulează
taskul de tratare. Două lucruri o hotărăsc.

*Cererea de comutare la ieșirea din ISR.* Trezirea mută taskul de tratare în
starea gata de execuție, dar un ISR se întoarce, implicit, în taskul pe care
l-a întrerupt. Planificatorul reevaluează situația abia la următorul tick. Ca
taskul de tratare să ruleze imediat, ISR-ul trebuie să ceară explicit o
**comutare de context** la ieșire. Cererea are sens doar dacă taskul trezit
este mai prioritar decât cel întrerupt; altfel comutarea ar fi o cheltuială
inutilă, de aceea mecanismul de trezire îi spune ISR-ului dacă este cazul.

*Prioritatea taskului de tratare.* Odată trezit, taskul concurează cu
celelalte. Latența lui este timpul de răspuns al unui task obișnuit și se
analizează la fel (vezi
[Analiza timpului de răspuns](../3-analysis/response-time-analysis.md)):

$$
L_{ISR\text{–}task} \;\le\; c_{sw} \;+\; \text{interferența taskurilor mai prioritare}
$$

unde $c_{sw}$ este costul unei comutări de context. Fără cererea de comutare se
adaugă până la un tick.

**Evenimente care împart o trezire.** Dacă taskul de tratare întârzie mai mult
decât intervalul dintre două evenimente, mai multe evenimente ajung să fie
servite de o singură trezire. Un fanion simplu ar reține doar „cel puțin unul”;
un contor reține *câte*; doar o coadă reține și *datele fiecăruia*.

**Cine plătește.** Munca nu dispare, doar își schimbă locul. Mutată într-un
task de prioritate $p$, ea întârzie doar taskurile cu prioritate mai mică decât
$p$, adică exact pe cele care, prin proiect, își permit să aștepte.

## 4. Exemplu

Un eveniment periodic apare la fiecare 1 ms, tick-ul este de 1 ms, iar costul
unei comutări de context este neglijabil față de aceste valori.

| Situația | Ce așteaptă taskul de tratare | Latența ISR–task maximă |
|---|---|---|
| prioritate mare, ISR-ul cere comutarea | nimic | aproximativ $c_{sw}$ |
| prioritate mare, ISR-ul *nu* cere comutarea | următorul tick | până la 1 ms |
| prioritate sub un task care calculează 3 ms | sfârșitul acelui calcul | până la 3 ms |

În al treilea caz evenimentele vin la 1 ms, iar taskul poate întârzia 3 ms:
până la trei evenimente împart o singură trezire. Cu un contor, taskul află că
au fost trei; dacă fiecare purta o măsurătoare, primele două s-au pierdut, în
afară de cazul în care ISR-ul le-a pus într-o coadă de cel puțin trei locuri.

Al doilea exemplu: un alt periferic are nevoie de 300 µs de muncă la fiecare
eveniment. Făcută în ISR, această muncă adaugă până la 300 µs la latența
tuturor întreruperilor de nivel egal sau mai mic. Mutată într-un task de
prioritate medie, adaugă 0 la latența întreruperilor și 0 la taskurile mai
prioritare; cele 300 µs le plătesc doar taskurile de sub el.

## 5. Greșeli frecvente

- **„În ISR munca se face cel mai repede, deci acolo îi este locul.”** Se face
  repede pentru acel eveniment și pe socoteala tuturor celorlalte.
- **„ISR-ul a fost punctual, deci sistemul a răspuns la timp.”** Răspunsul este
  gata când termină taskul de tratare. O prioritate prost aleasă anulează un
  ISR perfect.
- **„Un fanion ajunge ca să nu pierd evenimente.”** Un fanion nu numără. Dacă
  taskul poate întârzia peste intervalul dintre evenimente, este nevoie de un
  contor sau de o coadă.

## 6. Întrebări de verificare

1. De ce se întoarce un ISR, implicit, în taskul întrerupt și nu în cel mai
   prioritar task gata de execuție? Ce cost ar avea varianta cealaltă?
2. Un task de tratare are prioritatea cea mai mică din sistem. Ce limită
   superioară are latența lui și de ce depinde?
3. Evenimentele sosesc la 2 ms, iar taskul de tratare poate întârzia cel mult
   7 ms. Câte locuri trebuie să aibă coada ca să nu se piardă nimic?
4. Ce fel de muncă trebuie totuși făcută în ISR și nu poate fi amânată?

Pentru mai mult: R. Barry, *Mastering the FreeRTOS Real Time Kernel*, capitolul
despre gestiunea întreruperilor; A. Burns, A. Wellings, *Real-Time Systems and
Programming Languages*; vezi și [Bibliografie](../further-reading.md).

Laborator asociat: [Lab 4](../../lab4-interrupts/README.md).
